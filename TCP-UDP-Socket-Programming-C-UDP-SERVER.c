//Moawiah AL DOUM 139912
//Ramzi Mohammed 123844

#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <dirent.h>

#define MAXLINE 1024
#define SA struct sockaddr

int authenticateClient(int sockfd, struct sockaddr_in *client_addr, socklen_t len)
{
    char buffer[MAXLINE];

    recvfrom(sockfd, buffer, sizeof(buffer), 0, (SA *)client_addr, &len);

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr->sin_addr, client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_addr->sin_port);
    printf("%s:%d sent %s \n", client_ip, client_port, buffer);

    // Check if the received message is 'END-UDP' to terminate the server
    if (strcmp(buffer, "END-UDP") == 0)
    {printf("Terminating UDP server....\n");exit(0);}

    if (strcmp(buffer, "hello") != 0)
    {printf("Authentication failed\n");return -1;}

    char authenticationByte = 0x55;
    // Send one byte 0x55 to the client
    sendto(sockfd, &authenticationByte, 1, 0, (SA *)client_addr, len);

    char receivedByte;
    // Receive the 0xAA from the client
    recvfrom(sockfd, &receivedByte, 1, 0, (SA *)client_addr, &len);

    if (receivedByte != (char)(0xAA))
    {printf("!!!! Authentication Failed !!!!! \n");return -1;}

    printf("^ ^ Authentication Done ^ ^\n\n");
    return 1;
}

// Function to run the UDP server
void runUDPServer(int sockfd)
{
    for (;;)
    {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);

        printf("-**Waiting for connections\n");

        int authenticationResult = authenticateClient(sockfd, &client_addr, len);
        if (authenticationResult == -1)
            continue;

        // Get the files from the current directory
        DIR *directory;
        struct dirent *entry;

        // Open the current directory
        directory = opendir(".");

        // Check if the directory can be opened
        if (directory == NULL)
        {
            perror("Error in opening the directory");
            exit(EXIT_FAILURE);
        }

        char filesList[MAXLINE];
        bzero(filesList, MAXLINE);
        strcpy(filesList, "------------\n");

        // Read and print each entry in the directory
        while ((entry = readdir(directory)) != NULL)
        {
            strcat(filesList, entry->d_name);
            strcat(filesList, "\n");
        }
        closedir(directory);

        // Send files list available to the client
        sendto(sockfd, filesList, MAXLINE, 0, (SA *)&client_addr, len);

        char fileName[MAXLINE];
        // Receive file name
        bzero(fileName, MAXLINE);
        printf("Waiting for file name...\n");
        recvfrom(sockfd, fileName, MAXLINE, 0, (SA *)&client_addr, &len);

        printf("Server <- %s\n", fileName);

        // Try to open the file
        FILE *file = fopen(fileName, "rb");
        if (file == NULL)
        {
            // File not found
            printf("File %s not found!\n", fileName);
            char zeroByte = 0;

            // Send zero byte to tell the client that the file does not exist
            sendto(sockfd, &zeroByte, 1, 0, (SA *)&client_addr, len);
            continue;
        }

        char oneByte = 1;
        printf("Server -> %d\n", oneByte);
        // Send 1 to the client
        sendto(sockfd, &oneByte, 1, 0, (SA *)&client_addr, len);
        printf("Server -> %s \n", fileName);

        char buffer[MAXLINE];
        // Read bytes from the file and send it to the client
        while (1)
        {
            int bytesRead;
            // Read chunk from the file
            if ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) <= 0)
                break;

            // Send chunk to client
            sendto(sockfd, buffer, bytesRead, 0, (SA *)&client_addr, len);
        }

        // Send empty datagram
        sendto(sockfd, NULL, 0, 0, (SA *)&client_addr, sizeof(client_addr));
    }
}

// Main function
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("You must provide one command line argument -> port\n");
        exit(1);
    }

    // Check port
    int port = atoi(argv[1]);
    if (port < 0 || port > 65535)
    {
        printf("Wrong port number\n");
        exit(1);
    }

    int sockfd;
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));

    // Assign IP, PORT
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    // Socket create and verification
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

    if (bind(sockfd, (SA *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Bind failed");
        exit(1);
    }

    // Server function
    runUDPServer(sockfd);

    return 0;
}
