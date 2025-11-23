#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "networking.h" // Usa as funções de criação de socket e endereço

#define MAX_CLIENTS 10 // Número máximo de clientes
#define BUFFER_SIZE 2048
#define PORT 6969 // Porta que o servidor vai ouvir

int client_sockets[MAX_CLIENTS] = {0};
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// Função para enviar mensagens a todos os clientes
void broadcast_message(int sender_sock_fd, const char *message) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        int client_fd = client_sockets[i];
        // O servidor só envia a mensagem para quem NÃO é o remetente
        // porque o remetente já a exibiu localmente no client.c
        if (client_fd > 0 && client_fd != sender_sock_fd) {
            send(client_fd, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Função de thread para lidar com um único cliente
void *handle_client(void *client_socket_ptr) {
    int client_sock_fd = *((int *)client_socket_ptr);
    int read_size;
    char buffer[BUFFER_SIZE];
    char client_name[25]; 
    
    // --- NOVA FUNCIONALIDADE: 1º RECV é o NOME ---
    // A primeira mensagem é o nome do utilizador (máx 24 bytes)
    read_size = recv(client_sock_fd, client_name, sizeof(client_name) - 1, 0);

    if (read_size <= 0) {
        printf("[SERVIDOR] Cliente sem nome se desconectou.\n");
        goto cleanup; // Vai para a secção de limpeza
    }
    
    client_name[read_size] = '\0'; // Termina a string do nome
    
    printf("[SERVIDOR] Nova conexão aceita. Nome: %s.\n", client_name);
    
    // Mensagem de boas-vindas
    char welcome_msg[BUFFER_SIZE];
    snprintf(welcome_msg, sizeof(welcome_msg), "Bem-vindo ao Chatroom C do Yohan, %s! Há %d lugares livres.\n", client_name, MAX_CLIENTS);
    send(client_sock_fd, welcome_msg, strlen(welcome_msg), 0);
    
    // Broadcast de entrada
    char join_msg[BUFFER_SIZE];
    snprintf(join_msg, sizeof(join_msg), "[CHAT] %s entrou na sala.\n", client_name);
    broadcast_message(client_sock_fd, join_msg);
    
    // --- CICLO PRINCIPAL DE MENSAGENS ---
    // O servidor agora recebe mensagens normais
    while ((read_size = recv(client_sock_fd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[read_size] = '\0'; 
        
        if (strncmp(buffer, "exit", 4) == 0) {
            break; 
        }

        // Formata a mensagem com o nome do utilizador recebido
        char full_message[BUFFER_SIZE + 30];
        snprintf(full_message, sizeof(full_message), "[%s]: %s", client_name, buffer);

        printf("[Broadcast] %s", full_message);
        
        // Envia para todos, exceto o remetente
        broadcast_message(client_sock_fd, full_message);
        
        memset(buffer, 0, BUFFER_SIZE);
    }
    
    // Cleanup
    cleanup:; // Etiqueta para o goto
    
    if (read_size == 0 || read_size == -1) {
        char leave_msg[BUFFER_SIZE];
        snprintf(leave_msg, sizeof(leave_msg), "[CHAT] %s saiu da sala.\n", client_name);
        broadcast_message(client_sock_fd, leave_msg);
    }
    close(client_sock_fd);

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == client_sock_fd) {
            client_sockets[i] = 0; 
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    
    free(client_socket_ptr);
    pthread_exit(NULL); 
}

int main(int argc, char *argv[]) {
    int server_fd, new_socket;
    int opt = 1;
    struct sockaddr_in client_address; 
    socklen_t client_addrlen = sizeof(client_address); 

    printf("--- Servidor de Chat C Multi-threaded do Yohan ---\n");

    if ((server_fd = createTCPIpv4Socket()) == 0) {
        perror("A criação do socket falhou");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Setsockopt falhou");
        exit(EXIT_FAILURE);
    }
    
    struct sockaddr_in *server_address = createIpv4Address(NULL, PORT);

    if (server_address == NULL) {
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (bind(server_fd, (struct sockaddr *)server_address, sizeof(struct sockaddr_in)) < 0) {
        perror("Bind falhou");
        free(server_address);
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) { 
        perror("Listen falhou");
        free(server_address);
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Servidor a ouvir na porta %d...\n", PORT);
    
    free(server_address);

    while (1) {
        int *new_sock = (int *)malloc(sizeof(int));
        
        if ((*new_sock = accept(server_fd, (struct sockaddr *)&client_address, &client_addrlen)) < 0) {
            perror("Erro ao aceitar");
            free(new_sock); 
            continue; 
        }
        
        int i;
        pthread_mutex_lock(&clients_mutex);
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] == 0) {
                client_sockets[i] = *new_sock;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        if (i == MAX_CLIENTS) {
            printf("[SERVIDOR] Conexão rejeitada: Máximo de clientes atingido.\n");
            close(*new_sock);
            free(new_sock);
            continue;
        }

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_client, (void *)new_sock) < 0) {
            perror("Não foi possível criar a thread do cliente");
            close(*new_sock);
            free(new_sock);
            
            pthread_mutex_lock(&clients_mutex);
            client_sockets[i] = 0;
            pthread_mutex_unlock(&clients_mutex);
            
            continue;
        }
        pthread_detach(client_thread);
    }
    
    close(server_fd);
    
    return 0;
}