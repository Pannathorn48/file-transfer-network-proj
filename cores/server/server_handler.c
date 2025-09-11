#ifndef NETWORK_HEADERS_H
#define NETWORK_HEADERS_H

#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include <sys/_types/_socklen_t.h>
#include <unistd.h>
#include <arpa/inet.h>
//#include <sys/_endian.h>
#include <sys/socket.h>
#include "connection.h"
#include "error.h"

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
