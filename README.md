# ChatRoom-in-C
Este projeto é uma implementação robusta de um sistema de chat Cliente-Servidor baseado no protocolo TCP/IP, utilizando programação multithreaded (pthread) para lidar com múltiplos utilizadores concorrentemente.

O código foi desenvolvido com base nos princípios de modularidade, separando as funcionalidades de rede em um módulo (networking.c) e garantindo a estabilidade da memória com alocação dinâmica (malloc).

## Arquitetura do Projeto

O projeto é dividido em três módulos principais:

| Ficheiro | Tipo | Função |
| :--- | :--- | :--- |
| `networking.h` / `networking.c` | **Biblioteca** | Lógica de infraestrutura. Contém funções essenciais como `createTCPIpv4Socket` e `createIpv4Address` (com suporte a `INADDR_ANY`) e bibliotecas que permitem a compilação de executavel .exe para windows. |
| `server.c` | **Servidor** | Ouve na porta `6969`. Usa threads para lidar com cada cliente (o assistente) e faz o broadcast de mensagens para todos os participantes, exceto o remetente. |
| `client.c` | **Cliente** | Conecta-se ao servidor. Usa **duas threads** (`send_messages` e `receive_messages`) para permitir que o utilizador digite e receba mensagens ao mesmo tempo, garantindo que a própria mensagem seja exibida instantaneamente. |

## Requisitos 

O projeto requer o compilador GCC e a biblioteca `ptreads` (para threads).

## Compilação no Linux. 

Execute os seguintes comandos no terminal. É crucial listar todos os ficheiros de implementação (.c) e usar a flag -pthread para o multithreading:

 ```bash
   # Compila o Servidor, ligando a biblioteca de rede e threads
gcc server.c networking.c -o server -Wall -pthread
   
   # Compila o Cliente, ligando a biblioteca de rede e threads
gcc client.c networking.c -o client -Wall -pthread
   ```
## Execução 

Depois de compilar tanto o servidor como o cliente para rodar aplicação digite execute em terminais diferentes o comando 

```bash 
./server
```

```bash 
./client
```

note que para a conexão funcionar ambos o cliente e servidor tem de estar na mesma porta.

## Compatibilidade Windows 

para executar o programa em ambiente windows basta executar o client.exe e server.exe no terminal powershell
   
## Como Rodar Online (Port Forwarding)

Para que haja uma conexão online , é necessário configurar o roteador:

1. Preparação da Rede

IP Interno Estático: Certifique-se de que o IP local do seu PC (`consultavel pelo comando "ipconfig" `) é estático (recomendado usar Reserva DHCP no roteador) para evitar que a regra de encaminhamento quebre.

Porta: A porta utilizada é a 6969 no protocolo TCP também pode ser utilizado qualquer porta valida de sua preferencia (qualquer porta no intervalo 1024 - 65.535 alterando na linha `#define PORT` )

2. Configuração do Roteador

Aceda à interface de administração do seu roteador (geralmente 192.168.1.1 tambem se pode achar atras do roteador junto com credenciais de acesso).

Localize a secção Port Forwarding (ou NAT/Virtual Servers).

Crie uma nova regra de encaminhamento com os seguintes dados:

| Parâmetro | Valor |
| :--- | :--- |
| **Protocolo** | **TCP** |
| **Porta Externa (WAN Port)** | `6969 ou qualquer porta de sua preferencia` |
| **Porta Interna (LAN Port)** | `6969 ou qualquer porta de sua preferencia` |
| **Endereço IP Interno (Target)** | `O seu IP` |

 3. Conexão do Utilizador Remoto

O utilizador remoto deve:

Obter o seu IP Público (pesquisando no Google "Qual é o meu IP").

Alterar a linha no seu ficheiro client.c para o seu IP público: #define SERVER_IP "SEU_IP_PUBLICO_AQUI"

Compilar e executar o cliente.
