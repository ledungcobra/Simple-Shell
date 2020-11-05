#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#define MAX_LENGTH 80

int executeChild(string **args)
{

    //In child process
    int posRedirectWrite = findPosRedirectChar(*args, ">");
    int posRedirectRead = findPosRedirectChar(*args, "<");
    int fd = -1;

    //Kiem tra neu ton tai ki hieu > trong args
    if (posRedirectWrite != -1)
    {

        string filename = NewString((*args)[posRedirectWrite + 1]);
        fd = open(filename, O_CREAT | O_WRONLY, 0666);
        dup2(fd, STDOUT_FILENO);
        deleteFileNameAndRedirectCheck(args, posRedirectWrite);
    }
    //Kiem tra neu ton tai ki hieu < trong args
    if (posRedirectRead != -1)
    {
        string filename = NewString((*args)[posRedirectRead + 1]);
        fd = open(filename, O_RDONLY);
        dup2(fd, STDIN_FILENO);
        deleteFileNameAndRedirectCheck(args, posRedirectRead);
    }
    // Tien hanh chay tien trinh con: 
    //@Tham so dau la ten file,
    //@Tham so thu hai la danh sach tham so
    execvp((*args)[0], *args);
}

int checkForAmpersandCharacter(string *args)
{

    int count = countString(args);
    int result;
    if (strcmp(args[count - 1], "&") == 0)
    {
        result = 1;
    }
    else
    {
        result = 0;
    }

    return result;
}

int argsCount(char **args)
{
    int i;
    for (i = 0; args[i] != NULL; i++)
        ;
    return i;
}

int checkRedirectCommand(char **args)
{
    int n = argsCount(args);
    for (int i = 0; i < 2; i++)//vi chi co "<" và ">" nen chi co 2 vong lap
    {
        if (strcmp(args[n - 2], redirect[i]) == 0)
        {
            return 1;
        }
    }
    return 0;
}

//redirect input output
void redirectIO(char **args, char *filename)
{
    int file;
    int n = argsCount(args);
    filename = (char *)malloc(LSH_TOK_BUFSIZE * sizeof(char));
    if (filename)
        strcpy(filename, args[n - 1]);
    if (strcmp(args[n - 2], ">") == 0)
    {
        file = open(filename, O_WRONLY | O_CREAT, 0777);
        if (file == -1)
        {
            printf("Could not find the file to write\n");
            return;
        }
        if (dup2(file, STDOUT_FILENO) == -1)
        {
            printf("dup error\n");
            exit(EXIT_FAILURE);
        }
    }
    else if (strcmp(args[n - 2], "<") == 0)
    {
        filename = args[n - 1];
        file = open(filename, O_RDONLY);
        if (file == -1)
        {
            printf("Could not open the file to read\n");
            exit(EXIT_FAILURE);
        }
        if (dup2(file, STDIN_FILENO) == -1)
        {
            printf("dup error\n");
            exit(EXIT_FAILURE);
        }
    }
    char *temp;
    temp = args[0];
    free(args);
    args[0] = temp;
    args[1] = NULL;

    if (execvp(args[0], args) == -1)
    {
        printf("osh\n");
        exit(EXIT_FAILURE);
    }
    close(file);
}

int checkPipeCommand(char **args)
{
    int n = argsCount(args);
    for (int i = 0; i < n; i++)
    {
        if (strcmp(args[i], "|") == 0)
        {
            return i;
        }
    }
    return 0;
}

//Ham xu ly mang chuoi input, gan chuoi truoc | cho args1, sau | cho args2
void splitPipeCommand(char **args1, char **args2, char **args, int pos)
{
    int n = argsCount(args);
    int j = 0, k = 0, i = 0;

    while (i < pos && i < n)
    {
        args1[j++] = args[i++];
    }
    i = pos + 1;
    while (i > pos && i < n)
    {
        args2[k++] = args[i++];
    }
    args1[j] = NULL;
    args2[k] = NULL;
}

//Tao Pipe 
void createPipe(char **args, int pos)
{
    //Tham so dau vao cua child process 1
    char **args1 = (char**)malloc(MAX_LENGTH * sizeof(char*));
    if (!args1)
    {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    //Tham so dau vao cua child process 2
    char **args2 = (char**)malloc(MAX_LENGTH * sizeof(char*));
    if (!args2)
    {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    //Split 
    splitPipeCommand(args1, args2, args, pos);
    pid_t pid, wpid, ret;
    int status;

    int fd[2];
    //fd[0] - read
    // fd[1] - write
    if (pipe(fd) == -1)
    {
        printf("An error occurred with opening the pipe\n");
        exit(EXIT_FAILURE);
    }
    pid = fork();
    if (pid == 0)
    {
        // Child process(pid)
        int pid1 = fork();
        if (pid1 == 0)
        {
            //Chuld process(pid 1)
            close(fd[1]);
            if (dup2(fd[0], STDIN_FILENO) < 0)
            {
                perror("dup error\n");
                exit(EXIT_FAILURE);
            }
            close(fd[0]);
            if (execvp(args2[0], args2) == -1)
            {
                perror("lsh\n");
                exit(EXIT_FAILURE);
            }
        }
        //Parent Process (pip1)
        close(fd[0]);
        if (dup2(fd[1], STDOUT_FILENO) < 0)
        {
            perror("dup error\n");
            exit(EXIT_FAILURE);
        }
        close(fd[1]);
        if (execvp(args1[0], args1) == -1)
        {
            perror("lsh\n");
            exit(EXIT_FAILURE);
        }
        
        ret = waitpid(pid1, &status, WCONTINUED);
        
    }
    else if (pid < 0)
    {
        perror("lsh\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        //Parent process
        do
        {
            wpid = waitpid(pid, &status, WCONTINUED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status) && !__WIFCONTINUED(status));
    }

    exit(EXIT_SUCCESS);
}

int main()
{
    string *args; /* command line arguments */

    int should_run = 1; /* flag to determine when to exit program */
    string command = (string)malloc(sizeof(char) * MAX_LENGTH + 1);
    string history = NULL;

    int hasAmpersandChar = 0;

    while (should_run)
    {

        printf("osh>");
        fflush(stdin);
        gets(command);

        if (strcmp(command, "exit") == 0)
        {
            break;
        }

        //History
        if (strcmp(command, "!!") == 0)
        {
            if (history)
            {
                strcpy(command, history);
                printf("%s\n", history);
            }
            else
            {
                printf("No commands in history.");
            }
        }

        free(history);
        history = NULL;
        history = NewString(command);

        args = stringTokenizer(command);

        hasAmpersandChar = checkForAmpersandCharacter(args);
        // int argsCount = countString(args);

        pid_t pid = fork();

        if (pid == 0)
        {
            //Khi la children

            int posPipeChar = findPosForExistingCharacter(args, "|");

            if (posPipeChar != -1)
            {

                string *args1 = NULL;
                string *args2 = NULL;

                seperateArgs(args, posPipeChar, &args1, &args2);
                createPipe(args1, args2);
            }
            else
            {

                executeChild(&args);
            }
        }
        else if (pid > 0)
        {
            //From parent Process
            if (!hasAmpersandChar)
            {
                wait(NULL);
            }
        }
        else
        {
            printf("Fork Error");
        }
    }

    free(command);

    return 0;
}