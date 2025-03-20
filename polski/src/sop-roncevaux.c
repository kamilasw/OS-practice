#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define MAX_KNIGHT_NAME_LENGTH 20

int aliveknights = 0;


typedef struct{
    char name[MAX_KNIGHT_NAME_LENGTH];
    int hp;
    int attack;
    int enemysize;
    char armyname[MAX_KNIGHT_NAME_LENGTH];

}Knight;

int set_handler(void (*f)(int), int sig)
{
    struct sigaction act = {0};
    act.sa_handler = f;
    if (sigaction(sig, &act, NULL) == -1)
        return -1;
    return 0;
}

void msleep(int millisec)
{
    struct timespec tt;
    tt.tv_sec = millisec / 1000;
    tt.tv_nsec = (millisec % 1000) * 1000000;
    while (nanosleep(&tt, &tt) == -1)
    {
    }
}

int count_descriptors()
{
    int count = 0;
    DIR* dir;
    struct dirent* entry;
    struct stat stats;
    if ((dir = opendir("/proc/self/fd")) == NULL)
        ERR("opendir");
    char path[PATH_MAX];
    getcwd(path, PATH_MAX);
    chdir("/proc/self/fd");
    do
    {
        errno = 0;
        if ((entry = readdir(dir)) != NULL)
        {
            if (lstat(entry->d_name, &stats))
                ERR("lstat");
            if (!S_ISDIR(stats.st_mode))
                count++;
        }
    } while (entry != NULL);
    if (chdir(path))
        ERR("chdir");
    if (closedir(dir))
        ERR("closedir");
    return count - 1;  // one descriptor for open directory
}

Knight* read_knights(FILE *file, const char *army_name){

    int n;
    if (fscanf(file, "%d", &n) != 1) {
        fprintf(stderr, "Error reading number of knights in %s army.\n", army_name);
        exit(EXIT_FAILURE);
    }

    char name[MAX_KNIGHT_NAME_LENGTH];
    int hp, attack;

    Knight* army = malloc(sizeof(Knight)*n);

    for(int i=0;i<n;i++)
    {
        if (fscanf(file, "%s %d %d", name, &hp, &attack) != 3) {
            fprintf(stderr, "Error reading knight details in %s army.\n", army_name);
            exit(EXIT_FAILURE);
        }
        

        Knight k;
        strcpy((k.name),name);
        k.hp = hp;
        k.attack = attack;
        k.enemysize=0;
        strcpy(k.armyname,army_name);
        army[i] = k;

          
    }

    army[0].enemysize = n;

    return army;

}


void knightwork(Knight knight, int* writetoenemies,int readfromenemies){

    printf("I am %s knight %s. I will serve my king with my %d HP and %d attack.\n",knight.armyname, knight.name, knight.hp, knight.attack);

    msleep(100);
    srand(getpid());

    while(1)
    {
        if(knight.hp<=0)
        {
            printf("%s dies!!!\n",knight.name);
            aliveknights--;
            break;
        }

        ssize_t msg_size=0;

        char attack=0;
        int attackvalue =0;

        do{

            msg_size = read(readfromenemies, &attack, sizeof(char));


            if (msg_size > 0) {  // Valid data received
            attackvalue = (int)attack - (int)'0';
            knight.hp -= attackvalue;
            } 
            else if (msg_size == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                // No data available, normal case for non-blocking pipes
                break;
            }
            else if (msg_size == 0) {
                // Pipe closed, knight should exit
                return;
            }
            else {
                ERR("read");  // Only exit on real error
            }
                

        }while(msg_size>0);

        int enemyid = rand()%knight.enemysize;

        attackvalue = rand()%(knight.attack+1);
        attack = (char) (attackvalue + (int)'0');

        while(write(writetoenemies[enemyid],&attack,sizeof(char))==-1)
        {
            if(errno==EPIPE){break;}
            if(errno!=EAGAIN || errno!=EWOULDBLOCK){  ERR("write");}
        }

       if(attackvalue==0){
            printf("%s attacked enemy - he deflected!\n",knight.name);
        }else if(attackvalue <6){
            printf("%s a goes to strike, he hit right and well!\n",knight.name);
        }else{
            printf("%s strikes powerful blow, the shield he breaks and inflicts a big wound!\n",knight.name);
        }

        msleep(1 + rand()%10);

    }


    for(int i=0;i<knight.enemysize;i++)
    {
        if(close(writetoenemies[i])<0){ ERR("close");}
    }
    if(close(readfromenemies)<0){ERR("close");}

}


int main(int argc, char* argv[])
{
    srand(time(NULL));

    FILE *franci_file = fopen("franci.txt", "r");
    if (!franci_file) {
        printf("Franks have not arrived on the battlefield.\n");
        return EXIT_FAILURE;
    }

    FILE *saraceni_file = fopen("saraceni.txt", "r");
    if (!saraceni_file) {
        printf("Saracens have not arrived on the battlefield.\n");
        fclose(franci_file); // Close the first file before exiting
        return EXIT_FAILURE;
    }

    Knight* franci;
    Knight* saraceni;

    // Read and print knight details
    franci = read_knights(franci_file, "Frankish");
    saraceni = read_knights(saraceni_file, "Saracen");
     // Close files
     fclose(franci_file);
     fclose(saraceni_file);

    int francisize = franci[0].enemysize;
    int saracenisize = saraceni[0].enemysize;

    for(int i=0;i<francisize;i++)
    {
        franci[i].enemysize = saracenisize;
    }

    for(int i=0;i<saracenisize;i++)
    {
        saraceni[i].enemysize = francisize;
    }

    
    int* writetosaraceni = malloc(sizeof(int)*saracenisize);
    int* readfromsaraceni = malloc(sizeof(int)*saracenisize);

    int* writetofranci = malloc(sizeof(int)*francisize);
    int* readfromfranci = malloc(sizeof(int)*francisize);

    for(int i=0;i<saracenisize;i++)
    {
        int R[2];
        if(pipe2(R,O_NONBLOCK)<0)
        {
            ERR("pipe");
        }

        writetosaraceni[i] = R[1];
        readfromsaraceni[i] = R[0];

    }

    for(int i=0;i<francisize;i++)
    {
        int W[2];
        if(pipe2(W,O_NONBLOCK)<0)
        {
            ERR("pipe");
        }

        writetofranci[i] = W[1];
        readfromfranci[i] = W[0];

    }

    aliveknights=saracenisize+francisize;


    for(int i=0;i<saracenisize;i++)
    {

        switch(fork())
        {
            case 0:

            for(int k=0;k<saracenisize;k++)
            {
                if(close(writetosaraceni[k])<0){ERR("close");}

                if(i!=k)
                {
                    if(close(readfromsaraceni[k])<0){ERR("close");}
                }
        
            }
        
            for(int k=0;k<francisize;k++)
            {
                if(close(readfromfranci[k])<0){ERR("close");}
        
            }

                knightwork(saraceni[i],writetofranci,readfromsaraceni[i]);
                exit(EXIT_SUCCESS);
                break;
            case -1:
                ERR("fork");
        }


    }

    for(int i=0;i<francisize;i++)
    {

        switch(fork())
        {
            case 0:

            
            for(int k=0;k<francisize;k++)
            {
                if(close(writetofranci[k])<0){ERR("close");}

                if(i!=k)
                {
                    if(close(readfromfranci[k])<0){ERR("close");}
                }
        
            }
        
            for(int k=0;k<francisize;k++)
            {
                if(close(readfromsaraceni[k])<0){ERR("close");}
        
            }




                knightwork(franci[i],writetosaraceni,readfromfranci[i]);
                exit(EXIT_SUCCESS);
                break;
            case -1:
                ERR("fork");
        }


    }






    
    
    for(int i=0;i<saracenisize;i++)
    {
        if(close(writetosaraceni[i])<0 || readfromsaraceni[i]<0){ERR("close");}

    }

    for(int i=0;i<francisize;i++)
    {
        if(close(writetofranci[i])<0 || readfromfranci[i]<0){ERR("close");}

    }


    while(wait(NULL)>0){}

    if(aliveknights>0)
    {
        printf("war is over!!!\n");
    }else
    {
        printf("everyone died!!");
    }

  


     free(writetosaraceni);
     free(writetofranci);
     free(readfromsaraceni);
     free(readfromfranci);
      
    free(saraceni);
    free(franci);

      
      return EXIT_SUCCESS;


}
