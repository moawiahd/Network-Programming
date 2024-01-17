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
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <ctype.h>  

#define FILE_NAME_LEN 256
#define BUFFER_LEN 65535

int CreateSocket(int family, int type, int protocol) {
    int sockfd;
    if ((sockfd = socket(family, type, protocol)) < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

void InetPton(int family, const char *str_ptr, void *addr_ptr) {
    if (inet_pton(family, str_ptr, addr_ptr) <= 0) {
        perror("Invalid IP address format");
        exit(EXIT_FAILURE);
    }
}

void ConnectSocket(int fd, const struct sockaddr *sa, socklen_t sa_len) {
    if (connect(fd, sa, sa_len) < 0) {
        perror("Error connecting to server");
        exit(EXIT_FAILURE);
    }
}

void CloseSocket(int sockfd) {
    if (close(sockfd) == -1) {
        perror("Error closing socket");
        exit(EXIT_FAILURE);
    }
}

void PrintBufferSize(int sockfd, int type) {
    int s_buf, r_buf, len;

    len = sizeof(s_buf);
    if (getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &s_buf, &len) == -1) {
        perror("Error getting send buffer size");
        exit(EXIT_FAILURE);
    }

    len = sizeof(r_buf);
    if (getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &r_buf, &len) == -1) {
        perror("Error getting receive buffer size");
        exit(EXIT_FAILURE);
    }

    printf("Initial Send buffer size (%s): %d bytes\n", (type == SOCK_DGRAM) ? "UDP" : "TCP", s_buf);
    printf("Initial Receive buffer size (%s): %d bytes\n", (type == SOCK_DGRAM) ? "UDP" : "TCP", r_buf);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Server IP> <Server Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    int sockfd;
    char buffer[BUFFER_LEN + 1];
    char filename[FILE_NAME_LEN + 1];
    socklen_t server_len = sizeof(server_addr);

    sockfd = CreateSocket(AF_INET, SOCK_DGRAM, 0);
    PrintBufferSize(sockfd, SOCK_DGRAM);

    int s_buf, r_buf;
    printf("Enter new value for the Send buffer:\n");

    // Ensure the user enters an integer for the Send buffer
    while (1) {
        if (scanf("%d", &s_buf) == 1) {
            break;  // Exit the loop if the user enters an integer
        } else {
            printf("Error: Please enter a valid integer for the Send buffer.\n");
            while (getchar() != '\n');  // Clear the input buffer
        }
    }

    printf("Enter new value for the Receive buffer:\n");

    
    while (1) {
        if (scanf("%d", &r_buf) == 1) {
            break;  
        } else {
            printf("Error: Please enter a valid integer for the Receive buffer.\n");
            while (getchar() != '\n');  
        }
    }

    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &s_buf, sizeof(s_buf));
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &r_buf, sizeof(r_buf));

    printf("Edited Send buffer size (UDP): %d\n", s_buf);
    printf("Edited Receive buffer size (UDP): %d\n", r_buf);

    memset(&server_addr, 0, server_len);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    InetPton(AF_INET, argv[1], &server_addr.sin_addr);

    char hello_msg = 0x55;
    char ack_msg = 0xAA;

    sendto(sockfd, &hello_msg, sizeof(hello_msg), 0, (struct sockaddr *)&server_addr, server_len);
    printf("Hello sent\n");

    recvfrom(sockfd, buffer, BUFFER_LEN, 0, (struct sockaddr *)&server_addr, &server_len);
    if (buffer[0] == hello_msg) {
        printf("Hello massage confirmed. Sending acknowledgment\n");
        sendto(sockfd, &ack_msg, sizeof(ack_msg), 0, (struct sockaddr *)&server_addr, server_len);
    } else {
        printf("Server authentication failed\n");
        CloseSocket(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("List of Files on Server:\n");
    while (1) {
        recvfrom(sockfd, buffer, BUFFER_LEN, 0, (struct sockaddr *)&server_addr, &server_len);
        if (strcmp(buffer, "END") == 0) {
            break;
        }
        printf("%s\n", buffer);
        memset(buffer, 0, BUFFER_LEN);
    }

    while (1) {
        memset(filename, 0, FILE_NAME_LEN + 1);
        printf("Please enter a filename or \"done\" to exit:\n");
        scanf("%s", filename);

        // Send "done" as a termination signal
        if (strcmp(filename, "done") == 0) {
            printf("Terminating client...\n");
            sendto(sockfd, filename, strlen(filename), 0, (struct sockaddr *)&server_addr, server_len);
            break;
        }

        printf("Sending \"%s\" to the server\n", filename);
        sendto(sockfd, filename, strlen(filename), 0, (struct sockaddr *)&server_addr, server_len);

        char file_exists;
        ssize_t received_len = recvfrom(sockfd, &file_exists, sizeof(file_exists), 0, (struct sockaddr *)&server_addr, &server_len);
        if (received_len == -1) {
            perror("Error receiving from server");
            CloseSocket(sockfd);
            exit(EXIT_FAILURE);
        }

        if (file_exists == 1) {
            char newFilename[FILE_NAME_LEN + 1];
            char choice;  

            // Ensure the user enters 'Y' or 'N'
            while (1) {
                printf("The file exists. Do you want to save it with a different name? (Y/N)\n");
                scanf(" %c", &choice);  

                if (toupper(choice) == 'Y' || toupper(choice) == 'N') {
                    break;  // Exit the loop if the user enters 'Y' or 'N'
                } else {
                    printf("Error: Please enter 'Y' or 'N'.\n");
                }
            }

            if (toupper(choice) == 'Y') {
                printf("Enter a new filename:\n");
                scanf("%s", newFilename);
            } else {
                strcpy(newFilename, filename);
            }

            memset(buffer, 0, BUFFER_LEN);
            received_len = recvfrom(sockfd, buffer, BUFFER_LEN, 0, (struct sockaddr *)&server_addr, &server_len);
            if (received_len == -1) {
                perror("Error receiving from server");
                CloseSocket(sockfd);
                exit(EXIT_FAILURE);
            }

            FILE *file = fopen(newFilename, "w");
            if (!file) {
                perror("Error opening file");
                CloseSocket(sockfd);
                exit(EXIT_FAILURE);
            }

            fprintf(file, "%.*s", (int)received_len, buffer);
            fflush(file);  // Flush the file stream to ensure data is written immediately
            fclose(file);

            printf("The file has been retrieved and saved as \"%s\".\n", newFilename);
        } else {
            printf("File not found!\n");
        }
    }

    CloseSocket(sockfd);
    return 0;
}

