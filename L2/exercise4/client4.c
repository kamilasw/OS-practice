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

#define MAX_NAME 20

#define MAX_MQNAME 30

#define MAX_MSGSIZE 20

#define MAX_MSGS 10


#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t children_left = 0;

void sethandler(void (*f)(int, siginfo_t *, void *), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_sigaction = f;
    act.sa_flags = SA_SIGINFO;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void usage(char* name)
{
    printf("%s server_name client_name MAX_CHAR 20\n",name);
    exit(EXIT_FAILURE);
}

void client_work(mqd_t serverqueue, char* clientname,mqd_t clientqueue)
{

    char msg[MAX_MSGSIZE];

    snprintf(msg,MAX_MSGSIZE,"%s",clientname);

    if(mq_send(serverqueue,msg,sizeof(msg),0)<0){ERR("mq send");}

    printf("Client %s has connected!\n",clientname);


}



int main(int argc, char** argv)
{
    if(argc!=3)
    {
        usage(argv[0]);
    }

    char servername[MAX_NAME];
    char clientname[MAX_NAME];

    strcpy(servername,argv[1]);
    strcpy(clientname,argv[2]);

    char mqname[MAX_MQNAME];

    snprintf(mqname,sizeof(mqname),"/chat_%s",servername);

    mqd_t serverqueue = TEMP_FAILURE_RETRY(mq_open(mqname,O_CREAT | O_WRONLY,NULL,NULL));

    if(serverqueue == (mqd_t)-1)
    {
        ERR("mq open");
    }

    struct mq_attr attr = {};
    attr.mq_maxmsg = MAX_MSGS;
    attr.mq_msgsize = MAX_MSGSIZE;

    char mqnameclient[MAX_MQNAME];

    snprintf(mqnameclient,sizeof(mqnameclient),"/chat_%s",clientname);

    mqd_t clientqueue = TEMP_FAILURE_RETRY(mq_open(mqnameclient,O_CREAT | O_RDWR ,0666,&attr));

    if(clientqueue == (mqd_t)-1)
    {
        ERR("mq open");
    }


    client_work(serverqueue,clientname,clientqueue);


    if(mq_close(serverqueue)){ERR("mq close");}

    if(mq_close(clientqueue)){ERR("mq close");}

    printf("client %d closing...\n",getpid());

    return EXIT_SUCCESS;
}