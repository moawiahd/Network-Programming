//Moawiah AL DOUM 139912
//Ramzi Mohammed 123844

#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <signal.h>
#include <sys/wait.h>

#define MAX_BUFFER_SIZE 1024
#define SA struct sockaddr

void processClient(int connection_fd, char filename[MAX_BUFFER_SIZE])
{
    int bytesRead;
    struct sockaddr_in client_address; // Declare the client address structure
    socklen_t len = sizeof(client_address);

    if (strcmp(filename, "Error in file name ") == 0)
    {
        return;
    }

    // Send confirmation on the file
    char confirmation = 1;
    write(connection_fd, &confirmation, 1);

    // Create a file and read data
    FILE *file = fopen(filename, "wb");
    char data[MAX_BUFFER_SIZE];

    while ((bytesRead = read(connection_fd, data, MAX_BUFFER_SIZE)) > 0)
    {
        // Store data in the created file
        fwrite(data, 1, bytesRead, file);
    }

    if (bytesRead == 0)
        printf("%s successfully.\n", filename);

    fclose(file);
}

// SIGCHLD handler
void handleSIGCHLD(int signo)
{
    pid_t pid;
    int stat;

    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
        printf("Child %d terminated successfully!!\n", pid);

    return;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    bzero(&server_addr, sizeof(server_addr));

    // Check if the port is valid
    int port = atoi(argv[1]);
    if (port < 0 || port > 65535)
    {
        printf("Invalid port number: %d\n", port);
        exit(1);
    }

    // Socket create and verification
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        perror("Error in socket creation\n");
        exit(1);
    }

    // Assign IP, PORT
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    // Binding newly created socket to given IP and verification
    if (bind(server_socket, (SA *)&server_addr, sizeof(server_addr)) != 0)
    {
        perror("Error in bind\n");
        exit(1);
    }

    // Now the server is ready to listen and verify
    if (listen(server_socket, 10) != 0)
    {
        perror("Error in listen\n");
        exit(1);
    }

    printf("-------- SERVER is listening ... Waiting to Connect ! -------- \n");

    // Create a signal handler to catch SIGCHLD signal
    signal(SIGCHLD, handleSIGCHLD);
    socklen_t len = sizeof(client_addr);

    while (1)
    {
        // Accept a client
        client_socket = accept(server_socket, (SA *)&client_addr, &len);
        if (client_socket < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            perror(" !!!!! Error Receving !!!!! \n");
            exit(1);
        }

        char filename[MAX_BUFFER_SIZE];
        bzero(filename, MAX_BUFFER_SIZE);

        // Read file name
        if (read(client_socket, filename, sizeof(filename)) <= 0)
        {
            return 0;
        }

        if (!strcmp(filename, "END-TCP"))
        {
            printf("<><><><><> Shutting Down <><><><><>\n");
            exit(0);
        }

        printf("+++ Received Connection Success. +++\n");

        if (fork() == 0)
        {
            close(server_socket);
            processClient(client_socket, filename);
            exit(0);
        }

        close(client_socket);
    }

    return 0;
}
