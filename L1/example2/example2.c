#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MSG_SIZE 16

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))


    
void usage(char *name)
{
    fprintf(stderr, "USAGE: %s n\n", name);
    fprintf(stderr, "2<=n<=5 and 5<=M<=10\n");
    exit(EXIT_FAILURE);
}


void childwork(int readpipe, int writepipe, int m,int index)
{

    int* cards;
    cards = malloc(sizeof(int)*m);

    for(int i=0;i<m;i++)
    {
        cards[i] = i;
    }

    srand(getpid());
    printf("%d: player is ready!\n",getpid());


    while(m>0)
    {
        int randomcard = 0 + rand()%m;

        int chosencard=cards[randomcard];
        cards[randomcard] = cards[m-1];
        m--;

        char msg[MSG_SIZE];
        memset(msg,0,MSG_SIZE);

        if(read(readpipe,msg,MSG_SIZE)<0){ERR("readpipe");}
    
        memset(msg,0,MSG_SIZE);
        snprintf(msg,16,"%d %d",index,chosencard);

        if(write(writepipe,msg,MSG_SIZE)<0){ERR("writepipe");}



    }

    printf("player %d: no more cards left\n",index);


   free(cards);

}

void parentwork(int* writepipe,int* readpipe,int m,int n)
{


    int childid;
    int recievedcard;
    int countofrecievedcards=0;

    char msg[MSG_SIZE] = {0};


    srand(getpid());


    //lets make it only 4 rounds for now

    for(int k=0;k<7;k++){

        sleep(rand()%10);

        for(int i=0;i<n;i++)
        {
            memset(msg,0,MSG_SIZE);
            snprintf(msg,MSG_SIZE,"new_round");

            if(write(writepipe[i],msg,MSG_SIZE)<0)
            {
                if(errno==SIGPIPE)
                {
                    printf("player %d hasnt entered the round\n",i);
                    continue;
                }
                ERR("writepipe");
            }

        }

        printf("server: collecting the cards\n");


        for(int i=0;i<n;i++)
        {
            if(read(readpipe[i],msg,MSG_SIZE)<0)
            {
                if(errno==SIGPIPE)
                {
                    printf("player %d hasnt sent a card\n",i);
                    continue;
                }
                ERR("readpipe");
            }
            
            countofrecievedcards++;
    
            sscanf(msg,"%d %d",&childid,&recievedcard);
    
            printf("Got number %d from player %d\n",recievedcard,childid);
    
        }

        if(countofrecievedcards==n*m)
        {
            printf("all cards have been recieved\n");
            break;
        }

    }

   

  

    for(int i=0;i<n;i++)
    {
        if(close(writepipe[i])<0 || close(readpipe[i])<0)
        {
            ERR("closing usused pipe");
        }
    }

    

}


void createprocesses(int n, int m, int* parenttochild ,int* childtoparent){

    int R[2]; int W[2];
    for(int i=0;i<n;i++)
    {
        if(pipe(R)<0)
        {
            ERR("pipe");
        }

        if(pipe(W)<0){
            ERR("pipe");
        }

        switch(fork())
        {
            case 0:

                for(int k=0;k<i;k++)
                {
                    if(close(parenttochild[k])<0 || close(childtoparent[k])<0){ERR("close unused pipes");}
                }

                if(close(R[0])<0 || close(W[1])<0)
                {
                    ERR("close unsued pipes");
                }

                parenttochild[i] = W[0];
                childtoparent[i] = R[1];

                childwork(parenttochild[i],childtoparent[i],m,i);

                if(close(R[1])<0 || close(W[0])<0)
                {
                    ERR("close unsued pipes");
                }

                exit(EXIT_SUCCESS);
                break;
            case -1:
                ERR("fork");

        }
        
        if(close(R[1])<0 || close(W[0])<0){ERR("close unused pipes");}

        parenttochild[i] = W[1];
        childtoparent[i] = R[0];

    }

    printf("%d: server is ready!\n",getpid());

    parentwork(parenttochild,childtoparent,m,n);


}

int main(int argc, char** argv)
{

    if(argc!=3)
    {
        usage(argv[0]);
    }

    int n = atoi(argv[1]); //number of child processes - players
    int m = atoi(argv[2]); // number of cards of each player

    if(n<2||n>5||m<5||m>10){
        usage(argv[0]);
    }

    int* parenttochild = malloc(sizeof(int)*n);
    int* childtoparent = malloc(sizeof(int)*n);

    createprocesses(n,m,parenttochild,childtoparent);


 
    while(wait(NULL)>0){}

    free(childtoparent);
    free(parenttochild);

    printf("game finished.. closing the program\n");

    
    return EXIT_SUCCESS;
}

