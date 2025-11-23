#include <stdio.h>
#include <stdlib.h>    
#include <unistd.h>    
#include <string.h>    
#include <pthread.h>   
// LINHAS REMOVIDAS: As bibliotecas de rede são agora incluídas via networking.h
// #include <arpa/inet.hção
#include "networking.h" 

#define BUFFER_SIZE 2048
// --- IP E PORTA DEFINIDOS PELO USUÁRIO ---
#define SERVER_IP "127.0.0.0"  //defini o server ip para localHost para testes locais.para implementação online consultar README.  
#define PORT 6969 // escolhi esse valor arbitrariamente NICE :) 
// ---------------------------------------------

int client_sock_fd = 0; 
char username[25] = {0}; 
// Variável global para armazenar o prompt atual e evitar que ele se perca
char current_input_buffer[BUFFER_SIZE] = {0}; 

void *receive_messages(void *arg) {
    char buffer[BUFFER_SIZE];
    int read_size;

    while (1) {
        read_size = recv(client_sock_fd, buffer, BUFFER_SIZE - 1, 0);

        if (read_size > 0) {
            buffer[read_size] = '\0';
        
            // 1. Volta para o início da linha e limpa até o fim (escondendo a entrada atual)
            printf("\r\033[K%s\n", buffer); 
            
            // 2. Imprime o prompt novamente e reimprime o buffer de entrada (se houver)
            printf("> %s", current_input_buffer);
            
            fflush(stdout);
        } else if (read_size == 0) {
            printf("\n[ALERTA] Servidor desconectou.\n");
            exit(0); 
        } else if (read_size == -1) {
            perror("Recv falhou");
            exit(1); 
        }
        memset(buffer, 0, BUFFER_SIZE);
    }
    pthread_exit(NULL);
    return NULL;
}

void *send_messages(void *arg) {
    
    printf("\n--- Cliente de Chat C ---\n");
    printf("Entre na sala de chat!\n");
    printf("Digite as mensagens (ou 'exit' para sair):\n");

    while (1) {
        // --- PROMPT ÚNICO E LEITURA ---
        // Aqui, current_input_buffer atua como o nosso buffer de entrada.
        printf("> ");
        fflush(stdout); 
        
        if (fgets(current_input_buffer, BUFFER_SIZE, stdin) == NULL) {
            break; 
        }
        
        // CORREÇÃO DE SEGURANÇA: Remover nova linha (\n) do final do buffer antes de enviar
        size_t len = strlen(current_input_buffer);
        if (len > 0 && current_input_buffer[len - 1] == '\n') {
            current_input_buffer[len - 1] = '\0';
        }
        
        // Saída do loop se o buffer estiver vazio (apenas Enter)
        if (current_input_buffer[0] == '\0') {
            continue;
        }

        // NOVA FUNCIONALIDADE: Visualização da Própria Mensagem
        // Imprime a sua mensagem e uma nova linha para que o próximo prompt comece bem.
        printf("[%s]: %s\n", username, current_input_buffer); 

        // O send usa o buffer já limpo
        if (send(client_sock_fd, current_input_buffer, strlen(current_input_buffer), 0) < 0) {
            perror("Send falhou");
            break; 
        }

        if (strncmp(current_input_buffer, "exit", 4) == 0) {
            printf("[ALERTA] Desconectando...\n");
            break; 
        }
        
        // Limpa o buffer de entrada após o envio bem-sucedido
        memset(current_input_buffer, 0, BUFFER_SIZE);
    }
    // CORREÇÃO DO AVISO
    pthread_exit(NULL);
    return NULL; 
}

int main (){
    
    //  INICIALIZAÇÃO OBRIGATÓRIA DO WINSOCK PARA WINDOWS ---
    #ifdef _WIN32
        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
            fprintf(stderr, "WSAStartup falhou. Código de erro: %d\n", WSAGetLastError());
            return EXIT_FAILURE;
        }
    #endif
    // --- FIM DA INICIALIZAÇÃO WINSOCK ---

    printf("Digite seu nome (máx. 24 caracteres): ");
    char temp_name[25];
    // strcspn é do string.h
    if (fgets(temp_name, sizeof(temp_name), stdin) != NULL) {
        // CORREÇÃO DE SEGURANÇA: Usar strcspn para remover nova linha (\n)
        temp_name[strcspn(temp_name, "\n")] = 0;
        strncpy(username, temp_name, sizeof(username) - 1);
        username[sizeof(username) - 1] = '\0';
    } else {
        fprintf(stderr, "Erro ao ler o nome.\n");
        return EXIT_FAILURE;
    }
    
    // 1. Criação do Socket
    client_sock_fd = createTCPIpv4Socket();
    
    if (client_sock_fd < 0) {
        perror("Falha ao criar o socket");
        // O cliente fechará aqui se a criação do socket falhar.
        return EXIT_FAILURE;
    }
    
    // 2. Criação da Estrutura de Endereço (usando a função de rede)
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
        
        // NOVA FUNCIONALIDADE: Enviar o Nome para o Servidor
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

    // --- NOVO: FINALIZAÇÃO DO WINSOCK ---
    #ifdef _WIN32
        WSACleanup();
    #endif
    // --- FIM DA FINALIZAÇÃO ---
    
    return 0;
}
