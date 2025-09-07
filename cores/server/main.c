#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/_endian.h>
#include <sys/socket.h>
#include "../../headers/connection.h"
#include "../../headers/error.h"

int main(){
    int sock;
    char buffer[1024];
    struct sockaddr_in server , client;
    unsigned int client_len = sizeof(client);

    // handle create new connection
    sock = new_connection("8080", &server);
    switch (sock) {
        case ERR_SOCKET_FAIL:{
            perror("Error : failed to create socket");
            exit(EXIT_FAILURE);
        }
        case ERR_INVALID_PORT:{
            perror("Error : port is invalid");
            exit(EXIT_FAILURE);
        }
        case ERR_PORT_IN_USE:{
            perror("Error : port is in used");
            exit(EXIT_FAILURE);
        }
    }


    while(1){
        int len = recvfrom(sock, buffer, 1024, 0, (struct sockaddr*)&client, &client_len);
        buffer[len] = '\0';
        printf("Received: %s\n" , buffer);

        sendto(sock , buffer , len , 0 , (struct sockaddr*)&client , client_len);
    }

    return 0;



}
