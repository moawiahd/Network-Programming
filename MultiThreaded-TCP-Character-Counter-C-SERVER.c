//Ramzi Abulibbeh 123844 
//Moawiah Al-Doum 139912

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>

#define MAX_THREADS 100

int thread_counter = 0; // to keep track of threads
pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER; 

void* handle_client(void* arg) {
    int client_sock = *((int*)arg);
    free(arg);

    // for incrementing thread count and print it
    pthread_mutex_lock(&thread_mutex);
    int current_thread = ++thread_counter;
    printf("T%d: waiting for client request\n", current_thread);
    printf("T%d: creating thread\n", current_thread);
    printf("T%d: number of created threads so far = %d\n", current_thread, thread_counter);
    pthread_mutex_unlock(&thread_mutex);

    char buffer[1024];
    ssize_t bytes_read = recv(client_sock, buffer, sizeof(buffer), 0);
    buffer[bytes_read] = '\0';

    // for counting occurrences
    char last_char = buffer[strlen(buffer) - 1];
    int count = -1;
    for (int i = 0; i < strlen(buffer); ++i) {
        if (buffer[i] == last_char) {
            count++;
        }
    }

    printf("T%d: received string: %s\n", current_thread, buffer);
    printf("T%d: # of %c’s = %d\n", current_thread, last_char, count);
    send(client_sock, &count, sizeof(int), 0);
    printf("T%d: sent result back, exited.\n", current_thread);

    close(client_sock);
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <service_name>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;      
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_flags = AI_PASSIVE;     

    int status = getaddrinfo(NULL, argv[1], &hints, &result);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    int server_sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (server_sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (bind(server_sock, result->ai_addr, result->ai_addrlen) == -1) {
        perror("bind");
        close(server_sock);
        freeaddrinfo(result);
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);

    if (listen(server_sock, SOMAXCONN) == -1) {
        perror("listen");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    for (;;) {
        
        struct sockaddr_storage client_addr;
        socklen_t addr_size = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);

        pthread_t thread;
        int* client_sock_ptr = malloc(sizeof(int));
        *client_sock_ptr = client_sock;
        
        if (pthread_create(&thread, NULL, handle_client, client_sock_ptr) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }

        // Detach the thread to avoid memory leak
        pthread_detach(thread);
    }

    
    close(server_sock);

    return 0;
}

