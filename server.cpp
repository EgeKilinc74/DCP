Ege Kýlýnç, [13.12.2024 14:17]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

// Data structure to hold client information
typedef struct {
    int socket;
    struct sockaddr_in address;
    int id;
    char name[50];
} Client;

Client *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void broadcast_message(const char *message, int exclude_id) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] && clients[i]->id != exclude_id) {
            send(clients[i]->socket, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg) {
    Client *client = (Client *)arg;
    char buffer[BUFFER_SIZE];

    // Welcome message
    snprintf(buffer, BUFFER_SIZE, "Welcome to the game, %s!\n", client->name);
    send(client->socket, buffer, strlen(buffer), 0);
    snprintf(buffer, BUFFER_SIZE, "%s has joined the game.\n", client->name);
    broadcast_message(buffer, client->id);

    while (1) {
        int bytes_received = recv(client->socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            snprintf(buffer, BUFFER_SIZE, "%s has left the game.\n", client->name);
            broadcast_message(buffer, client->id);
            break;
        }
        buffer[bytes_received] = '\0';
        snprintf(buffer + bytes_received, BUFFER_SIZE - bytes_received, " [%s]", client->name);
        broadcast_message(buffer, client->id);
    }

    close(client->socket);
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] && clients[i]->id == client->id) {
            clients[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    free(client);
    return NULL;
}

int main() {
    int server_socket, new_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_len = sizeof(client_address);

    // Initialize server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server started on port %d\n", PORT);

    while (1) {
        new_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_len);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }

        // Add new client
        pthread_mutex_lock(&clients_mutex);
        int i;
        for (i = 0; i < MAX_CLIENTS; ++i) {
            if (!clients[i]) {
                clients[i] = malloc(sizeof(Client));
                clients[i]->socket = new_socket;
                clients[i]->address = client_address;
                clients[i]->id = i;
                snprintf(clients[i]->name, 50, "Player%d", i + 1);
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        if (i == MAX_CLIENTS) {
            printf("Maximum clients reached. Connection refused.\n");
            close(new_socket);
        } else {
            pthread_t tid;
            pthread_create(&tid, NULL, handle_client, (void *)clients[i]);
            pthread_detach(tid);
        }
    }

    close(server_socket);
    return 0;
}

