#ifndef NETWORK_HEADERS_H
#define NETWORK_HEADERS_H

#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "connection.h"
#include "error.h"
#include "checksum.h"


#endif /* NETWORK_HEADERS_H */

int new_connection(char* port , struct sockaddr_in* server){
    int port_number = atoi(port);
    if (port_number == 0 || port_number > 65535 || port_number < 0){
        return ERR_INVALID_PORT;
    }

    int sock = socket(AF_INET, SOCK_DGRAM , 0);

    if (sock < 0){
        perror("Socket creation failed");
        return ERR_SOCKET_FAIL;
    }

    memset(server, 0, sizeof(*server));
    server->sin_family = AF_INET;
    server->sin_addr.s_addr = INADDR_ANY;
    server->sin_port = htons(port_number);

    if(bind(sock, (struct sockaddr*)server, sizeof(*server)) < 0){
        perror("Bind failed");
        close(sock);
        return ERR_PORT_IN_USE;
    }

    printf("Server is listening on port %d ...\n", port_number);
    return sock;
}

int server_handle_handshake(int sock, struct message *msg , struct sockaddr_in client){
    unsigned int client_len = sizeof(client);
    // Send META-ACK
    memset(msg, 0, sizeof(*msg));
    msg->data_length = 0;
    HDR_SET_ACK(msg->flags, HDR_ACK_ACK);
    HDR_SET_META(msg->flags);
    HDR_SET_SEQ(msg->flags, 0); // Initial sequence number

    set_message_checksum(msg); 

    size_t handshake_size = sizeof(msg->checksum) + sizeof(msg->flags);

    if (sendto(sock, msg, handshake_size, 0, (struct sockaddr *)&client, client_len) < 0) {
        perror("sendto failed");
        return ERR_SEND_FAIL;
    }

    // Set 30 sec timeout for handshake
    struct timeval tv;
    tv.tv_sec = 30;
    tv.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt failed");
        return ERR_SOCKET_FAIL;
    }

    // Wait for ACK
    memset(msg, 0, sizeof(*msg));
    int recv_len = recvfrom(sock, msg, sizeof(*msg), 0, (struct sockaddr *)&client, &client_len);
    if (recv_len < 0) {
        perror("recvfrom failed or timeout");
        return ERR_RECV_FAIL;
    }

    // Restore socket to blocking mode
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    if (HDR_GET_ACK(msg->flags) != HDR_ACK_ACK || HDR_GET_SEQ(msg->flags) != 1) {
        fprintf(stderr, "Invalid ACK received during handshake\n");
        return ERR_INVALID_HANDSHAKE;
    }

    printf("Handshake completed with client %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
    return 0;
}
