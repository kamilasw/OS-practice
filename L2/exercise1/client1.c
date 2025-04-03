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

//CLIENT PROGRAM

typedef struct{
    int clientid;
}message;

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

void client_work(mqd_t clientqueue)
{
    printf("client: queue created\n");
    sleep(1);

}

int main(int argc, char** argv)
{
    
    char msqname[20];

    snprintf(msqname,sizeof(msqname),"/%d",getpid());


    mqd_t mqclient = mq_open(msqname,O_CREAT | O_RDONLY,0666,NULL);

    if(mqclient==(mqd_t)-1)
    {
        ERR("mq open");
    }

    client_work(mqclient);



    if(mq_close(mqclient)){ERR("mq close");}

    if(mq_unlink(msqname)){ERR("mq unlink");}


    printf("client: terminating...\n");
    return EXIT_SUCCESS;
}