#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#pragma GCC ignored - Wall
#define MAX_LENGTH 80

typedef char *string;

//Dung de sao chep mot chuoi dang char *
string NewString(string src)
{
    string temp = (string)malloc(sizeof(char) * (strlen(src) + 1));
    strcpy(temp, src);
    return temp;
}

string *stringTokenizer(string src)
{
    string *result = NULL;
    result = (string *)malloc(sizeof(char) * MAX_LENGTH);

    memset(result,NULL,MAX_LENGTH);

    int currentStringIndex = 0;

    string delim = NewString(" ");

    string *token = strtok(src, delim);

    while (token != NULL)
    {

        result[currentStringIndex++] = NewString(token);
        token = strtok(NULL, delim);
    }
    return result;
}

int countString(string *listStrings)
{
    int i = 0;
    for (i = 0; listStrings[i] && i < MAX_LENGTH; i++)
    {
    }
    return i;
}

int checkHasAmpersandCharacter(string * args){
    
    int count = countString(args);
    return !strcmp(args[count-1],"&");

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

        if (strcmp(command, "!!") == 0)
        {
            if (history != NULL)
            {
                strcpy(command, history);
                printf("%s\n", history);
            }
        }
        
        args = stringTokenizer(command);
        
        hasAmpersandChar = checkHasAmpersandCharacter(args);
        // int argsCount = countString(args);

        pid_t pid = fork();

        if (pid == 0)
        {
            //In child process
            execvp(args[0],args);
        }
        else if (pid > 0)
        {
            //From parent Process
            if(!hasAmpersandChar){
                wait(NULL);
            }

        }
        else
        {
            //Error
            perror("Error when fork");
        }

        free(history);
        history = NewString(command);
    }

    // while (should_run)
    // {

    //     printf("ssh>");
    //     fflush(stdout);

    //     pid_t pid = fork();

    //     if (pid == 0)
    //     {
    //         printf("Run in child process");
    //         execvp("echo", argv);
    //     }
    //     else
    //     {
    //         printf("Run in parent process");
    //         wait(NULL);
    //     }

    //     gets(command);
    // }

    return 0;
}