#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>

#define SIZE 1000
#define MAX_CLIENTS 10

int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(9002);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Binding failed");
        close(serverSocket);
        exit(1);
    }

    if (listen(serverSocket, MAX_CLIENTS) < 0) {
        perror("Listening failed");
        close(serverSocket);
        exit(1);
    }

    fd_set masterSet, readSet;
    FD_ZERO(&masterSet);
    FD_ZERO(&readSet);

    FD_SET(serverSocket, &masterSet);
    int maxFd = serverSocket;

    int clientSockets[MAX_CLIENTS] = {0};
    char clientNames[MAX_CLIENTS][SIZE] = {0}; // Store client usernames

    while (1) {
        readSet = masterSet;

        if (select(maxFd + 1, &readSet, NULL, NULL, NULL) < 0) {
            perror("Select failed");
            close(serverSocket);
            exit(1);
        }

        if (FD_ISSET(serverSocket, &readSet)) {
            int newSocket = accept(serverSocket, NULL, NULL);
            if (newSocket < 0) {
                perror("Accept failed");
                continue;
            }

            int i;
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (clientSockets[i] == 0) {
                    clientSockets[i] = newSocket;
                    FD_SET(newSocket, &masterSet);
                    if (newSocket > maxFd) {
                        maxFd = newSocket;
                    }

                    // Receive username from new client
                    char username[SIZE];
                    int bytesReceived = recv(newSocket, username, SIZE - 1, 0);
                    if (bytesReceived > 0) {
                        username[bytesReceived] = '\0';
                        strncpy(clientNames[i], username, SIZE);

                        // Notify all clients about new user
                        char joinMessage[SIZE + 50]; // Adjusted size to avoid overflow
                        snprintf(joinMessage, sizeof(joinMessage), "%s has joined", username);
                        printf("%s\n", joinMessage); // Print to server terminal
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (clientSockets[j] != 0 && j != i) {
                                send(clientSockets[j], joinMessage, strlen(joinMessage), 0);
                            }
                        }
                    }
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = clientSockets[i];
            if (FD_ISSET(sd, &readSet)) {
                char buffer[SIZE];
                int bytesReceived = recv(sd, buffer, SIZE - 1, 0); // Reserve space for null terminator
                if (bytesReceived <= 0) {
                    close(sd);
                    FD_CLR(sd, &masterSet);
                    clientSockets[i] = 0;

                    // Notify all clients about user leaving
                    if (clientNames[i][0] != '\0') {
                        char leaveMessage[SIZE + 50]; // Adjusted size to avoid overflow
                        snprintf(leaveMessage, sizeof(leaveMessage), "%s has left", clientNames[i]);
                        printf("%s\n", leaveMessage); // Print to server terminal
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (clientSockets[j] != 0) {
                                send(clientSockets[j], leaveMessage, strlen(leaveMessage), 0);
                            }
                        }
                    }
                } else {
                    buffer[bytesReceived] = '\0';
                    printf("%s\n", buffer); // Print received message

                    // Broadcast message to all other clients
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (clientSockets[j] != 0 && clientSockets[j] != sd) {
                            send(clientSockets[j], buffer, strlen(buffer), 0);
                        }
                    }
                }
            }
        }
    }

    close(serverSocket);
    return 0;
}
