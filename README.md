# os Programming a Shell
1. abstract
A simple shell implemented in C language can accept commands entered by users and perform operations, and support multiple pipelines and redirection.
After the program runs, it will simulate the shell, display the current user name, host name and path in green font, and wait for the user to enter the command. After the program reads the instructions entered by the user one by one, it divides the instructions into multiple string commands according to the space, and then judges the type of the command. If the command is wrong, the error message will be printed in red font.
If the command is exit, call the custom exit function and send a terminal signal to the program process to end the process.
If the command is CD, judge the parameters, call the chdir () function to modify the current path, and return the corresponding results. If the modification is successful, use the getcwd() function to update the current path.
In case of other commands, judge whether there is a legal pipeline first. If there is a pipeline, the command in front of the pipeline symbol is executed in the child process. After the parent process waits for the child process to finish, the command behind the pipeline symbol is processed recursively. If there is no pipeline, execute the command directly. When executing a command, first judge whether the command exists and whether there is a legal redirection, and then use execvp() to perform the corresponding operation.
2. New function
display the current user name, host name and working path
judge whether the command exists
execute external commands
recursive implementation of multiple pipelines
3 shell design
3.1 basic commands
To implement a simple shell in Linux system with C language, we first need to think that the shell needs to constantly analyze the input commands, so as a main process, we can't exit, and at the same time, we also need to constantly call execvp () to execute commands. To achieve this goal, you can use the fork () statement to create a child process and let the child process call the execvp () system call to execute the command. In this way, the parent process can continuously create a child process according to the input command. At the same time, the parent process does not have to exit to realize the function of the shell. Of course, before that, we need to preprocess. This is because we input a string. We need to cut the string according to the space and divide it into small strings as command parameters, and then execute.
3.2 redirection
After understanding this, introduce the processing method of input redirection: after cutting the input string into small strings, you can traverse the input string to identify whether there is "<" or ">" string, which indicates input redirection and output redirection respectively. When we encounter input redirection, our standard input is no longer the keyboard, but the file after the redirection character. In order to realize this function, we need to close (0) and then open ("XXX. TXT"), so that the file descriptor No. 0 is assigned to XXX Txt, and then when reading data from standard input, it will be from XXX Txt file read. Similarly, output redirection can close (1) first and then open the corresponding file. This solves the redirection problem.
3.3 single pipe
There are two kinds of pipelines: anonymous pipeline and named pipeline. Anonymous pipeline is mainly used for communication between parent and child processes. Here we use anonymous channels, Two commands are used as child processes and parent processes to realize communication (only one pipeline is considered for the time being). The idea of redirection is still used in. Because the essence of the pipeline is a file, it is enough to redirect the output of sub process 2 to the pipeline. However, the binding function of the pipeline is not like opening a file, but uses the dup (pipe [1]) system call to bind the writing end of the pipeline to the smallest free file descriptor. Of course, before that, close (1) is required. 
3.4 multistage pipeline
The above content can only solve the single pipeline problem. When we need to solve the problem of executing multiple commands in turn, we need to use multi-level pipelines, that is, the form of command1 | command2 | command3 |... | commandn. An effective method is recursive implementation, that is, first fork recursion to the bottom layer, execute command1, then out of the stack in turn, execute command2... Until the parent process executes commandn.
However, this method was not used in this experiment, but a simpler method was selected by using the essence of "shared file" of pipeline: the parent process maintains a shared file, uses the parent process to create child process 1, and redirects the output of the child process to the shared file; After the parent process reclaims process 1, fork the next child process 2, redirect the input of this process to the shared file, empty the shared file after reading data from the file, and redirect the output of this process to the shared file; Parent reclaims child process 2... Until the final parent reclaims child process n-1, then enter redirect to shared file and execute the last command n. The method of sharing files is also very simple. Since processes 1, 2... N-1 belong to the child processes created by the parent process and inherit the global variables of the parent process, the string representation of the file name can be defined as a global variable.In this way, multi-level pipeline is realized.
3.5 keyboard interrupt signal processing
In the actual shell, if a command is executing, you can interrupt the process by sending a SIGINT signal through Ctrl + C, but the shell itself will not exit. Therefore, the shell we implement must also meet this condition. To do this, we only need to define PID as a global variable and satisfy PID = fork(). After receiving the interrupt signal, send kill to the PID. Since the return value of the parent process is the process number of the child process, the child process can be killed to stop the currently running command of the shell. 

4 effect display
4.1 start myshell

The first line in the figure is the system shell. In order to distinguish from the system, I set the default information display of the customized shell to green. 
4.2 execute info, pwd, cd, execute, mygrep commands

cd: When the parameter is wrong, the corresponding error message will be printed in red font according to the situation.
4.3 executing external commands

4.4 redirect

4.5 pipe and others


5 difference
Linux shell implemented in Win32 is partially compatible, but there are many changes and incompatibilities In Win32, CreateProcess replaces fork to create a new process, CreateFile replaces open to open or create a file, and readfile replaces file to read a file Most calling functions become more detailed and understandable than Linux However, there are some incompatible system calls. For example, Win32 does not support mount, unmount and link In addition, Win32 needs security and signal protection, so the calls of Chmod and kill are deleted
6 reflection
The command types in the shell are not exactly the same. Some commands only include commands and parameters, while others include input files or other contents in addition to commands and parameters. What can be passed through the pipeline is the input content, not the command parameters. The essence of pipeline is still a file, which is just a file that can be shared by multiple processes, so multiple processes can transfer data through this file. According to the essence of the pipeline, it is a shared file, so the pipeline is implemented based on file operation. You can directly skip the step of establishing the pipeline and realize multi pipeline communication directly through the shared file. In this process, many processes need to be involved. The input is redirected in the shared file and the output is redirected in the shared file. After querying the official documents, it is found that when the file is in the open state, the remove () function will not be executed immediately, but wait until the end of the open. This avoids the problem that the data file is deleted before it is read from the shared file.
7 reference
[1] JiZhong, 2021. bilibili. [Online]
Available at: https://www.bilibili.com/video/BV1f5411E7Vy?from=search&seid=10943294839567115347&spm_id_from=333.337.0.0[Accessed 6 December 2021].

[2] Peter Baer Galvin, Greg Gagne, 2002. OPERATING SYSTEM CONCEPTS. 7th ed. New York: 高等教育出版社·北京.

[3] SunShine, 2019. CSDN. [Online]
Available at: https://blog.csdn.net/LH0912666/article/details/87897629?ops_request_misc=%257B%2522request%255Fid%2522%253A%2522163930813816780255286899%2522%252C%2522scm%2522%253A%252220140713.130102334..%2522%257D&request_id=163930813816780255286899&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~sobaiduend~default-5-87897629.pc_search_result_control_group&utm_term=linux+shell%E5%91%BD%E4%BB%A4&spm=1018.2226.3001.4187[Accessed 5 December 2021].

[4] WeiXin, 2020. CSDN. [Online]
Available at: https://blog.csdn.net/weixin_39856055/article/details/111274690
[Accessed 11 December 2021].
