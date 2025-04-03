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

#define LIFE_SPAN 10
#define MAX_NUM 10

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


void sigchld_handler(int sig, siginfo_t *s, void *p)
{
    pid_t pid;
    for (;;)
    {
        pid = waitpid(0, NULL, WNOHANG);
        if (pid == 0)
            return;
        if (pid <= 0)
        {
            if (errno == ECHILD)
                return;
            ERR("waitpid");
        }
        children_left--;
    }
}

void mq_handler(int sig, siginfo_t *info, void *p)
{
    mqd_t *pin;

    int num;
    unsigned msg_prio;


    pin =  (mqd_t *)info->si_value.sival_ptr;

    static struct sigevent notif;
    notif.sigev_notify = SIGEV_SIGNAL;
    notif.sigev_signo = SIGRTMIN;
    notif.sigev_value.sival_ptr = pin;

    if (mq_notify(*pin, &notif) < 0)
    {
        ERR("mq_notify");
    }
      
    while(1)
    {
        if (mq_receive(*pin, (char *)&num, 1, &msg_prio) < 1)
        {
            if (errno == EAGAIN)
                break;
            else
                ERR("mq_receive");
        }

        if (0 == msg_prio)
        {
            printf("MQ: got timeout from %d.\n", num);

        }
        else
        {
            printf("MQ: %d is a bingo number!\n", num);
        }       
       

    }



}

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s n k p l\n", name);
    fprintf(stderr, "100 > n > 0 - number of children\n");
    exit(EXIT_FAILURE);
}

void child_work(int n, mqd_t pin, mqd_t pout ){

    srand(getpid());

    int E = rand()%MAX_NUM;
    int N = rand()%MAX_NUM;

    int num=0;

    for(int life = rand()%LIFE_SPAN + 1; life > 0;life--)
    {
        if(mq_receive(pout,(char*)&num,sizeof(char),NULL)<1)
        {
            ERR("mq recieve");
        }

        printf("player %d: recieved nr. %d\n",getpid(),num);

        if(E == num)
        {
            if(mq_send(pin,(const char*)&E,sizeof(char),1))
            {
                ERR("mq send");
            }
            return;
        }
    }

    if(mq_send(pin,(const char*)&num,sizeof(char),0))
    {
        ERR("mq send");
    }

}

void parent_work(int n, mqd_t pin, mqd_t pout){

    srand(getpid());

    while(children_left)
    {
        int num = rand()%MAX_NUM;

        if(mq_send(pout,(const char*)&num,1,0))
        {
            ERR("mq send");
        }
        sleep(1);
    }

    printf("parent stopping work..\n");


}

int main(int argc, char **argv)
{
    if(argc!=2)
    {
        usage(argv[0]);
    }

    mq_unlink("/pout");
    mq_unlink("/pin");

    mqd_t pin;
    mqd_t pout;

    struct mq_attr attr = {
        .mq_flags = 0,
        .mq_maxmsg = 10,
        .mq_msgsize = 1
    };


    pout = mq_open("/pout",O_CREAT | O_RDWR, 0666,&attr);
    if(pout == (mqd_t)-1){
        ERR("mq_open");
    }

    pin = mq_open("/pin",O_CREAT | O_RDWR | O_NONBLOCK, 0666, &attr);
    if(pin == (mqd_t)-1){
        ERR("mq_open");
    }

    sethandler(sigchld_handler, SIGCHLD);
    sethandler(mq_handler, SIGRTMIN);

    static struct sigevent notif;
    notif.sigev_notify = SIGEV_SIGNAL;
    notif.sigev_signo = SIGRTMIN;
    notif.sigev_value.sival_ptr = &pin;

    if(mq_notify(pin,&notif)<0)
    {
        ERR("mq notify");
    }


    int n = atoi(argv[1]);

    children_left = n;  
  

    for(int i=0;i<n;i++)
    {
        
        switch(fork())
        {
            case 0:
                //child
                child_work(n,pin,pout);
                exit(EXIT_SUCCESS);
            case -1:
                ERR("fork");
        }

    }

    parent_work(n,pin,pout);


    while(wait(NULL)>0){}

    mq_close(pout);
    mq_close(pin);

    if (mq_unlink("/pin"))
    {
        ERR("mq unlink");
    }

    if (mq_unlink("/pout"))
    {
        ERR("mq unlink");

    }

    printf("bingo finished!\n");
    return EXIT_SUCCESS;
 
}
