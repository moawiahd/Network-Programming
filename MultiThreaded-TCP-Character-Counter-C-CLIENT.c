//Ramzi Abulibbeh 123844 
//Moawiah Al-Doum 139912

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <semaphore.h>

struct ThreadData {
    char character;
    int thread_id;
    char first_str[1024]; 
    char ip_address[1024];
    char port_number[1024];
};

int total_occurrences = 0; 
sem_t thread_semaphore; //for thread synchronization

void* send_request(void* arg) {
    struct ThreadData* data = (struct ThreadData*)arg;

    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;       
    hints.ai_socktype = SOCK_STREAM; 

    int status = getaddrinfo(data->ip_address, data->port_number, &hints, &result);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    int client_sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (client_sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (connect(client_sock, result->ai_addr, result->ai_addrlen) == -1) {
        perror("connect");
        close(client_sock);
        freeaddrinfo(result);
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);

    // for appending the character to the end
    strncat(data->first_str, &data->character, 1);

    // Wait for the semaphore to ensure ordered output
    sem_wait(&thread_semaphore);

    // Send the modified first string to the server
    send(client_sock, data->first_str, strlen(data->first_str), 0);

    int count;
    recv(client_sock, &count, sizeof(int), 0);

    printf("T%d: # of %c’s in the first string = %d\n", data->thread_id, data->character, count);

    // to increment the total occurrences count
    total_occurrences += count;

    // Signal the semaphore to allow the next thread to execute
    sem_post(&thread_semaphore);

    close(client_sock);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_name> <service_name>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    sem_init(&thread_semaphore, 0, 1);

    char first_str[1024];
    char second_str[1024];
    printf("T1: Enter an Alpha-Numeric string to look in: ");
    fgets(first_str, sizeof(first_str), stdin);
    first_str[strcspn(first_str, "\n")] = '\0'; 

    printf("T1: Enter characters to be counted (as one string): ");
    fgets(second_str, sizeof(second_str), stdin);
    second_str[strcspn(second_str, "\n")] = '\0'; 

    printf("T1: Creating %lu threads to count for %s\n", strlen(second_str), second_str);

    pthread_t threads[strlen(second_str)];
    struct ThreadData thread_data_array[strlen(second_str)];

    for (size_t i = 0; i < strlen(second_str); ++i) {
        thread_data_array[i].character = second_str[i];
        thread_data_array[i].thread_id = i + 2; // to Start thread ID from 2 to avoid conflict with main thread which is T1
        strncpy(thread_data_array[i].first_str, first_str, sizeof(thread_data_array[i].first_str));
        strcpy(thread_data_array[i].ip_address, argv[1]);
        strcpy(thread_data_array[i].port_number, argv[2]);
        if (pthread_create(&threads[i], NULL, send_request, &thread_data_array[i]) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    for (size_t i = 0; i < strlen(second_str); ++i) {
        pthread_join(threads[i], NULL);
    }

    printf("T1: # of total occurrences = %d\n", total_occurrences);
    printf("T1: exiting...\n");

    sem_destroy(&thread_semaphore);

    return 0;
}

