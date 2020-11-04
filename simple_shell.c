#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#define MAX_LENGTH 80

typedef char *string;

//Dung de sao chep mot chuoi dang char *
string NewString(string src)
{
    string temp = (string)malloc(sizeof(char) * (strlen(src) + 1));
    strcpy(temp, src);
    return temp;
}
//Phan tach chuoi bang cach dung khoang trang de phan tach
string *stringTokenizer(string src)
{
    string *result = NULL;
    result = (string *)malloc(sizeof(string) * MAX_LENGTH);

    memset(result, NULL, MAX_LENGTH);

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
//Dem so luong ki chuoi co trong mang listStings
int countString(string *listStrings)
{
    int i = 0;
    for (i = 0; listStrings[i] && i < MAX_LENGTH; i++)
    {
        //DO Nothing
    }
    return i;
}
//Kiem tra su xuat hien cua ki tu & trong tham so truyen vao
//Neu ki tu & nam o cuoi cung thi tra ve 1
//Nguoc lai tra ve 0
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
//Tim vi tri xuat hien cua mot ki tu trong mang co trong mang
int findPosForExistingCharacter(string *args, string character)
{
    int count = countString(args);
    int result = -1;

    for (int i = 0; i < count; i++)
    {
        if (strcmp(args[i], character) == 0)
        {
            result = i;
            break;
        }
    }

    return result;
}
//Tim vi tri xuat hien cua > hoac < co trong danh sach cac tham so
int findPosRedirectChar(string *args, string character)
{

    int result = -1;
    int count = countString(args);
    for (int i = 0; i < count; i++)
    {
        if (strcmp(args[i], character) == 0)
        {
            result = i;
            break;
        }
    }

    return result;
}
//Xoa ten file la ki hieu redirect trong tham so dong lenh
void deleteFileNameAndRedirectCheck(string **args, int redirectPos)
{

    int count = countString(*args);

    free((*args)[count - 1]);
    free((*args)[count - 2]);

    (*args)[count - 2] = NULL;
}
//Phan tach args khi co ki tu pipe trong command
int seperateArgs(string *src, int posPipeChar, string **args1, string **args2)
{

    int count = countString(src);

    *args1 = (string *)malloc(sizeof(string) * (posPipeChar + 1));
    memset(*args1, NULL, posPipeChar + 1);

    int x = count - posPipeChar - 1;

    *args2 = (string *)malloc(sizeof(string) * (x + 1));
    memset(*args2, 0, x + 1);

    for (int i = 0; i < posPipeChar; i++)
    {
        (*args1)[i] = src[i];
    }
    int j = 0;

    for (int i = posPipeChar + 1; i <= count - 1; i++)
    {
        (*args2)[j++] = src[i];
    }
}
//Thuc hien khoi chay tien trinh con
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

// //DEBUGING
// void printListString(string *src)
// {
//     fflush(stdin);

//     for (int i = 0; i < countString(src); i++)
//     {
//         printf("%s\n", src[i]);
//     }
// }


//Tao duong ong 
//@param args1: tham so cua tien trinh 1 
//@param args2: tham so cua tien trinh 2
void createPipe(string *args1, string *args2)
{
    //Tao kenh giao tiep giua tien trinh con va tien trinh cha
    int fileDescs[2];
    pipe(fileDescs);
    
    //Tien hanh clone tien trinh hien tai
    pid_t pid1 = fork();

    //Neu clone tien trinh thanh cong
    if (pid1 == 0)
    {
        // Ket qua tu STDOUT_FILENO chuyen sang fileDescs[1] (Write-Only-File)
        dup2(fileDescs[1], STDOUT_FILENO);
        //Dong file  dung de doc do khong dung
        close(fileDescs[0]);
        executeChild(&args1);
    }
    else if (pid1 > 0) // Neu day la tien trinh cha
    {

        //Clone lai tien trinh cha hien tai
        pid_t pid2 = fork();
        //Neu day la tien trinh con
        if (pid2 == 0)
        {
            //Child
            //Chuyen doi kenh truyen tu lieu nhap vao la file trung gian duoc tao 
            //tu qua trinh pipe
            dup2(fileDescs[0], STDIN_FILENO);
            //Dong file ghi do khong dung
            close(fileDescs[1]);
            executeChild(&args2);

        }
        else if (pid2 > 0)   //Neu day la tien trinh cha 
        {
            close(fileDescs[0]);
            close(fileDescs[1]);
            wait(NULL);

        }
        else
        {
            printf("Error forking");
        }
    }
    else
    {
        //ERROR
        printf("ERROR");
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
            break;
        }

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