//Ramzi Abulibbeh 123844 
//Moawiah Al-Doum 139912

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

int CreateSocket(int family, int type, int protocol) {
    int sockfd;
    if ((sockfd = socket(family, type, protocol)) < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

void BindSocket(int fd, const struct sockaddr *sa, socklen_t salen) {
    if (bind(fd, sa, salen) < 0) {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }
}

int ReadSocket(int fd, void *ptr, size_t nbytes) {
    int n;
    if ((n = read(fd, ptr, nbytes)) < 0) {
        perror("Error reading from socket");
        exit(EXIT_FAILURE);
    }
    return n;
}

void CloseSocket(int sockfd) {
    if (close(sockfd) == -1) {
        perror("Error closing socket");
        exit(EXIT_FAILURE);
    }
}

void HandleError(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int listenfd, filefd;
    socklen_t clilen;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in servaddr, cliaddr;
    int read_b, send_b;

    listenfd = CreateSocket(AF_INET, SOCK_DGRAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    BindSocket(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    printf("Server listening on port %d...\n", atoi(argv[1]));

    clilen = sizeof(cliaddr);

    char hello_msg = 0x55;
    char ack_msg = 0xAA;

    recvfrom(listenfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&cliaddr, &clilen);
    if (buffer[0] == hello_msg) {
        printf("hello received\n");
        sendto(listenfd, &hello_msg, sizeof(hello_msg), 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
    } else {
        printf("Authentication failed\n");
        CloseSocket(listenfd);
        exit(EXIT_FAILURE);
    }

    recvfrom(listenfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&cliaddr, &clilen);
    if (buffer[0] != ack_msg) {
        printf("Authentication failed. Exiting.\n");
        CloseSocket(listenfd);
        exit(EXIT_FAILURE);
    } else {
        printf("Acknowledgment received\n");
    }

    DIR *directory;
    struct dirent *entry;
    directory = opendir(".");

    if (directory != NULL) {
        while ((entry = readdir(directory)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            sendto(listenfd, entry->d_name, strlen(entry->d_name), 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
        }
        bzero(buffer, sizeof(buffer));
        char end_send[BUFFER_SIZE] = "END";
        sendto(listenfd, end_send, sizeof(end_send), 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
        closedir(directory);
    } else {
        printf("Unable to open directory\n");
        exit(EXIT_FAILURE);
    }

    int terminate_server = 0;

    while (!terminate_server) {
        memset(buffer, 0, BUFFER_SIZE);
        recvfrom(listenfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&cliaddr, &clilen);

        printf("Received \"%s\" from the client using UDP(%s, %d)\n", buffer, inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

        // Check if the received string is "done"
        if (strcmp(buffer, "done") == 0) {
            // Set the termination flag
            terminate_server = 1;
            printf("Terminating server...\n");
            break;  // Exit the loop immediately when receiving "done"
        }

        // Check if the file exists in the directory
        char file_exist;
        struct stat st;
        if (stat(buffer, &st) == 0) {
            // File exists
            file_exist = 1;
            sendto(listenfd, &file_exist, sizeof(file_exist), 0, (struct sockaddr *)&cliaddr, clilen);

            filefd = open(buffer, O_RDONLY);
            if (filefd < 0) {
                perror("Error opening file");
                continue;
            }

            int f_size = lseek(filefd, 0, SEEK_END);
            lseek(filefd, 0, SEEK_SET);

            printf("Sending the file to the client.\n");
            while ((read_b = ReadSocket(filefd, buffer, BUFFER_SIZE)) > 0) {
                send_b = sendto(listenfd, buffer, read_b, 0, (struct sockaddr *)&cliaddr, clilen);
                if (send_b < 0) {
                    perror("Error sending to client");
                }
            }

            CloseSocket(filefd);
            printf("File sent to the client.\n");

        } else {
            // File does not exist
            file_exist = 0;
            sendto(listenfd, &file_exist, sizeof(file_exist), 0, (struct sockaddr *)&cliaddr, clilen);
            printf("File not found. Awaiting new request.\n");
        }
    }

    CloseSocket(listenfd);
    return 0;
}

