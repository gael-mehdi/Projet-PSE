#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

int client_count = 0;
int client_sockets[MAX_CLIENTS];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void send_message_all(char* message, int current_client) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < client_count; i++) {
        if (client_sockets[i] != current_client) {
            send(client_sockets[i], message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&mutex);
}

void *handle_client(void *arg) {
    int client_socket = *((int *)arg);
    char buffer[BUFFER_SIZE];
    char welcome_message[] = "Bienvenue sur le serveur de messagerie !\n";
    
    send(client_socket, welcome_message, strlen(welcome_message), 0);
    printf("Nouvelle connexion établie. Socket client : %d\n", client_socket);
    
    pthread_mutex_lock(&mutex);
    client_sockets[client_count++] = client_socket;
    pthread_mutex_unlock(&mutex);
    
    while (1) {
        int message_length = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (message_length <= 0) {
            pthread_mutex_lock(&mutex);
            for (int i = 0; i < client_count; i++) {
                if (client_sockets[i] == client_socket) {
                    printf("Déconnexion. Socket client : %d\n", client_socket);
                    close(client_socket);
                    while (i < client_count - 1) {
                        client_sockets[i] = client_sockets[i + 1];
                        i++;
                    }
                    client_count--;
                    break;
                }
            }
            pthread_mutex_unlock(&mutex);
            break;
        }
        buffer[message_length] = '\0';
        send_message_all(buffer, client_socket);
    }
    
    return NULL;
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    pthread_t thread_id;
    
    // Création du socket serveur
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Erreur lors de la création du socket serveur");
        exit(EXIT_FAILURE);
    }
    
    // Configuration de l'adresse du serveur
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(12345);
    
    // Lien du socket à l'adresse et au port spécifiés
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Erreur lors du lien du socket serveur");
        exit(EXIT_FAILURE);
    }
    
    // Écoute des connexions entrantes
    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("Erreur lors de l'écoute des connexions entrantes");
        exit(EXIT_FAILURE);
    }
    
    printf("Serveur de messagerie lancé. En attente de connexions...\n");
    
    while (1) {
        socklen_t client_address_length = sizeof(client_address);
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_length);
        
        if (client_socket == -1) {
            perror("Erreur lors de la tentative d'acceptation d'une connexion");
            exit(EXIT_FAILURE);
        }
        
        pthread_create(&thread_id, NULL, handle_client, &client_socket);
        pthread_detach(thread_id);
    }
    
    close(server_socket);
    
    return 0;
}