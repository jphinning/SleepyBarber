// Joao Hinning e Luan Rogalewski

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 5
#define PORT 3000

sem_t barber_sem, customer_sem, mutex, print_sem;
int num_waiting = 0;
int currently_serving = -1;

typedef struct {
    int id;
    int socket;
} Client;

void print_status(const char *status) {
    sem_wait(&print_sem);
    printf("%s\n", status);
    sem_post(&print_sem);
}

void *barber_thread(void *arg) {
    while (1) {
        sem_wait(&customer_sem);
        sem_wait(&mutex);

        currently_serving = num_waiting;
        num_waiting--;

     
        sem_post(&mutex);

        char status[100];
        sprintf(status, "Barber is cutting hair for client %d", currently_serving);
        print_status(status);

        sprintf(status, "This is how many are waiting %d", num_waiting);
        print_status(status);

        
        sem_post(&barber_sem);

        sleep(rand() % 30 + 1);

        sprintf(status, "Barber finished cutting hair for client %d.", currently_serving);
        print_status(status);

        currently_serving = -1;

    }
}

void *customer_thread(void *arg) {
    Client *client = (Client *)arg;
    char status[100];

    sem_wait(&mutex);

    if (num_waiting < MAX_CLIENTS) {
        num_waiting++;

        sem_post(&customer_sem);
        sem_post(&mutex);

        sprintf(status, "You are in the queue. Position in queue: %d. This is your ID = %d\n", num_waiting, client->id);

        send(client->socket, status, sizeof(status), 0);
        
        sem_wait(&barber_sem);

        sprintf(status, "You are being served by the barber. This is your ID = %d.\n", client->id);
       
        send(client->socket, status, sizeof(status), 0);
        close(client->socket);
    } else {
        sem_post(&mutex);
        send(client->socket, "No available seats. Please come back later.\n", sizeof("No available seats. Please come back later.\n"), 0);
        close(client->socket);
    }

    free(client);
    return NULL;
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Initialize server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket
    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

    // Listen for incoming connections
    listen(server_socket, 500);

    // Initialize semaphores
    sem_init(&barber_sem, 0, 0);
    sem_init(&customer_sem, 0, 0);
    sem_init(&mutex, 0, 1);
    sem_init(&print_sem, 0, 1);

    // Create barber thread
    pthread_t barber;
    pthread_create(&barber, NULL, barber_thread, NULL);

    printf("Server Running on PORT: %d \n", PORT);

    int client_id = 0;

    while (1) {
        // Accept a connection
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);

        // Create a thread to handle the client
        Client *client = (Client *)malloc(sizeof(Client));
        client->id = client_id++;
        client->socket = client_socket;

        pthread_t customer;
        pthread_create(&customer, NULL, customer_thread, (void *)client);
    }

    return 0;
}