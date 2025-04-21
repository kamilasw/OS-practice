#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define MSG_SIZE 200

void usage(char* name)
{
    fprintf(stderr, "USAGE: %s n\n", name);
    fprintf(stderr, "3<=n<=20 3<=k<=9\n");
    exit(EXIT_FAILURE);

}

void childwork(int teachertostudent, int studenttoteacher)
{
    printf("%d: child is ready\n",getpid());


    char msg[MSG_SIZE];
    memset(msg,0,MSG_SIZE);

    if(read(teachertostudent,msg,MSG_SIZE)<0){ERR("reading from teacher");}
    
    printf("%s\n",msg);
   // printf("%d: HERE\n",getpid());

    //memset(msg,0,MSG_SIZE);
    snprintf(msg,MSG_SIZE,"%d: HERE",getpid());

    if(write(studenttoteacher,msg,MSG_SIZE)<0){ERR("writing to teacher");}

    if(close(teachertostudent)<0 || close(studenttoteacher)<0){ERR("close unused pipes");}


}

void parentwork(int n, int* teachertostudent, int studenttoteacher,int* studentIDs)
{
    printf("%d: parent is ready\n",getpid());

    //take attandance

    int countofpresentstudent=0;

    char msg[MSG_SIZE];
    memset(msg,0,MSG_SIZE);

    for(int i=0;i<n;i++)
    {
        snprintf(msg,MSG_SIZE,"Is %d here?",studentIDs[i]);
        

        //printf("Is %d here?\n",studentIDs[i]);
     
        if(write(teachertostudent[i],msg,MSG_SIZE)<0){ERR("write to student");}

        if(read(studenttoteacher,msg,MSG_SIZE)<0){ERR("read from student");}

        int studentid = 0;

        sscanf(msg,"%d: HERE",&studentid);
        printf("%s\n",msg);
       
    }

    printf("attandance checked!\n");

   

    for(int i=0;i<n;i++)
    {
        if(close(teachertostudent[i])<0)
        {
            ERR("close unused pipes");
        }
    }

    if(close(studenttoteacher)<0){ERR("close unused pipes");}
}

void createchildren(int n,int* teachertostudent,int studenttoteacher)
{
    int R[2];
    if(pipe(R)<0)
    {
        ERR("pipe R");
    }

    int W[2];

    int* StudentIDs = malloc(sizeof(int)*n);

    for(int i=0;i<n;i++)
    {
        

        if(pipe(W)<0)
        {
            ERR("pipe W");
        }       


        switch(StudentIDs[i] = fork())
        {
            case 0:

            for(int k=0;k<i;k++)
            {
                if(close(teachertostudent[k])<0){ERR("close unused pipes");}
            }

            if(close(W[1])<0){ERR("close unused pipes");}

            if(close(R[0])<0)
            {
                ERR("close unused pipes");
            }

            studenttoteacher = R[1];

            childwork(W[0],studenttoteacher);
            exit(EXIT_SUCCESS);
            break;
            case -1:
                ERR("fork");
        }



        if(close(W[0])<0)
        {
            ERR("close unused pipes");
        }

        teachertostudent[i] = W[1];
    }

    if(close(R[1])<0)
    {
        ERR("close unused pipe");
    }

    parentwork(n,teachertostudent,R[0],StudentIDs);
}


int main(int argc, char** argv)
{
    if(argc!=3)
    {
        usage(argv[0]);
    }

    int n =atoi( argv[1]);
    int k = atoi(argv[2]);

    if(n<3 || n>20 || k<3 || k>9)
    {
        usage(argv[0]);
    }

    int* teachertostudent = malloc(sizeof(int)*n);
    int studenttoteacher;

    createchildren(n,teachertostudent,studenttoteacher);


    while(wait(NULL)>0)
    {

    }

    printf("finishing class..\n");

    return EXIT_SUCCESS;
}


