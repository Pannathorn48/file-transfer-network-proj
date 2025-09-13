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



int main(){
    int sock;
    struct message msg;
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
        memset(&msg, 0, sizeof(msg));
        int len = recvfrom(sock, &msg, sizeof(msg), 0, (struct sockaddr*)&client, &client_len);
        if(len < 0){
            perror("recvfrom failed");
            continue;
        }

        msg.data[sizeof(msg.data)-1] = '\0';
        printf("Received: %s\n" , msg.data);

        msg.flags = (msg.flags & TYPE_MARKS) |  MSG_ACK;
        msg.data[0] = '\0';

        if(sendto(sock ,  &msg, sizeof(msg), 0 , (struct sockaddr*)&client , client_len) < 0){
            perror("sendto failed");
        }

        // Waiting for client send request file
        int fileNameRequestlen = recvfrom(sock, &msg, sizeof(msg), 0, (struct sockaddr*)&client, &client_len);
        if(fileNameRequestlen < 0){
            perror("recvfrom failed");
            continue;
        }

        if (send_file(msg.data, sock, client) > 0) {
           memset(&msg, 0, sizeof(msg));
            msg.flags = MSG_ERR;
            msg.data_length = 0;
            if (sendto(sock, &msg, sizeof(msg), 0, (struct sockaddr *)&client, client_len) < 0)
            {
                perror("sendto failed");
            }
            continue; 
        }
        
    }

    return 0;



}
