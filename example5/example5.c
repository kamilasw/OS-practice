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

#define DECK_SIZE 52



void usage(char* name)
{
    fprintf(stderr,"USAGE: %s\n",name);
    fprintf(stderr,"4<=n<=7 m>=4 m*n<=52\n");
    exit(EXIT_FAILURE);
}

void childwork(int parenttochild, int m,int nextplayer,int previousplayer)
{

    printf("child %d: ready!\n",getpid());
    int* cards = malloc(sizeof(int)*m);
    int buffer[m];

    if(read(parenttochild,buffer,m*sizeof(int))<0){ERR("read");}
    
    for(int i=0;i<m;i++)
    {
        cards[i] =buffer[i];
    }


    printf("player %d cards: ",getpid());
    for(int i=0;i<m;i++)
    {
        printf("%d ",cards[i]);
    }
    printf("\n");

    int numofcards = m;

    int suitcount[4] = {0};
    
    while(1)
    {
        int card = cards[0];
        for(int i=0;i<numofcards-1;i++)
        {
            cards[i] = cards[i+1];
        }
        cards =realloc(cards,sizeof(int)*(numofcards-1));
        numofcards--;
        suitcount[card%4]--;

        if(write(nextplayer,&card,sizeof(int))<0){ERR("write");}

        
        if(read(previousplayer,&card,sizeof(int))<0){ERR("read");}
        //printf("player %d: card %d recieved!\n",getpid(),card);


          cards = realloc(cards,sizeof(int)*(numofcards+1));

        cards[numofcards] = card;
        numofcards++;

        suitcount[card%4]++;
        
        if(suitcount[card%4]>=m){
            printf("player %d: my ship sails!!\n",getpid());
        }


    }
    

    free(cards);
    if(close(parenttochild)<0){ERR("close pipes");}
}


void parentwork(int* parenttochild,int n,int m){
    
    printf("parent ready!\n");
    int* carddeck = malloc(sizeof(int)*DECK_SIZE);

    for(int i=0;i<DECK_SIZE;i++)
    {
        carddeck[i] = i;
    }

    for (int i = DECK_SIZE - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = carddeck[i];
        carddeck[i] = carddeck[j];
        carddeck[j] = temp;
    }

    int deckindex=0;
    for(int i=0;i<n;i++)
    {
        int* childcards = malloc(sizeof(int)*m*sizeof(int));
        srand(getpid());
    
        int count=0;
    
        for(int k=0;k<m;k++)
        {
            childcards[k] = carddeck[deckindex];
            deckindex++;
        }

    

        if(write(parenttochild[i],childcards,m*sizeof(int))<0){ERR("write to child");}
    
        free(childcards);
    }
    




    for(int k=0;k<n;k++)
    {
        if(close(parenttochild[k])<0){ERR("close unused pipes");}
    }
    free(carddeck);
}



void createchildren(int n, int m)
{
    
    int* parenttochild = malloc(sizeof(int)*n);

    int pipetoreadforthenextplayer;

    int* pipestocloselater = malloc(sizeof(int)*n);

    int pipeforfirstandlast[2];
    if(pipe(pipeforfirstandlast)<0){ERR("pipe");}

    for(int i=0;i<n;i++)
    {
        int W[2];
        if(pipe(W)<0){ERR("fork");}
     
        int R[2];

        if(pipe(R)<0){ERR("pipe");}
        
        

        switch(fork())
        {
            case 0:

                
 
                for(int k=0;k<i;k++)
                {
                    if(close(parenttochild[k])<0){ERR("close unused pipes");}
                }

                if(close(W[1])<0){ERR("close unused pipes");}

                if(close(R[0])<0){ERR("close");}

                if(i==0)
                {
                    childwork(W[0],m,R[1],pipeforfirstandlast[0]);
                }else if(i==n-1){

                    if(close(R[1])<0){ERR("close");}
                    childwork(W[0],m,pipeforfirstandlast[1],pipetoreadforthenextplayer);
                }
                else
                {
                    childwork(W[0],m,R[1],pipetoreadforthenextplayer);
                }



              

                printf("player %d: finishing..\n",getpid());
                exit(EXIT_SUCCESS);
                break;

            case -1:
                ERR("fork");
        }
        pipetoreadforthenextplayer = R[0];

        pipestocloselater[i] = R[0];

        if(close(W[0])<0)
        {
            ERR("close unused pipes");
        }

        if(close(R[1])<0){ERR("close");}

        parenttochild[i] = W[1];

    }

    for(int i=0;i<n;i++)
    {
        if(close(pipestocloselater[i])<0){ERR("close");}
    }
    parentwork(parenttochild,n,m);


 
    free(parenttochild);

}

int main(int argc, char** argv)
{
    if(argc!=3)
    {
        usage(argv[0]);
    }

    int n = atoi(argv[1]);
    int m = atoi(argv[2]);

    if(n<4 || n>7 || m<4 || m*n>52)
    {
        usage(argv[0]);
    }

    createchildren(n,m);


    while(wait(NULL)>0)
    {

    }

    printf("exiting the game..\n");

    return EXIT_SUCCESS;
}