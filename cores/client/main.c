#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "connection.h"
#include "message.h"
#include "checksum.h"
#include <sys/types.h>

int status = CLIENT_NOT_CONN;

int main(int argc , char *argv[]){

    setup_connection_timeout_status(&status, 30);
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

    // Get the client's local IP address for checksum calculation
    if (getsockname(sock, (struct sockaddr*)&client_addr, &client_addr_len) < 0) {
        perror("getsockname failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

   

    while(1){
         // Perform handshake with server
        int handshake_status = client_handle_handshake(&status, sock, &msg, server);
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
        char fileName[1024];
        printf("Enter filename to request (or 'quit' to exit): ");
        if (!fgets(fileName, sizeof(fileName), stdin)) {
            break;
        }

        fileName[strcspn(fileName, "\n")] = '\0';

        if (strcmp(fileName, "quit") == 0) {
            break;
        }

        // Call the updated request_file function with the correct arguments
        request_file(fileName, sock, server, client_addr);
    }

    close(sock);
    return 0;
}