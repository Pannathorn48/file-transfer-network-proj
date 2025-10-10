#include "standard_lib.h"
#include "network_lib.h"
#include "connection.h"
#include "message.h"
#include "checksum.h"


int main(int argc , char *argv[]){
    #if defined(_WIN32) || defined(_WIN64)
    WSADATA wsa_data;
    int wsa_initialized = 0;
    if (WSAStartup(MAKEWORD(2,2), &wsa_data) != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }
    wsa_initialized = 1;
    #endif

    if (argc != 3) {
        perror("Require server IP address and port as argument");
        exit(EXIT_FAILURE);
    }
    int sock;
    struct message msg;
    struct sockaddr_in server, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(atoi(argv[2]));

    #if defined(_WIN32) || defined(_WIN64)
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("Connect failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    #endif

    // Get the client's local IP address for checksum calculation
    if (getsockname(sock, (struct sockaddr*)&client_addr, &client_addr_len) < 0) {
        perror("getsockname failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

   

    while(1){
         // Perform handshake with server
        
        char fileName[1024];
        printf("Enter filename to request (or 'quit' to exit): ");
        if (!fgets(fileName, sizeof(fileName), stdin)) {
            break;
        }

        fileName[strcspn(fileName, "\n")] = '\0';

        if (strcmp(fileName, "quit") == 0) {
            break;
        }

        int handshake_status = client_handle_handshake(sock, &msg, server);
        if (handshake_status != 0) {
            switch (handshake_status)
            {
            case ERR_SEND_FAIL:
                fprintf(stderr, "Error: Failed to send handshake message\n");
                break;
            case ERR_RECV_FAIL:
                fprintf(stderr, "Error: Failed to receive handshake response\n");
                break;
            case ERR_INVALID_HANDSHAKE:
                fprintf(stderr, "Error: Invalid handshake response\n");
                break;
            default:
                fprintf(stderr, "Error: Unknown handshake error\n");
                break;
            }
            close(sock);
            exit(EXIT_FAILURE);
        }

        // Call the updated request_file function with the correct arguments
        request_file(fileName, sock, server);
    }

    close(sock);
    #if defined(_WIN32) || defined(_WIN64)
        WSACleanup();
    #endif
    return 0;
}