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

#define MAX_CLIENTS 8

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
    printf("%s server_name MAX_CHAR 20\n",name);
    exit(EXIT_FAILURE);
}


void server_work(mqd_t serverqueue)
{
    int connectedclients = 0;

    char clientsnames[MAX_CLIENTS][MAX_NAME];

    mqd_t clientqueues[MAX_CLIENTS];

    char clientsmqname[MAX_CLIENTS][MAX_MQNAME];

    while(connectedclients<MAX_CLIENTS)
    {
        char msg[MAX_MSGSIZE];

        int priority;

        if(TEMP_FAILURE_RETRY(mq_receive(serverqueue,msg,MAX_MSGSIZE,&priority))<0){ERR("mq recieve");}

        if(priority==0 && connectedclients<MAX_CLIENTS)
        {
            printf("server: [%d] {%s}\n",priority,msg);


            snprintf(clientsmqname[connectedclients],sizeof(clientsmqname[connectedclients]),"/chat_%s",msg);

            clientqueues[connectedclients] = TEMP_FAILURE_RETRY(mq_open(clientsmqname[connectedclients],O_RDWR));

            if(clientqueues[connectedclients]==(mqd_t)-1){ERR("mq open");}

            strcpy(clientsnames[connectedclients],msg);
            connectedclients++;

            printf("server: opened the chat with %s\n",msg);
        }

      
    }

    for(int i=0;i<connectedclients;i++)
    {
        if(mq_close(clientqueues[i])){ERR("mq close");}
        if(mq_unlink(clientsmqname[i])){ERR("mq unlink");}
    }




}

int main(int argc, char** argv)
{

    if(argc!=2)
    {
        usage(argv[0]);
    }

    char servername[MAX_NAME];

    strcpy(servername,argv[1]);

    char mqname[MAX_MQNAME];

    snprintf(mqname,sizeof(mqname),"/chat_%s",servername);

    struct mq_attr attr = {};
    attr.mq_maxmsg = MAX_MSGS;
    attr.mq_msgsize = MAX_MSGSIZE;

    mqd_t serverqueue = TEMP_FAILURE_RETRY(mq_open(mqname,O_CREAT | O_RDONLY,0666,&attr));

    if(serverqueue == (mqd_t)-1)
    {
        ERR("mq open");
    }

    server_work(serverqueue);


    if(mq_close(serverqueue)){ERR("mq close");}

    if(mq_unlink(mqname)){ERR("mq unlink");}


    printf("server closing...\n");
    return EXIT_SUCCESS;

}