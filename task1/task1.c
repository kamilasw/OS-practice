/*

    FIFO:
    #include <sys/stat.h>
    int mkfifo(const char *path, mode_t mode);
        mode: 
        Constant	Octal	Description
        S_IRUSR	    0400	Read permission for owner
        S_IWUSR	    0200	Write permission for owner
        S_IXUSR	    0100	Execute/search permission for owner
        S_IRGRP	    0040	Read permission for group
        S_IWGRP	    0020	Write permission for group
        S_IXGRP	    0010	Execute/search permission for group
        S_IROTH	    0004	Read permission for others
        S_IWOTH	    0002	Write permission for others
        S_IXOTH	    0001	Execute/search permission for others


 Upon successful completion, these functions shall return 0.  Other‐
 wise, these functions shall return -1 and set errno to indicate the
 error. If -1 is returned, no FIFO shall be created.        
*/

/*
man 3p isalpha:

 #include <ctype.h>

       int isalpha(int c);
       int isalpha_l(int c, locale_t locale);
test for an alphabetic character

The  isalpha() and isalpha_l() functions shall return non-zero if c
is an alphabetic character; otherwise, they shall return 0.
*/


#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s fifo_file\n", name);
    exit(EXIT_FAILURE);
}

void readfromfifo(int fifo)
{
    ssize_t count;
    char buffer[PIPE_BUF];

    do
    {
        if((count = read(fifo,buffer,PIPE_BUF))<0)
        {
            ERR("read");
        }
        if(count>0 )
        {
            printf("\nPID:%d-------------------------------------\n", *((pid_t *)buffer));
            for (int i = sizeof(pid_t); i < PIPE_BUF; i++)
                if (isalnum(buffer[i]))
                    printf("%c", buffer[i]);
        }
    }while(count>0);

}

int main(int argc,char** argv)
{
    
    if(argc!=2)
    {
        usage(argv[0]);
    }

    int fifo;

    if(mkfifo(argv[1], S_IRUSR|S_IWUSR|S_IWGRP|S_IRGRP)!= 0)
    {
        if (errno != EEXIST)
         ERR("create fifo");
    }

    if((fifo = open(argv[1],O_RDONLY))<0)
    {
        ERR("open");
    }
    
    readfromfifo(fifo);

    if(close(fifo)<0)
    {
        ERR("close");
    }

    if (unlink(argv[1]) < 0)
      ERR("remove fifo:");
    
    return EXIT_SUCCESS;
}