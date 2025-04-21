#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MSG_SIZE (PIPE_BUF - sizeof(pid_t))
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s fifo_file file\n", name);
    exit(EXIT_FAILURE);
}



void writetofifo(int fifo,int file){

    ssize_t count;
    char buffer[PIPE_BUF];

    do
    {
        count = read(file, buffer + sizeof(pid_t), MSG_SIZE);
        if (count < 0)
            ERR("read");

        if (count < MSG_SIZE)
        {
            memset(buffer + sizeof(pid_t) + count, 0, MSG_SIZE - (size_t)count);
        }

        /* Only send a chunk if we actually read something */
        if (count > 0)
        {
            /* Tag each chunk with the PID */
            *((pid_t *)buffer) = getpid();

       
            // Write exactly PIPE_BUF bytes in one call.
            if (write(fifo, buffer, PIPE_BUF) < 0)
                ERR("write");
        }

    }while(count == MSG_SIZE);


    
}

int main (int argc, char** argv)
{
    if(argc!=3)
    {
        usage(argv[0]);
    }

    int fifo;
    int file;

    
    if(mkfifo(argv[1], S_IRUSR|S_IWUSR|S_IWGRP|S_IRGRP)!= 0)
    {
        if (errno != EEXIST)
         ERR("create fifo");
    }

    if((fifo = open(argv[1],O_WRONLY))<0)
    {
        ERR("open");
    }
    if((file = open(argv[2],O_RDONLY))<0)
    {
        ERR("file open");
    }

    writetofifo(fifo,file);

    if(close(fifo)<0)
    {
        ERR("close");
    }
    if(close(file)<0)
    {
        ERR("file close");
    }



    return EXIT_SUCCESS;
}