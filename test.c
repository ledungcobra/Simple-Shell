#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#define MAX_LENGTH 80

int main()
{
    
    int fd = open("./file.txt",O_WRONLY);

    dup2(fd,STDOUT_FILENO);
    


    // close(fd);



    return 0;
}
