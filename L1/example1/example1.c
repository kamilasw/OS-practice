#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define MAX_BUFF 200
volatile sig_atomic_t keep_running = 1;

void sigint_handler(int sig) {
    keep_running = 0;
}


int randomnum()
{
    srand(getpid());

    return rand()%100;
}

typedef struct{
    int number;
    char token[2];
}Message;

void sendnumber(int fd, const char* token, int number)
{
    Message msg;
    msg.number = number;
    strcpy(msg.token,token);   

    if(write(fd,&msg,sizeof(msg))!=sizeof(msg))
    {
        ERR("write msg");
    }
}

Message recievenumber(int fd)
{
    Message msg;
    ssize_t bytesRead = read(fd, &msg, sizeof(msg));
    
    if (bytesRead == 0) {
        // Pipe is closed (other process exited), terminate gracefully
        printf("PID %d: Pipe closed. Exiting...\n", getpid());
        keep_running = 0;
        msg.number = 0;  // Ensure we signal termination
    } else if (bytesRead != sizeof(msg)) {
        ERR("read msg");
    }
    printf("PID %d: %d\n",(int)getpid(),msg.number);
    return msg;
}

void firstchild(int P1,int P2)
{
   // printf("%d: p1%d p2%d\n",(int)getpid(),P1,P2);

   Message msg;
  
   while(keep_running)
   {
       msg = recievenumber(P1);
       if (msg.number == 0) {  // Termination condition
        printf("PID %d: Received 0. Terminating...\n", getpid());
        break;
    }
       msg.number += randomnum();
       sendnumber(P2,msg.token,msg.number);
   }

    if(close(P1)<0){ERR("close P1[0]");}
    if(close(P2)<0){ERR("close P2[1]");}
}

void secondchild(int P2,int P3)
{
    //printf("%d: p2%d p3%d\n",(int)getpid(),P2,P3);
    Message msg;
   
    while(keep_running)
    {
        msg = recievenumber(P2);
        if (msg.number == 0) {  // Termination condition
            printf("PID %d: Received 0. Terminating...\n", getpid());
            break;
        }
        msg.number += randomnum();
        sendnumber(P3,msg.token,msg.number);
    }

    if(close(P2)<0){ERR("close P2[0]");}
    if(close(P3)<0){ERR("close P3[1]");}
}

void createchildren(int P1[],int P2[],int P3[],int n)
{

    for(int i=0;i<2;i++)
    {
        switch(fork())
        {
            case 0:
                if(i==0)
                {
                    if(close(P1[1])<0){ERR("close P1[1]");}
                    if(close(P2[0])<0){ERR("close P2[0]");}
                    if(close(P3[0])<0){ERR("close P3[0]");}
                    if(close(P3[1])<0){ERR("close P3[1]");}
                    firstchild(P1[0],P2[1]);
                }
                else
                {
                    if(close(P1[0])<0){ERR("close P1[0]");}
                    if(close(P1[1])<0){ERR("close P1[1]");}
                    if(close(P2[1])<0){ERR("close P2[1]");}
                    if(close(P3[0])<0){ERR("close P3[0]");}
                    secondchild(P2[0],P3[1]);
                }
                exit(EXIT_SUCCESS);
                break;
            case -1:
                ERR("fork");
        }
    }

    if(close(P1[0])<0){ERR("close P1[0]");}
    if(close(P2[0])<0){ERR("close P2[0]");}
    if(close(P2[1])<0){ERR("close P2[1]");}
    if(close(P3[1])<0){ERR("close P3[1]");}

}



int main(int argc, char** argv)
{
    if(argc!=1)
    {
        fprintf(stderr,"no ags requested!\n");
    }

    // Register SIGINT handler
    signal(SIGINT, sigint_handler);


    int P1[2]; //pipe: parent -> first child
    int P2[2]; //pipe: first child -> second child
    int P3[2]; //pipe: second child -> parent
    
    if(pipe(P1)<0)
    {
        ERR("pipe P1");
    }

    if(pipe(P2)<0)
    {
        ERR("pipe P2");
    }

    if(pipe(P3)<0)
    {
        ERR("pipe P3");
    }

    createchildren(P1,P2,P3,2);

   // printf("%d: p1%d p3%d\n",getpid(),P1[1],P3[0]);

    Message msg;

    sendnumber(P1[1],"1",1);

    while(keep_running)
    {
        msg = recievenumber(P3[0]);
        if(!keep_running){break;}
        msg.number += randomnum();
        sendnumber(P1[1],msg.token,msg.number);
    }

     // Send termination signal (0) before exiting
    sendnumber(P1[1], "1", 0);
    printf("Parent: Sent termination signal. Exiting...\n");

    if(close(P1[1])<0){ERR("close P1[1]");}
    if(close(P3[0])<0){ERR("close P3[0]");}

    while(wait(NULL)>0){}


    return EXIT_SUCCESS;
   


}


