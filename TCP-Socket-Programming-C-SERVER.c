//Ramzi Abulibbeh 123844 
//Moawiah Al-Doum 139912

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>

int verifyPassword(char *password, char *error_message) {
    int length = strlen(password);
    int categories = 0;
    char missing_requirements[100] = "";
//Here I want the code to first check if the password is 8-16 then checks for the other requirements
    if (length < 8) {
        strcpy(error_message, "Password is too short (less than 8 characters).");
        return 0;
    } else if (length > 16) {
        strcpy(error_message, "Password is too long (more than 16 characters).");
        return 0;
    }

    for (int i = 0; i < length; i++) {
        if (isupper(password[i])) {
            categories |= 1; 
        } else if (islower(password[i])) {
            categories |= 2; 
        } else if (isdigit(password[i])) {
            categories |= 4; 
        } else if (strchr("!@#", password[i]) != NULL) {
            categories |= 8; 
        }
    }

    if (__builtin_popcount(categories) < 3) {
        if ((categories & 1) == 0) {
            strcat(missing_requirements, "*Uppercase alphabet, ");
        }
        if ((categories & 2) == 0) {
            strcat(missing_requirements, "*Lowercase alphabet, ");
        }
        if ((categories & 4) == 0) {
            strcat(missing_requirements, "*Numbers, ");
        }
        if ((categories & 8) == 0) {
            strcat(missing_requirements, "*Special characters.");
        }
        strcpy(error_message, "These requirements are missing: ");
        strncat(error_message, missing_requirements, strlen(missing_requirements) - 2); 
        return 0;
    }

    return 1; // Password is valid
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);

    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("Error in socket");
        exit(1);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Error in binding");
        exit(1);
    }

    if (listen(serverSocket, 10) == 0) {
        printf("Server listening on port %d...\n", port);
    } else {
        perror("Error in listening");
        exit(1);
    }

    clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (clientSocket < 0) {
        perror("Error in accepting");
        exit(1);
    }

    while (1) {
        char receivedPassword[100];
        int result;
        char error_message[100];

        ssize_t bytes_received = recv(clientSocket, receivedPassword, sizeof(receivedPassword), 0);
        if (bytes_received < 0) {
            perror("Error in receiving");
            exit(1);
        } else if (bytes_received == 0) {
            
            break;
        } else {
            receivedPassword[bytes_received] = '\0'; 
        }

        result = verifyPassword(receivedPassword, error_message);

        //Send the result back to the client
        send(clientSocket, &result, sizeof(result), 0);

        if (result == 0) {
            // if password is not valid it sends an error message to the client
            send(clientSocket, error_message, sizeof(error_message), 0);
        }

        printf("Client IP: %s, Port: %d, Password: %s\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), receivedPassword);
    }

    
    close(serverSocket);
    return 0;
}

