#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "connection.h"
#include "error.h"
#include <stdbool.h>
#include "request_send_file.h"
#include "message.h"

int main(int argc , char *argv[]) {
    if (argc != 2){
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int sock;
    struct message msg;
    struct sockaddr_in server, client;
    unsigned int client_len = sizeof(client);

    // handle create new connection
    sock = new_connection(argv[1], &server);
    switch (sock)
    {
    case ERR_SOCKET_FAIL:
    {
        perror("Error : failed to create socket");
        exit(EXIT_FAILURE);
    }
    case ERR_INVALID_PORT:
    {
        perror("Error : port is invalid");
        exit(EXIT_FAILURE);
    }
    case ERR_PORT_IN_USE:
    {
        perror("Error : port is in used");
        exit(EXIT_FAILURE);
    }
    }


    while (1)
    {
        memset(&msg, 0, sizeof(msg));
        int fileNameRequestlen = recvfrom(sock, &msg, sizeof(msg), 0, (struct sockaddr *)&client, &client_len);
        if (fileNameRequestlen < 0) {
            perror("recvfrom failed");
            continue;
        }

        if (HDR_GET_META(msg.flags) && (msg.data_length == 0)) {
            printf("META received, sending META-ACK...\n");
            int handshake_status = server_handle_handshake(sock, &msg, client);
            printf("Handshake status: %d\n", handshake_status);
            if (handshake_status < 0) {
                switch (handshake_status){
                case ERR_SEND_FAIL:
                    perror("Error : failed to send handshake ACK\n");
                    continue;
                case ERR_SOCKET_FAIL:
                    perror("Error : socket error during handshake\n");
                    continue;
                case ERR_RECV_FAIL:
                    perror("Error : failed to receive handshake ACK\n");
                    continue;
                case ERR_INVALID_HANDSHAKE:
                    perror("Error : invalid handshake packet\n");
                    continue;
                }
            }
        } else if (msg.data_length > 0) {
            // Handle file request - for now just print the filename
            printf("File requested: %.*s\n", msg.data_length, msg.data);
            // TODO: Implement segment_file function
            // segment_file(msg.data, sock, client);
        } else {
            perror("Invalid request - no data");
            continue;
        }
    }

    return 0;
}