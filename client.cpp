Ege Kýlýnç, [13.12.2024 14:17]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

void *receive_messages(void *arg) {
    int sockfd = *((int *)arg); // arg'yi int türüne dönüþtür
    char buffer[BUFFER_SIZE];

    while (1) {
        int bytes_received = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            break;
        }
        buffer[bytes_received] = '\0';
        printf("%s\n", buffer);
    }
    return NULL;
}

int main() {
    int sockfd;
    struct sockaddr_in server_address;
    char message[BUFFER_SIZE];

    // Socket oluþtur
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        exit(EXIT_FAILURE);
    }

    // Sunucuya baðlan
    if (connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Mesajlarý almak için bir thread baþlat
    pthread_t receive_thread;
    pthread_create(&receive_thread, NULL, receive_messages, (void *)&sockfd);

    // Mesaj göndermek için ana döngü
    while (1) {
        printf("Enter message: ");
        fgets(message, BUFFER_SIZE, stdin);
        message[strcspn(message, "\n")] = '\0';  // Yeni satýr karakterini kaldýr

        // Mesajý sunucuya gönder
        send(sockfd, message, strlen(message), 0);
    }

    close(sockfd);
    return 0;
}

