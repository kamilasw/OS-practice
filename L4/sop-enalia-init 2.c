#include "common.h"

#define MAX_CLIENTS 4
#define MAX_USERNAME_LENGTH 32
#define MAX_MESSAGE_SIZE 64

typedef struct 
{
    char username[MAX_USERNAME_LENGTH];
    int user_fd;
    int line;
}user_info;


void usage(char* program_name)
{
    fprintf(stderr, "Usage: \n");

    fprintf(stderr, "\t%s", program_name);
    set_color(2, SOP_PINK);
    fprintf(stderr, " port\n");

    fprintf(stderr, "\t  port");
    reset_color(2);
    fprintf(stderr, " - the port on which the server will run\n");

    exit(EXIT_FAILURE);
}

int sethandler(void (*f)(int), int sig)
{
    struct sigaction act;
    memset(&act, 0 , sizeof(struct sigaction));
    act.sa_handler = f;
    if(sigaction(sig, &act, NULL)<0)
        return -1;
    return 0;
}

void decline_user(int client_fd)
{
    if(bulk_write(client_fd, "Server is full\n", 16)<0)
    {
        if(errno != EPIPE)
            ERR("write");
    }
    if(TEMP_FAILURE_RETRY(close(client_fd)) < 0)
        ERR("close");
}

void add_user(int client_fd, user_info info[MAX_CLIENTS], int* counter, int epoll_fd)
{
    user_info new_user;
    new_user.user_fd = client_fd;
    new_user.line = 0;
    memset(new_user.username,0,MAX_USERNAME_LENGTH);
    
    info[(*counter)] = new_user;
    (*counter)++;

    int new_flags = fcntl(client_fd, F_GETFL) | O_NONBLOCK;
    fcntl(client_fd, F_SETFL, new_flags);

    printf("[%d] connected\n",client_fd);

    char* message = "Please enter your username\n";
    if(bulk_write(client_fd, message, strlen(message))<0)
    {
        if(errno != EPIPE)
            ERR("bulk_write");
    }

    message = "User logging in...\n";
    int size = strlen(message);
    for(int i = 0; i<(*counter)-1;i++)
    {
        if(bulk_write(info[i].user_fd,message,size)<0)
        {
            if(errno!=EPIPE)
                ERR("bulk_write");
        }
    }

    struct epoll_event client_event;
    client_event.events = EPOLLIN | EPOLLRDHUP;;
    client_event.data.fd = client_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event) == -1)
    {
        ERR("epoll_ctl");
    }
}

void read_input(user_info infos[MAX_CLIENTS], int num, user_info* client_info)
{
    int received = 0;
    char c;
    int size;

    char buf[MAX_MESSAGE_SIZE];
    int len = 0;
    while(1)
    {
        size = bulk_read(client_info->user_fd, &c, 1);
        if(size<0)
            ERR("bulk_read");
        if(size == 0)
            break;

        buf[len] = c;
        len++;
        received = 1;
        if(c == '\n')
        {
            if(client_info->line == 0)
            {
                buf[len-1] = '\0';
                strcpy(client_info->username,buf);

                if(num == 1)
                {
                    char* message = "You're the first one here!\n";
                
                    if(bulk_write(client_info->user_fd,message,strlen(message))<0)
                    {
                        if(errno != EPIPE)
                            ERR("bulk_write");
                    }  
                }
                else
                {
                    char message[MAX_MESSAGE_SIZE] = "Current users:\n";
                    if(bulk_write(client_info->user_fd,message,strlen(message))<0)
                    {
                        if(errno != EPIPE)
                            ERR("bulk_write");
                    }  
                    for(int i = 0; i<num;i++)
                    {
                        if(infos[i].username[0] == 0)
                        {
                            memset(message,0,MAX_MESSAGE_SIZE);
                            snprintf(message,MAX_MESSAGE_SIZE,"[%d]\n", infos[i].user_fd);
                            if(bulk_write(client_info->user_fd,message,strlen(message))<0)
                            {
                                if(errno != EPIPE)
                                    ERR("bulk_write");
                            } 
                        }
                        else
                        {
                            memset(message,0,MAX_MESSAGE_SIZE);
                            snprintf(message,MAX_MESSAGE_SIZE,"[%d] %s\n", infos[i].user_fd,infos[i].username);
                            if(bulk_write(client_info->user_fd,message,strlen(message))<0)
                            {
                                if(errno != EPIPE)
                                    ERR("bulk_write");
                            } 
                        }
                    }
                }
                printf("[%d] logged in as [%s]\n",client_info->user_fd, client_info->username);
                for(int j = 0; j<num-1;j++)
                {
                    char message[MAX_MESSAGE_SIZE];
                    set_color(infos[j].user_fd, SOP_GREEN);
                    snprintf(message, MAX_MESSAGE_SIZE,"User [%s] logged in\n",client_info->username);
                    if(bulk_write(infos[j].user_fd, message,strlen(message))<0)
                    {
                        if(errno != EPIPE)
                            ERR("bulk_write");
                    }
                    reset_color(infos[j].user_fd);
                }
            }
            client_info->line++;
            break;
        }
        printf("%c", c);
    }
    printf("\n");
    // char* message = "\nHello World!\n";
    // if(received)
    // {
    //     if(bulk_write(client_info->user_fd,message,strlen(message))<0)
    //     {
    //         if(errno != EPIPE)
    //             ERR("bulk_write");
    //     }
    // }
}

int main(int argc, char** argv) 
{ 
    if(argc!=2)
        usage(argv[0]); 

    if(sethandler(SIG_IGN,SIGPIPE)<0)
        ERR("sethandler");  

    int16_t port = atoi(argv[1]);
    int listen_fd = bind_tcp_socket(port, MAX_CLIENTS);

    int new_flags = fcntl(listen_fd, F_GETFL) | O_NONBLOCK;
    fcntl(listen_fd, F_SETFL, new_flags);

    int epoll_fd;
    if ((epoll_fd = epoll_create1(0)) < 0)
    {
        ERR("epoll_create1");
    }
    struct epoll_event event, events[MAX_CLIENTS+1];
    event.events = EPOLLIN | EPOLLRDHUP;
    event.data.fd = listen_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event) == -1)
    {
        ERR("epoll_ctl");
    }

    user_info info[MAX_CLIENTS];
    int counter = 0;

    while(1)
    {
        int ready_fds = epoll_wait(epoll_fd,events, MAX_CLIENTS+1,-1);
        for(int i = 0; i<ready_fds;i++)
        {
            if(events[i].data.fd == listen_fd)
            {
                int client_fd = add_new_client(listen_fd);
                if(counter>=MAX_CLIENTS)
                {
                    decline_user(client_fd);
                }
                else
                {
                    add_user(client_fd, info, &counter, epoll_fd);
                }
            }
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP)) 
            {
                if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL) < 0)
                        ERR("epoll_ctl DEL");
                close(events[i].data.fd);
                for (int j = 0; j < counter; j++) 
                {
                    if (info[j].user_fd == events[i].data.fd) 
                    {
                        for(int a = j; a<counter-1;a++)
                            info[a] = info[a+1];
                    }
                }
                counter--;
                printf("Client %d disconnected\n", events[i].data.fd);
            }
            else 
            {
                for(int j = 0; j<counter;j++) {
                    if(events[i].data.fd == info[j].user_fd) {
                        read_input(info,counter,&info[j]);
                    }
                }
            }
        }
    }

    if(TEMP_FAILURE_RETRY(close(epoll_fd))<0)
        ERR("close");
    if(TEMP_FAILURE_RETRY(close(listen_fd))<0)
        ERR("close");

    return EXIT_SUCCESS;
} 
