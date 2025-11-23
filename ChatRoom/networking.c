#include "networking.h"
#include <stdio.h>
#include <stdlib.h> // Para malloc, free
#include <string.h> // Para memset, strcmp


/**
 * Cria o File Descriptor do socket (AF_INET, SOCK_STREAM).
 * @return O File Descriptor do socket ou -1 em caso de erro.
 */
int createTCPIpv4Socket() { 
    return socket(AF_INET, SOCK_STREAM, 0); 
}

/**
 * Cria e configura a estrutura sockaddr_in no heap.
 *
 * Se 'ip' for NULL ou "0.0.0.0", configura para INADDR_ANY (modo servidor).
 *
 * @param ip A string do endereço IP.
 * @param port O número da porta.
 * @return Um ponteiro para a estrutura sockaddr_in alocada ou NULL em caso de erro.
 */
struct sockaddr_in* createIpv4Address (const char *ip, int port){
    
    struct sockaddr_in *address = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    
    if (address == NULL) {
        perror("Falha na alocação de memória (malloc)");
        return NULL;
    }
    
    memset(address, 0, sizeof(struct sockaddr_in)); 

    address->sin_port = htons(port); 
    address->sin_family = AF_INET;
    
    // Lógica para INADDR_ANY (modo servidor)
    if (ip == NULL || strcmp(ip, "0.0.0.0") == 0 || strcmp(ip, "") == 0) {
        address->sin_addr.s_addr = INADDR_ANY;
    } else {
        // Lógica para IP específico (modo cliente)
        if (inet_pton(AF_INET, ip, &address->sin_addr) <= 0) {
            fprintf(stderr, "Erro ao converter IP: %s\n", ip);
            free(address); 
            return NULL;
        }
    }

    return address;
}