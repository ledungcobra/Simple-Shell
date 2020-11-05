#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#define MAX_LENGTH 80

int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

char *lsh_read_line(void)
{
    char *buffer = NULL;
    size_t bufsize = 0;
    if (getline(&buffer, &bufsize, stdin) == -1)
    {
        if (feof(stdin))
            exit(EXIT_SUCCESS); // We receive an EOF
        else
        {
            perror("readline");
            exit(EXIT_FAILURE);
        }
    }
    return buffer;
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

char **lsh_split_line(char *line)
{
    int bufsize = LSH_TOK_BUFSIZE, pos = 0;
    char **tokens = (char **)malloc(bufsize * sizeof(char *));
    if (!tokens)
    {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }
    char *token = NULL;
    token = strtok(line, LSH_TOK_DELIM);
    while (token != NULL)
    {
        tokens[pos++] = token;
        if (pos >= bufsize)
        {
            bufsize += LSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens)
            {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, LSH_TOK_DELIM);
    }
    tokens[pos] = NULL;
    return tokens;
}

char *redirect[] = {">", "<"};
int lsh_num_redirect()
{
    return sizeof(redirect) / sizeof(char *);
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
    for (int i = 0; i < lsh_num_redirect(); i++)
    {
        if (strcmp(args[n - 2], redirect[i]) == 0)
        {
            return 1;
        }
    }
    return 0;
}

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

void createPipe(char **args, int pos)
{
    char **args1 = (char**)malloc(MAX_LENGTH * sizeof(char*));
    if (!args1)
    {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }
    char **args2 = (char**)malloc(MAX_LENGTH * sizeof(char*));
    if (!args2)
    {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }
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
        int pid1 = fork();
        if (pid1 == 0)
        {
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
        // Child process
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

int lsh_launch(char **args)
{
    pid_t pid, wpid;
    int status;
    char *filename = NULL;
    int n = argsCount(args);
    pid = fork();

    if (pid == 0)
    {
        //Child process
        if (n >= 3)
        {
            if (checkRedirectCommand(args))
            {
                redirectIO(args, filename);
            }
            int pos = checkPipeCommand(args);
            if(pos != 0)
            {
                createPipe(args, pos);
            }
        }
        if (execvp(args[0], args) == -1)
        {
            perror("lsh");
            exit(EXIT_FAILURE);
        } 
    }
    else if (pid < 0)
    {
        perror("lsh");
        exit(EXIT_FAILURE);
    }
    else
    {
        do
        {
            wpid = waitpid(pid, &status, WCONTINUED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}

char *builtin_str[] = {
    "cd", "help", "exit"};

int (*builtin_func[])(char **) = {
    &lsh_cd, &lsh_help, &lsh_exit};

int lsh_num_builtins()
{
    return sizeof(builtin_str) / sizeof(char *);
}

int lsh_cd(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "lsh: expected argument to \"cd\"\n");
    }
    else
    {
        if (chdir(args[1]) != 0)
        {
            perror("lsh");
        }
    }
    return 1;
}

int lsh_help(char **args)
{
    int i;
    printf("Stephen Brennan's LSH\n");
    printf("Type program names and arguments, and hit enter.\n");
    printf("The following are built in:\n");

    for (i = 0; i < lsh_num_builtins(); i++)
    {
        printf("  %s\n", builtin_str[i]);
    }

    printf("Use the man command for information on other programs.\n");
    return 1;
}

int lsh_exit(char **args)
{
    return 0;
}

int lsh_execute(char **args)
{
    int i;
    if (args[0] == NULL)
    {
        // An empty command was entered.
        return 1;
    }
    for (i = 0; i < lsh_num_builtins(); i++)
    {
        if (strcmp(args[0], builtin_str[i]) == 0)
        {
            return (*builtin_func[i])(args);
        }
    }
    return lsh_launch(args);
}

void lsh_loop(void)
{
    char *line;
    char **args;
    int status;
    do
    {
        /* code */
        // while(line != NULL)
        // {
        //     char *line_buf;
        //     size_t size = 0;
        //     getline(&line_buf, &size, stdin);
        //     fflush(stdin);
        // }
        printf("osh>");
        fflush(stdout);
        line = lsh_read_line(); // read the input from the user
        args = lsh_split_line(line);
        status = lsh_execute(args);

        free(line);
        free(args);
    } while (status);
}

int main(int argc, char *argv[])
{
    // Load config files, if any.
    // Run command loop.
    lsh_loop();
    // Perform any shutdown/cleanup.
    return EXIT_SUCCESS;
}
