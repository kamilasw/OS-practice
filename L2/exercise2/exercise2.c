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

void usage(char* name)
{
    printf("%s: 2<=N<=20 100<= T1<= T2 <= 5000\n",name);
    exit(EXIT_FAILURE);
}

void parent_work(int n, int t1, int t2, mqd_t serverqueue){

    srand(getpid());
  

    while(1)
    {
        int time = t1 + rand()%t2;

        sleep(time/100);

        float num1 = rand()%10000;
        num1 = num1/100;
        float num2 = rand()%10000;
        num2 = num2/100;

        char msg[64];

        snprintf(msg,sizeof(msg),"new task: %f %f",num1,num2);

        if(mq_send(serverqueue,msg,sizeof(msg),0)<0){ERR("mq send");}


    }






}

void child_work(int t1, int t2,mqd_t serverqueue){

    srand(getpid());

    int time = t1 + rand()%t2;

    printf("[%d] Worker ready!\n",getpid());

    sleep(time/100);

 


    int completedtasks=0;

    while(completedtasks<5)
    {

        char msg[64];

        if(mq_receive(serverqueue,msg,sizeof(msg),0)<0){ERR("mq recieve");}

        float num1,num2;

        sscanf(msg,"new task: %f %f",&num1,&num2);

        printf("%d worker reciever task: %f %f\n",getpid(),num1, num2);
        completedtasks++;
     
        int time = t1 + rand()%t2;
        sleep(time/100);

        printf("%d worker result: %f\n",getpid(),num1+num2);


    }



    printf("[%d] Exiting...\n",getpid());


    

    if(mq_close(serverqueue)){ERR("mq close");}

}

void created_children(int n, int t1, int t2, mqd_t serverqueue){


    for(int i=0;i<n;i++)
    {
        switch(fork())
        {
            case 0:
                //worker

                child_work(t1,t2,serverqueue);

                exit(EXIT_SUCCESS);
                break;
            case -1:
                ERR("fork");
        }
    }

    parent_work(n,t1,t2,serverqueue);


}

int main(int argc, char** argv)
{
    if(argc!=4)
    {
        usage(argv[0]);
    }

    int n = atoi(argv[1]);
    int t1 = atoi(argv[2]);
    int t2 = atoi(argv[3]);

    if(n<2 || n>20 || t1<100 || t1>t2 || t2 > 5000) {usage(argv[0]);}

    

    printf("Server is starting...\n");

    struct mq_attr attr = {};
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 64;


    char qname[30];

    snprintf(qname,sizeof(qname),"/task_queue_%d",getpid());

    mqd_t serverqueue = mq_open(qname,O_CREAT | O_RDWR, 0666,&attr);


    created_children(n,t1,t2,serverqueue);



    while(wait(NULL)>0){}

    printf("All child processes have finished.\n");

    if(mq_close(serverqueue)){ERR("mq close");}

    if(mq_unlink(qname)){ERR("mq unlink");}

    printf("server exiting...\n");


    return EXIT_SUCCESS;
}