#define _GNU_SOURCE
#include <errno.h>
#include <mqueue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

//SERVER PROGRAM

typedef struct{
    int clientid;
    int a;
    int b;
}Message;

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))



void sethandler(void (*f)(int, siginfo_t *, void *), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_sigaction = f;
    act.sa_flags = SA_SIGINFO;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}



void server_work(mqd_t PID_s, mqd_t PID_d, mqd_t PID_m)
{
    printf("server: successfully created all 3 message queues\n");
    sleep(1);


    Message msg;

    if(mq_receive(PID_s,(char*)&msg,sizeof(Message),NULL)<0){ERR("mq recieve");}

     
    char msqname[20];

    snprintf(msqname,sizeof(msqname),"/%d",msg.clientid);

    mqd_t mqclient = mq_open(msqname, O_WRONLY ,0666,NULL);

    if(mqclient==(mqd_t)-1)
    {
        ERR("mq open");
    }

    int result = msg.a + msg.b;

    if(mq_send(mqclient,(char*)&result, sizeof(result),0)<0){ERR("mq send");}

    

}

int main(int argc, char** argv)
{

  

    mqd_t PID_s = mq_open("/pids", O_CREAT | O_RDONLY, 0666,NULL);
    mqd_t PID_d = mq_open("/pidd", O_CREAT | O_RDONLY, 0666,NULL);
    mqd_t PID_m = mq_open("/pidm", O_CREAT | O_RDONLY, 0666,NULL);

    
    if(PID_s==(mqd_t)-1)
    {
        ERR("mq open");
    }
    
    if(PID_d==(mqd_t)-1)
    {
        ERR("mq open");
    }
    
    if(PID_m==(mqd_t)-1)
    {
        ERR("mq open");
    }

    server_work(PID_s,PID_d,PID_m);

    if(mq_close(PID_s))
    {
        ERR("mq close");
    }

    if(mq_close(PID_m))
    {
        ERR("mq close");
    }

    if(mq_close(PID_d))
    {
        ERR("mq close");
    }

    if(mq_unlink("/pids")){ERR("mq unlink");}
    if(mq_unlink("/pidm")){ERR("mq unlink");}
    if(mq_unlink("/pidd")){ERR("mq unlink");}


    printf("server:  finishing...\n");
    return EXIT_SUCCESS;
}

