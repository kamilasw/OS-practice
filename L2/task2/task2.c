#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define CHILD_COUNT 4

#define QUEUE_NAME_MAX_LEN 32
#define CHILD_NAME_MAX_LEN 32

#define MSG_SIZE 64
#define MAX_MSG_COUNT 4

#define ROUNDS 5

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))




int main(int argc, char** argv)
{

    mqd_t queues[CHILD_COUNT];
    char names[CHILD_COUNT][CHILD_NAME_MAX_LEN];
    char queue_names[CHILD_COUNT][CHILD_NAME_MAX_LEN];

    for(int i=0;i<CHILD_COUNT;i++)
    {
        snprintf(queue_name)
    }



    return EXIT_SUCCESS;
}