#ifndef NETWORKING_H
#define NETWORKING_H

// --- LÓGICA DE COMPILAÇÃO CRUZADA (Windows vs. Linux) ---

#ifdef _WIN32
    // Se estiver a compilar para Windows (usando MinGW)
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    // #pragma comment(lib, "ws2_32.lib") // REMOVIDO: Linha desnecessária para MinGW que gera aviso.
    #define close closesocket
#else
    // Se estiver a compilar para Linux (usando GCC)
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

// --- Fim da Lógica Condicional ---


/**
 * Cria o File Descriptor do socket (AF_INET, SOCK_STREAM).
 * @return O File Descriptor do socket ou -1 em caso de erro.
 */
int createTCPIpv4Socket();

/**
 * Cria e configura a estrutura sockaddr_in no heap.
 *
 * NOTA: A memória alocada por malloc deve ser liberada (free()) pelo chamador.
 *
 * @param ip A string do endereço IP.
 * @param port O número da porta.
 * @return Um ponteiro para a estrutura sockaddr_in alocada ou NULL em caso de erro.
 */
struct sockaddr_in* createIpv4Address (const char *ip, int port);

#endif // NETWORKING_H