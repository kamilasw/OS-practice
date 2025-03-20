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

    
// MAX_BUFF must be in one byte range
#define MAX_BUFF 200

volatile sig_atomic_t last_signal = 0;


void usage(char *name)
{
    fprintf(stderr, "USAGE: %s n\n", name);
    fprintf(stderr, "0<n<=10 - number of children\n");
    exit(EXIT_FAILURE);

}

int sethandler(void (*f)(int), int sigNo) {
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction)); // Initialize act structure to zero
    act.sa_handler = f; // Set the signal handler function
    if (-1 == sigaction(sigNo, &act, NULL)) // Apply the handler using sigaction
        return -1; // Return error if sigaction fails
    return 0; // Success
}

// SIGCHLD handler to reap child processes and prevent zombies
void sigchld_handler(int sig) {
    pid_t pid;
    for (;;) {
        pid = waitpid(0, NULL, WNOHANG); // Non-blocking wait for any child process
        if (0 == pid) return; // No more child processes to reap
        if (0 >= pid) { // Error handling for waitpid
            if (ECHILD == errno) return; // No more children, return safely
            ERR("waitpid:"); // Print error and terminate if unexpected error occurs
        }
    }
}

void sig_handler(int sig) { last_signal = sig; }

void sig_killme(int sig)
{
    if (rand() % 5 == 0)
        exit(EXIT_SUCCESS);
}


void childwork(int fd, int pipeR)
{
    srand(getpid());
    
    char c, buf[MAX_BUFF + 1];
    unsigned char s;

    if (sethandler(sig_killme, SIGINT))
     ERR("Setting SIGINT handler in child");


    while(1)
    {
        if(TEMP_FAILURE_RETRY(read(fd,&c,1))<1)
        {
            ERR("read");
        }
        
        s = 1 +rand()%MAX_BUFF;
        buf[0] = s;
        memset(buf+1,c,s);
        if(TEMP_FAILURE_RETRY(write(pipeR,buf,s+1))<0)
        {
            ERR("write to main pipe");
        }
    }

  
   
}

void createchildandpipes(int n, int*fds,int pipeR){

    int fd[2];
    int max = n;

    while(n)
    {
        if(pipe(fd))
        {
            ERR("pipe");
        }

        switch(fork())
        {
            case 0://child
            
            while(n<max)
            {
                if(fds[n]&&close(fds[n++]))
                {
                    ERR("close");
                }
            }

            free(fds);

            if(close(fd[1]))
            {
                ERR("close");
            }

            childwork(fd[0],pipeR);

            if(close(fd[0]))
            {
                ERR("close");
            }

            if(close(pipeR))
            {
                ERR("close");
            }

            exit(EXIT_SUCCESS);
            case -1:
                ERR("fork");
        }

        if(close(fd[0]))
        {
            ERR("close");
        }

        fds[--n] = fd[1];


        
    }


}



void parentwork(int n, int *fds, int R)
{
    unsigned char c; 
    char buf[MAX_BUFF];
    int status, i;
    srand(getpid());
  
    if (sethandler(sig_handler, SIGINT)) // Set SIGINT handler for parent
    ERR("Setting SIGINT handler in parent");

    while(1)
    {
        if (SIGINT == last_signal) { // If SIGINT was received
            i = rand() % n; // Pick a random child

            while (0 == fds[i % n] && i < 2 * n) // Skip closed pipes
                i++;

            i %= n;
            if (fds[i]) { // If the selected pipe is still open
                c = 'a' + rand() % ('z' - 'a'); // Generate a random lowercase letter
                status = TEMP_FAILURE_RETRY(write(fds[i], &c, 1)); // Send the letter to the child
            }
                if (status != 1) { // If the write fails, close the pipe
                    if (TEMP_FAILURE_RETRY(close(fds[i])))
                        ERR("close");
                    fds[i] = 0; // Mark as closed
                }
            

        }
        last_signal=0;
    

    status = read(R, &c, 1); // Read buffer size
    if (status < 0 && errno == EINTR) // Handle interrupted read
        continue;

    if (status < 0)
        ERR("read header from R");

    if (0 == status) // No more data, exit loop
        break;
    
    if (TEMP_FAILURE_RETRY(read(R, buf, c)) < c) // Read full buffer
        ERR("read data from R");

    buf[(int)c] = 0; // Null-terminate the string
    printf("\n%s\n", buf); // Print received message
    }
}

int main(int argc, char** argv)
{
    if(argc!=2)
    {
        usage(argv[0]);
    }

    int n = atoi(argv[1]);
    if(n<=0 || n>10)
    {
        usage(argv[0]);
    }

    int pipeR[2]; //for child to parent 

    if(pipe(pipeR)==-1)
    {
        ERR("pipeR pipe");
    }

    int *fds; //file descriptors
    fds = (int*) malloc(sizeof(int)*n);
    if(fds==NULL)
    {
        ERR("malloc");
    }

    if(sethandler(sigchld_handler,SIGCHLD)){
        ERR("setting SIGCHLD");
    }

    //create children and pipes

    createchildandpipes(n,fds,pipeR[1]);

    if(close(pipeR[1]))
    {
        ERR("close");
    }

    parentwork(n,fds,pipeR[0]);

    while(n--)
    {
        if(fds[n]&&close(fds[n]))
        {
            ERR("close");
        }
    }

    if(close(pipeR[0]))
    {
        ERR("close");
    }

    free(fds);


    return EXIT_SUCCESS;
}