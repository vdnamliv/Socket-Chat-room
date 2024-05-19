#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define SIZE 1000

void *receiveMessages(void *socketDescriptor) {
    int sockfd = *(int *)socketDescriptor;
    char serverResponse[SIZE];
    while (1) {
        int bytesReceived = recv(sockfd, serverResponse, SIZE, 0);
        if (bytesReceived <= 0) {
            perror("Receiving failed");
            close(sockfd);
            pthread_exit(NULL);
        }
        serverResponse[bytesReceived] = '\0';
        printf("%s\n", serverResponse);
    }
    return NULL;
}

int main() {
    int socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socketDescriptor < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(9002);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (connect(socketDescriptor, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Connection failed");
        close(socketDescriptor);
        exit(1);
    }

    char username[SIZE];
    printf("Enter your username: ");
    fgets(username, SIZE, stdin);
    username[strcspn(username, "\n")] = 0; // Remove trailing newline

    // Send username to server
    send(socketDescriptor, username, strlen(username), 0);

    pthread_t receiveThread;
    pthread_create(&receiveThread, NULL, receiveMessages, &socketDescriptor);

    char message[SIZE];
    while (1) {
        fgets(message, SIZE, stdin);
        message[strcspn(message, "\n")] = 0; // Remove trailing newline

        char formattedMessage[SIZE + SIZE];
        snprintf(formattedMessage, sizeof(formattedMessage), "%.998s: %.998s", username, message);
        send(socketDescriptor, formattedMessage, strlen(formattedMessage), 0);
    }

    close(socketDescriptor);
    return 0;
}
