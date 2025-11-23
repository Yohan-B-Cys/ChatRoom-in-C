#include <stdio.h>
#include <stdlib.h>    // Para EXIT_FAILURE, malloc, free
#include <unistd.h>    // Para close(), write(), STDOUT_FILENO
#include <string.h>    // Para strlen, memset, strncmp, strcspn
#include <pthread.h>   // ESSENCIAL para a comunicação simultânea
#include "networking.h" // Usa as funções de criação de socket e endereço

#define BUFFER_SIZE 2048
#define SERVER_IP "127.0.0.1"  //essa é a configuração de IP eu deixei no localhost para teste local.
#define PORT 6969              // esse é o numero da porta ela pode ser qualquer numero entre 1024 e 65.535 eu escolhi 6969 pq achei NICE .

// File Descriptor global
int client_sock_fd = 0; 

// VARIÁVEL GLOBAL: Armazena o nome do utilizador
char username[25] = {0}; 

/**
 * @brief Thread para continuamente receber e exibir mensagens do servidor.
 */
void *receive_messages(void *arg) {
    char buffer[BUFFER_SIZE];
    int read_size;

    while (1) {
        // Recebe dados do servidor
        read_size = recv(client_sock_fd, buffer, BUFFER_SIZE - 1, 0);

        if (read_size > 0) {
            buffer[read_size] = '\0';
            // Usa write() para evitar que a entrada do utilizador no teclado seja corrompida
            write(STDOUT_FILENO, buffer, strlen(buffer)); 
            fflush(stdout);
            
        } else if (read_size == 0) {
            // Servidor desconectado
            printf("\n[ALERTA] Servidor desconectou.\n");
            exit(0); 
        } else if (read_size == -1) {
            perror("Recv falhou");
            exit(1); 
        }
        
        memset(buffer, 0, BUFFER_SIZE);
    }
    
    pthread_exit(NULL);
}

/**
 * @brief Thread para lidar com a entrada do utilizador e enviar mensagens.
 */
void *send_messages(void *arg) {
    char message_buffer[BUFFER_SIZE];

    printf("\n---Cliente de Chat C que o Yohan ficou 2 madrugadas fazendo---\n");
    printf("Entre na sala de chat e digite algo!\n");
    printf("Digite as mensagens (ou 'exit' para sair):\n");

    while (1) {
        // Lê a entrada do utilizador
        if (fgets(message_buffer, BUFFER_SIZE, stdin) == NULL) {
            break; 
        }
        
        // --- NOVA FUNCIONALIDADE: Visualização da Própria Mensagem ---
        // 1. Imprime a mensagem localmente, formatada, para que o utilizador veja imediatamente.
        printf("[%s]: %s", username, message_buffer);

        // 2. Envia a mensagem (o servidor é que vai adicionar o nome para o broadcast)
        if (send(client_sock_fd, message_buffer, strlen(message_buffer), 0) < 0) {
            perror("Send falhou");
            break; 
        }

        // Verifica se o utilizador digitou 'exit'
        if (strncmp(message_buffer, "exit", 4) == 0) {
            printf("[ALERTA] Desconectando...\n");
            break; 
        }
    }
    
    pthread_exit(NULL);
}


int main (){
        
    // --- NOVA FUNCIONALIDADE: Pedir Nome do Utilizador ---
    printf("Digite seu nome (máx. 24 caracteres): ");
    char temp_name[25];
    if (fgets(temp_name, sizeof(temp_name), stdin) != NULL) {
        // Remove o caractere de nova linha ('\n') adicionado pelo fgets
        temp_name[strcspn(temp_name, "\n")] = 0;
        // Copia o nome para a variável global
        strncpy(username, temp_name, sizeof(username) - 1);
        username[sizeof(username) - 1] = '\0';
    } else {
        // Tratar erro de entrada
        fprintf(stderr, "Erro ao ler o nome.\n");
        return EXIT_FAILURE;
    }
    
    // 1. Criação do Socket
    client_sock_fd = createTCPIpv4Socket();
    
    if (client_sock_fd < 0) {
        perror("Falha ao criar o socket");
        return EXIT_FAILURE;
    }
    
    // 2. Criação da Estrutura de Endereço
    struct sockaddr_in *server_address = createIpv4Address(SERVER_IP, PORT);
 
    if (server_address == NULL) {
        close(client_sock_fd);
        return EXIT_FAILURE;
    }

    printf("Tentando conectar a %s na porta %d como %s...\n", SERVER_IP, PORT, username);
    
    // 3. Conexão
    int resultado = connect(client_sock_fd, (struct sockaddr *)server_address, sizeof(struct sockaddr_in));
    
    if( resultado == 0) {
        printf("Conectado com sucesso. Enviando nome...\n");
        
        // --- NOVA FUNCIONALIDADE: Enviar o Nome para o Servidor ---
        // A primeira coisa que o cliente envia é o seu nome.
        if (send(client_sock_fd, username, strlen(username), 0) < 0) {
            perror("Falha ao enviar nome inicial");
            free(server_address);
            close(client_sock_fd);
            return EXIT_FAILURE;
        }
    } else { 
        perror("Falha ao conectar");
        free(server_address);
        close(client_sock_fd);
        return EXIT_FAILURE;
    }
    
    // 4. Criação das Threads
    pthread_t send_thread, receive_thread;
    
    if (pthread_create(&receive_thread, NULL, receive_messages, NULL) != 0) {
        perror("Não foi possível criar a thread de recebimento");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&send_thread, NULL, send_messages, NULL) != 0) {
        perror("Não foi possível criar a thread de envio");
        exit(EXIT_FAILURE);
    }
    
    // Espera que a thread de envio termine
    pthread_join(send_thread, NULL);
    
    // 5. Limpeza Final
    free(server_address);
    close(client_sock_fd);
    
    return 0;
}