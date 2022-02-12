
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    char *str;
    int size = 0;
    printf("Enter size(MB): ");
    scanf("%d", &size);
    str = (char *)malloc(size * 1024 * 1024);
    if(str==NULL)
    {
        printf("malloc failed\n");
    }
    while (1)
    {
        sleep(100);
    }
    return 0;
}
