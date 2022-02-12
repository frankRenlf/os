#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <errno.h>
#include <pwd.h>
#include <dirent.h>

#define BUF_SZ 256
#define TRUE 1
#define FALSE 0

const char* COMMAND_INFO = "info";
const char* COMMAND_MYGREP = "mygrep";
const char* COMMAND_EXIT = "exit";
const char* COMMAND_EX = "ex";
const char* COMMAND_HELP = "help";
const char* COMMAND_CD = "cd";
const char* COMMAND_IN = "<";
const char* COMMAND_OUT = ">";
const char* COMMAND_PIPE = "|";

// 内置的状态码
enum {
	RESULT_NORMAL,
	ERROR_FORK,
	ERROR_COMMAND,
	ERROR_WRONG_PARAMETER,
	ERROR_MISS_PARAMETER,
	ERROR_TOO_MANY_PARAMETER,
	ERROR_CD,
	ERROR_SYSTEM,
	ERROR_EXIT,

	/* 重定向的错误信息 */
	ERROR_MANY_IN,
	ERROR_MANY_OUT,
	ERROR_FILE_NOT_EXIST,
	
	/* 管道的错误信息 */
	ERROR_PIPE,
	ERROR_PIPE_MISS_PARAMETER
};

char username[BUF_SZ];
char hostname[BUF_SZ];
char curPath[BUF_SZ];
char commands[BUF_SZ][BUF_SZ];

int isCommandExist(const char* command);
void getUsername();
void getHostname();
int getCurWorkDir();
int splitCommands(char command[BUF_SZ]);
int callExit();
int callCommand(int commandNum);
int callCommandWithPipe(int left, int right);
int callCommandWithRedi(int left, int right);
int callCd(int commandNum);

int main() {
	/* 获取当前工作目录、用户名、主机名 */
	int result = getCurWorkDir();
	if (ERROR_SYSTEM == result) {
		fprintf(stderr, "\e[31;1mError: System error while getting current work directory.\n\e[0m");
		exit(ERROR_SYSTEM);
	}
	getUsername();
	getHostname();

	/* 启动myshell */
	char argv[BUF_SZ];
	while (TRUE) {
		printf("\e[32;1m%s@%s:%s\e[0m$ ", username, hostname,curPath); // 显示为绿色
		/* 获取用户输入的命令 */
		fgets(argv, BUF_SZ, stdin);
		int len = strlen(argv);
		if (len != BUF_SZ) {
			argv[len-1] = '\0';
		}

		int commandNum = splitCommands(argv);
		
		if (commandNum != 0) { // 用户有输入指令
			
			if (strcmp(commands[0], COMMAND_EXIT) == 0) { // exit命令
				printf("Terminate\n");
				result = callExit();
				if (ERROR_EXIT == result) {
					exit(-1);
				}
			}else if (strcmp(commands[0], COMMAND_INFO) == 0){ //info_command
				printf("COMP2211 Simplified Shell by sc19lr\n");
			}else if (strcmp(commands[0], COMMAND_EX) == 0){ //ex
				char *myargs[3];
				myargs[0]=strdup("./");
				strcat(myargs[0],commands[1]);
				myargs[1]=strdup(commands[2]);
				myargs[2]=NULL;
				execvp(myargs[0],myargs);
			}else if (strcmp(commands[0], COMMAND_MYGREP) == 0){ //mygrep
				FILE *fp;
				char temp[200];
				int sum = 0;
				fp = fopen(commands[2],"r");
				while(!feof(fp))
				{
					fgets(temp,1000,fp);
					if(strstr(temp,commands[1]))
					{
						printf("%s",temp);
						sum++;
					}
				}
				printf("\nsum: %d\n",sum);
				
			}else if (strcmp(commands[0], COMMAND_CD) == 0) { // cd命令
				result = callCd(commandNum);
				switch (result) {
					case ERROR_MISS_PARAMETER:
						fprintf(stderr, "\e[31;1mError: Miss parameter while using command \"%s\".\n\e[0m"
							, COMMAND_CD);
						break;
					case ERROR_WRONG_PARAMETER:
						fprintf(stderr, "\e[31;1mError: No such path \"%s\".\n\e[0m", commands[1]);
						break;
					case ERROR_TOO_MANY_PARAMETER:
						fprintf(stderr, "\e[31;1mError: Too many parameters while using command \"%s\".\n\e[0m"
							, COMMAND_CD);
						break;
					case RESULT_NORMAL: // cd命令正常执行，更新当前工作l目录
						result = getCurWorkDir();
						if (ERROR_SYSTEM == result) {
							fprintf(stderr
								, "\e[31;1mError: System error while getting current work directory.\n\e[0m");
							exit(ERROR_SYSTEM);
						} else {
							break;
						}
				}
			} else { // 其它命令
				result = callCommand(commandNum);
				switch (result) {
					case ERROR_FORK:
						fprintf(stderr, "\e[31;1mError: Fork error.\n\e[0m");
						exit(ERROR_FORK);
					case ERROR_COMMAND:
						fprintf(stderr, "\e[31;1mError: Command not exist in myshell.\n\e[0m");
						break;
					case ERROR_MANY_IN:
						fprintf(stderr, "\e[31;1mError: Too many redirection symbol \"%s\".\n\e[0m", COMMAND_IN);
						break;
					case ERROR_MANY_OUT:
						fprintf(stderr, "\e[31;1mError: Too many redirection symbol \"%s\".\n\e[0m", COMMAND_OUT);
						break;
					case ERROR_FILE_NOT_EXIST:
						fprintf(stderr, "\e[31;1mError: Input redirection file not exist.\n\e[0m");
						break;
					case ERROR_MISS_PARAMETER:
						fprintf(stderr, "\e[31;1mError: Miss redirect file parameters.\n\e[0m");
						break;
					case ERROR_PIPE:
						fprintf(stderr, "\e[31;1mError: Open pipe error.\n\e[0m");
						break;
					case ERROR_PIPE_MISS_PARAMETER:
						fprintf(stderr, "\e[31;1mError: Miss pipe parameters.\n\e[0m");
						break;
				}
			}
		}
	}
}

int isCommandExist(const char* command) { // 判断指令是否存在
	if (command == NULL || strlen(command) == 0) return FALSE;

	int result = TRUE;
	
	int fds[2];
	if (pipe(fds) == -1) {
		result = FALSE;
	} else {
		/* 暂存输入输出重定向标志 */
		int inFd = dup(STDIN_FILENO);
		int outFd = dup(STDOUT_FILENO);

		pid_t pid = vfork();
		if (pid == -1) {
			result = FALSE;
		} else if (pid == 0) {
			/* 将结果输出重定向到文件标识符 */
			close(fds[0]);
			dup2(fds[1], STDOUT_FILENO);
			close(fds[1]);

			char tmp[BUF_SZ];
			sprintf(tmp, "command -v %s", command);
			system(tmp);
			exit(1);
		} else {
			waitpid(pid, NULL, 0);
			/* 输入重定向 */
			close(fds[1]);
			dup2(fds[0], STDIN_FILENO);
			close(fds[0]);

			if (getchar() == EOF) { // 没有数据，意味着命令不存在
				result = FALSE;
			}
			
			/* 恢复输入、输出重定向 */
			dup2(inFd, STDIN_FILENO);
			dup2(outFd, STDOUT_FILENO);
		}
	}

	return result;
}

void getUsername() { // 获取当前登录的用户名
	struct passwd* pwd = getpwuid(getuid());
	strcpy(username, pwd->pw_name);
}

void getHostname() { // 获取主机名
	gethostname(hostname, BUF_SZ);
}

int getCurWorkDir() { // 获取当前的工作目录
	char* result = getcwd(curPath, BUF_SZ);
	if (result == NULL)
		return ERROR_SYSTEM;
	else return RESULT_NORMAL;
}

int splitCommands(char command[BUF_SZ]) { // 以空格分割命令， 返回分割得到的字符串个数
	int num = 0;
	int i, j;
	int len = strlen(command);

	for (i=0, j=0; i<len; ++i) {
		if (command[i] != ' ') {
			commands[num][j++] = command[i];
		} else {
			if (j != 0) {
				commands[num][j] = '\0';
				++num;
				j = 0;
			}
		}
	}
	if (j != 0) {
		commands[num][j] = '\0';
		++num;
	}

	return num;
}

int callExit() { // 发送terminal信号退出进程
	pid_t pid = getpid();
	if (kill(pid, SIGTERM) == -1) 
		return ERROR_EXIT;
	else return RESULT_NORMAL;
}

int callCommand(int commandNum) { // 给用户使用的函数，用以执行用户输入的命令
	pid_t pid = fork();
	if (pid == -1) {
		return ERROR_FORK;
	} else if (pid == 0) {
		/* 获取标准输入、输出的文件标识符 */
		int inFds = dup(STDIN_FILENO);
		int outFds = dup(STDOUT_FILENO);

		int result = callCommandWithPipe(0, commandNum);
		
		/* 还原标准输入、输出重定向 */
		dup2(inFds, STDIN_FILENO);
		dup2(outFds, STDOUT_FILENO);
		exit(result);
	} else {
		int status;
		waitpid(pid, &status, 0);
		return WEXITSTATUS(status);
	}
}

int callCommandWithPipe(int left, int right) { // 所要执行的指令区间[left, right)，可能含有管道
	if (left >= right) return RESULT_NORMAL;
	/* 判断是否有管道命令 */
	int pipeIdx = -1;
	for (int i=left; i<right; ++i) {
		if (strcmp(commands[i], COMMAND_PIPE) == 0) {
			pipeIdx = i;
			break;
		}
	}
	if (pipeIdx == -1) { // 不含有管道命令
		return callCommandWithRedi(left, right);
	} else if (pipeIdx+1 == right) { // 管道命令'|'后续没有指令，参数缺失
		return ERROR_PIPE_MISS_PARAMETER;
	}

	/* 执行命令 */
	int fds[2];
	if (pipe(fds) == -1) {
		return ERROR_PIPE;
	}
	int result = RESULT_NORMAL;
	pid_t pid = vfork();
	if (pid == -1) {
		result = ERROR_FORK;
	} else if (pid == 0) { // 子进程执行单个命令
		close(fds[0]);
		dup2(fds[1], STDOUT_FILENO); // 将标准输出重定向到fds[1]
		close(fds[1]);
		
		result = callCommandWithRedi(left, pipeIdx);
		exit(result);
	} else { // 父进程递归执行后续命令
		int status;
		waitpid(pid, &status, 0);
		int exitCode = WEXITSTATUS(status);
		
		if (exitCode != RESULT_NORMAL) { // 子进程的指令没有正常退出，打印错误信息
			char info[4096] = {0};
			char line[BUF_SZ];
			close(fds[1]);
			dup2(fds[0], STDIN_FILENO); // 将标准输入重定向到fds[0]
			close(fds[0]);
			while(fgets(line, BUF_SZ, stdin) != NULL) { // 读取子进程的错误信息
				strcat(info, line);
			}
			printf("%s", info); // 打印错误信息
			
			result = exitCode;
		} else if (pipeIdx+1 < right){
			close(fds[1]);
			dup2(fds[0], STDIN_FILENO); // 将标准输入重定向到fds[0]
			close(fds[0]);
			result = callCommandWithPipe(pipeIdx+1, right); // 递归执行后续指令
		}
	}

	return result;
}

int callCommandWithRedi(int left, int right) { // 所要执行的指令区间[left, right)，不含管道，可能含有重定向
	if (!isCommandExist(commands[left])) { // 指令不存在
		return ERROR_COMMAND;
	}	

	/* 判断是否有重定向 */
	int inNum = 0, outNum = 0;
	char *inFile = NULL, *outFile = NULL;
	int endIdx = right; // 指令在重定向前的终止下标

	for (int i=left; i<right; ++i) {
		if (strcmp(commands[i], COMMAND_IN) == 0) { // 输入重定向
			++inNum;
			if (i+1 < right)
				inFile = commands[i+1];
			else return ERROR_MISS_PARAMETER; // 重定向符号后缺少文件名

			if (endIdx == right) endIdx = i;
		} else if (strcmp(commands[i], COMMAND_OUT) == 0) { // 输出重定向
			++outNum;
			if (i+1 < right)
				outFile = commands[i+1];
			else return ERROR_MISS_PARAMETER; // 重定向符号后缺少文件名
				
			if (endIdx == right) endIdx = i;
		}
	}
	/* 处理重定向 */
	if (inNum == 1) {
		FILE* fp = fopen(inFile, "r");
		if (fp == NULL) // 输入重定向文件不存在
			return ERROR_FILE_NOT_EXIST;
		
		fclose(fp);
	}
	
	if (inNum > 1) { // 输入重定向符超过一个
		return ERROR_MANY_IN;
	} else if (outNum > 1) { // 输出重定向符超过一个
		return ERROR_MANY_OUT;
	}

	int result = RESULT_NORMAL;
	pid_t pid = vfork();
	if (pid == -1) {
		result = ERROR_FORK;
	} else if (pid == 0) {
		/* 输入输出重定向 */
		if (inNum == 1)
			freopen(inFile, "r", stdin);
		if (outNum == 1)
			freopen(outFile, "w", stdout);

		/* 执行命令 */
		char* comm[BUF_SZ];
		for (int i=left; i<endIdx; ++i)
			comm[i] = commands[i];
		comm[endIdx] = NULL;
		execvp(comm[left], comm+left);
		exit(errno); // 执行出错，返回errno
	} else {
		int status;
		waitpid(pid, &status, 0);
		int err = WEXITSTATUS(status); // 读取子进程的返回码

		if (err) { // 返回码不为0，意味着子进程执行出错，用红色字体打印出错信息
			printf("\e[31;1mError: %s\n\e[0m", strerror(err));
		}
	}


	return result;
}

int callCd(int commandNum) { // 执行cd命令
	int result = RESULT_NORMAL;

	if (commandNum < 2) {
		result = ERROR_MISS_PARAMETER;
	} else if (commandNum > 2) {
		result = ERROR_TOO_MANY_PARAMETER;
	} else {
		int ret = chdir(commands[1]);
		if (ret) result = ERROR_WRONG_PARAMETER;
	}

	return result;
}



// int arg=0;//命令个数
// char buf[1024];//读入字符串
// char *command[100];//切分后的字符串数组
// int pid;//设为全局变量，方便获得子进程的进程号
// char *f="temp.txt";//共享文件
// void sigcat(){
// 	kill(pid,SIGINT);
// }
// //read
// void read_command() {
// 	char *temp=strtok(buf," ");
// 	int i=0;
// 	while(temp) {
// 		command[i++]=temp;
// 		temp=strtok(NULL," ");
// 	}
// 	arg=i;
// 	command[i]=0;//命令形式的字符串数组最后一位必须是NULL
// }

// int flag[100][2];//管道的输入输出重定向标记
// char *file[100][2]={0};//对应两个重定向的文件
// char *argv[100][100];//参数
// int ar=0;//管道个数
// //解析命令
// void analazy_command() {
// 	ar=0;
// 	for(int i=0;i<100;i++) {
// 		flag[i][0]=flag[i][1]=0;
// 		file[i][0]=file[i][1]=0;
// 		for(int j=0;j<100;j++) {
// 			argv[i][j]=0;
// 		}
// 	}
// 	for(int i=0;i<arg;i++) argv[0][i]=command[i];//初始化第一个参数
// 	argv[0][arg]=NULL;
// 	int a=0;//当前命令参数的序号
// 	for(int i=0;i<arg;i++) {
//         //判断是否存在管道
// 		if(strcmp(command[i],"|")==0) {//c语言中字符串比较只能用strcmp函数
// 			//printf("遇到 | 符号\n");
// 			argv[ar][a++]=NULL;
// 			ar++;
// 			a=0;
// 		}
// 		else if(strcmp(command[i],"<")==0) {//存在输入重定向
// 			flag[ar][0]=1;
// 			file[ar][0]=command[i+1];
// 			argv[ar][a++]=NULL;
// 		}
// 		else if(strcmp(command[i],">")==0) {//没有管道时的输出重定向
// 			flag[ar][1]=1;
// 			file[ar][1]=command[i+1];
// 			argv[ar][a++]=NULL;//考虑有咩有输入重定向的情况
// 		}
//         else argv[ar][a++]=command[i];
// 	}
// }

// //创建子进程，执行命令
// int do_command() {
// 	//printf("seccesee||\n");
// 	pid=fork();//创建的子进程
// 	if(pid<0) {
// 		perror("fork error\n");
//         exit(0);
// 	}
// 	//先判断是否存在管道，如果有管道，则需要用多个命令参数，并且创建新的子进程。否则一个命令即可
// 	else if(pid==0) {
// 		if(!ar) {//没有管道
// 			if(flag[0][0]) {//判断有无输入重定向
// 				close(0);
// 				int fd=open(file[0][0],O_RDONLY);
// 			}
// 			if(flag[0][1]) {//判断有无输出重定向
// 				close(1);
// 				int fd2=open(file[0][1],O_WRONLY|O_CREAT|O_TRUNC,0666);
// 			}
// 			execvp(argv[0][0],argv[0]);
// 		}
// 		else {//有管道
//             int tt;//记录当前遍历到第几个命令
// 			for(tt=0;tt<ar;tt++) {
// 				int pid2=fork();
// 				if(pid2<0) {
// 					perror("fork error\n");
// 					exit(0);
// 				}
// 				else if(pid2==0) {
// 					if(tt) {//如果不是第一个命令，则需要从共享文件读取数据
//                         close(0);
// 					    int fd=open(f,O_RDONLY);//输入重定向
//                     }
//                     if(flag[tt][0]) {
// 						close(0);
// 						int fd=open(file[tt][0],O_RDONLY);
// 					}
// 					if(flag[tt][1]) {
// 						close(1);
// 						int fd=open(file[tt][1],O_WRONLY|O_CREAT|O_TRUNC,0666);
// 					}		
//                     close(1);
//                     remove(f);//由于当前f文件正在open中，会等到解引用后才删除文件
//                     int fd=open(f,O_WRONLY|O_CREAT|O_TRUNC,0666);
// 					if(execvp(argv[tt][0],argv[tt])==-1) {
//                         perror("execvp error!\n");
//                         exit(0);
//                     }
// 				}
// 				else {//管道后的命令需要使用管道前命令的结果，因此需要等待
// 					waitpid(pid2,NULL,0);
// 				}
// 			}
//             //接下来需要执行管道的最后一条命令
// 			close(0);
// 			int fd=open(f,O_RDONLY);//输入重定向
// 			if(flag[tt][1]) {
// 				close(1);
// 				int fd=open(file[tt][1],O_WRONLY|O_CREAT|O_TRUNC,0666);
// 			}
// 			execvp(argv[tt][0],argv[tt]);
// 		}
// 	}
// 	//father
// 	else {
// 		waitpid(pid,NULL,0);
// 	}
// 	return 1;
// }
// int main(int argc, char *argv[]) {
// 	signal(SIGINT, &sigcat);//注册信号响应函数
// 	while(gets(buf))  {    //读入一行字符串
// 		//初始化
// 		for(int i=0;i<100;i++) command[i]=0;
// 		arg=0;//初始化参数个数
// 		read_command();//将字符串拆分成命令形式的字符串数组
// 		analazy_command();//分析字符串数组中的各种格式，如管道或者重定向
// 		do_command();//创建子进程执行命令
// 	}
// 	return 0;
// } 

