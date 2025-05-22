
// Server sends back the sum of digits in the PID as int16_t.

// Server accepts clients until C-c.

//On SIGINT it exits and prints the highest sum it calculated.


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>

#define BUF_SIZE 64

volatile sig_atomic_t keep_running = 1;  // Controls main loop
int max_sum = 0;                         // Tracks highest sum seen

// Helper: calculate sum of digits from a PID string
int sum_of_digits(const char *str) {
    int sum = 0;
    for (int i = 0; str[i]; i++) {
        if (str[i] >= '0' && str[i] <= '9')
            sum += str[i] - '0';  // Convert char to digit
    }
    return sum;
}

// Signal handler for Ctrl+C
void handle_sigint(int sig) {
    (void)sig;
    keep_running = 0; // Triggers exit from server loop
}


int main(int argc, char** argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    //convert port argument to integer
    uint16_t port = atoi(argv[1]);

    // Setup SIGINT handler
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = handle_sigint;
    sigaction(SIGINT, &act, NULL);

    
    //create TCP socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    //setup server address structure
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,            // IPv4
        .sin_port = htons(port),          // port (converted to network byte order)
        .sin_addr = { htonl(INADDR_ANY) } // bind to all available interfaces
    };


    //bind the socket to the specified port
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_fd);
        return EXIT_FAILURE;
    }

    //start listening for incoming connections
    if (listen(server_fd, 1) < 0) { //only need 1 connection for stage 1
        perror("listen");
        close(server_fd);
        return EXIT_FAILURE;
    }
    printf("Server listening on port %d...\n", port);

    //when accepting one client:
    /*
   
    //accept an incoming connection (blocking)
    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("accept");
        close(server_fd);
        return EXIT_FAILURE;
    }

    //buffer to store the received PID text
    char buf[BUF_SIZE] = {0};

    //read data sent by client into the buffer
    ssize_t received = read(client_fd, buf, sizeof(buf) - 1); // leave space for '\0'
    
    if (received < 0) {
        perror("read");
    } else {
        buf[received] = '\0';         // Null-terminate the string
        printf("Received: %s\n", buf); // Print the client's PID
    }


    //close both client and server sockets
    close(client_fd);

    */


    //for accepting multiple clients

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
             if (errno == EINTR) break; // Interrupted by signal
            perror("accept");
            continue;
        }

        char buf[BUF_SIZE] = {0};
        ssize_t received = read(client_fd, buf, sizeof(buf) - 1);
        if (received <= 0) {
            perror("read");
            close(client_fd);
            continue;
        }

        buf[received] = '\0';  // Null-terminate input
        printf("Received PID: %s\n", buf);

        int sum = sum_of_digits(buf);
        int16_t sum_to_send = (int16_t)sum;

        if (write(client_fd, &sum_to_send, sizeof(sum_to_send)) != sizeof(sum_to_send)) {
            perror("write");
        }

        close(client_fd);
    }

    close(server_fd);

    return EXIT_SUCCESS;
}