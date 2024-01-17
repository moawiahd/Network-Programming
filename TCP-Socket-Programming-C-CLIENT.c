//Ramzi Abulibbeh 123844 
//Moawiah Al-Doum 139912

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>

// Color codes for the text
#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define RESET "\x1b[0m"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server IP> <server port>\n", argv[0]);
        exit(1);
    }

    char *serverIP = argv[1];
    int serverPort = atoi(argv[2]);

    int clientSocket;
    struct sockaddr_in serverAddr;

    
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        perror("Error in socket");
        exit(1);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    serverAddr.sin_addr.s_addr = inet_addr(serverIP);

    
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Error in connection");
        exit(1);
    }

    // I added this extra just to tell the client that it has connected successfully
    printf("Connected to the server successfully.\n");

    while (1) {
        char password[100];
        int result;
        char errorMessage[100];

        printf("------------------------------------------------------\n**Type 'new' to enter a new password or 'done' to exit: ");
        char userChoice[10];
        scanf("%s", userChoice);
        
        //To force the user to enter "new" or "done" only, any other input here it would print Invalid choice
        if (strcmp(userChoice, "new") == 0) {
            
            printf("Enter your password: ");
            scanf("%s", password);

            // Send the password to the server
            send(clientSocket, password, strlen(password), 0);

            // Receive and display the result from the server
            recv(clientSocket, &result, sizeof(result), 0);

            if (result == 1) {
                printf(GREEN "Password is valid.\n" RESET);
            } else {
                recv(clientSocket, errorMessage, sizeof(errorMessage), 0);
                errorMessage[strlen(errorMessage)] = '\0';
                printf(RED "Password is invalid: %s\n" RESET, errorMessage);
            }
        } else if (strcmp(userChoice, "done") == 0) {
            close(clientSocket);
            printf("Session terminated.\n");
            exit(0);
        } else {
            printf("Invalid choice. Please type 'new' or 'done'.\n");
        }
    }

    
    close(clientSocket);
    return 0;
}

