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
    result = (string *)malloc(sizeof(string) * MAX_LENGTH);

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
        //DO Nothing
    }
    return i;
}

int checkForAmpersandCharacter(string * args){
    
    int count = countString(args);
    int result;
    if(strcmp(args[count-1],"&")==0){
        result = 1;
    }else{
        result = 0;
    }


    return result;

}

int findPosForExistingCharacter(string *args,string character){
    int count = countString(args);
    int result = -1;
    
    for(int i=0;i<count;i++){
        if(strcmp(args[i],character) == 0){
            result = i;
            break;
        }
    }

    return result;

}

int findPosRedirectChar(string *args,string character){

    int result = -1;
    int count = countString(args);
    for(int i = 0;i<count;i++){
        if(strcmp(args[i],character) == 0){
            result = i;
            break;
        }
    }

    return result;

}

void deleteFileNameAndRedirectCheck(string** args,int redirectPos){
    
    int count = countString(*args);

    free(*args[count-1]);
    free(*args[count-2]);

    *args[count-2] = NULL;

}
// string* append(string* args,string value){

// }
//1 3
int seperateArgs(string *src,int posPipeChar,string** args1,string** args2){
    
    int count = countString(src);

    *args1 = (string*) malloc(sizeof(string)*(posPipeChar+1));
    memset(*args1,NULL,posPipeChar+1);

    int x = count-posPipeChar-1;    


    *args2 = (string*) malloc(sizeof(string)*(x+1));
    memset(*args2,0,x+1);


    for(int i=0;i<posPipeChar;i++){
        (*args1)[i] = src[i];

    }  
    int j = 0;

    for(int i=posPipeChar+1;i<=count-1;i++){
        (*args2)[j++] = src[i];
    }


}


int executeChild(string ** args){
    //In child process
    int posRedirectWrite = findPosRedirectChar(*args,">");
    int posRedirectRead = findPosRedirectChar(*args,"<");
   


    if(posRedirectWrite!=-1){
        string filename = NewString(*args[posRedirectWrite+1]);
        int fd = open(filename,O_WRONLY);
        dup2(fd,STDOUT_FILENO);
        deleteFileNameAndRedirectCheck(args,posRedirectWrite);
    }

    if(posRedirectRead!=-1){

        string filename = NewString(*args[posRedirectRead+1]);

        int fd = open(filename,O_RDONLY);
        dup2(fd,STDIN_FILENO);
        deleteFileNameAndRedirectCheck(args,posRedirectRead);
    }


    execvp((*args)[0],*args);
}


//DEBUGING
void printListString(string* src){
    fflush(stdin);

    for(int i = 0;i<countString(src);i++){
        printf("%s\n",src[i]);
    }
}

void createPipe(string * args1,string* arg2){
    int fileDescriptors[2];
    pipe(fileDescriptors);

    pid_t pid1 = fork();

    if( pid1 == 0){
        
        printf("Tien trinh mot running: \n");
        printListString(args1);

        //Chuyen write output sang fileDesciptors[1]
        dup2(fileDescriptors[1], STDOUT_FILENO);
        close(fileDescriptors[1]);
        executeChild(args1);

    }else if(pid1>0){
        //Parent
        int pid2 = fork();

        if(pid2 == 0){
            
            printf("Tien trinh 2 running");    
            
            dup2(fileDescriptors[0],STDIN_FILENO);
            close(fileDescriptors[1]);

        }else{
            
            close(fileDescriptors[1]);
            wait(NULL);

        }


    }else{
        //ERROR

    }

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
            should_run = 0;
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
       

        hasAmpersandChar = checkForAmpersandCharacter(args);
        // int argsCount = countString(args);

        
        if (fork() == 0)
        {  
            printf("run child");
            //Args include 
            int posPipeChar = findPosForExistingCharacter(args,"|");
            
            if(posPipeChar != -1){
                        
                string* args1 = NULL;  
                string* args2 = NULL;

                seperateArgs(args,posPipeChar,&args1,&args2);
                createPipe(args1,args2);
                               

            }else{
               
                executeChild(args);

            }

        }
        else 
        {
            //From parent Process
            if(!hasAmpersandChar){
                wait(NULL);
            }

        }
        
        free(history);
        history = NewString(command);
    }



    return 0;
}