#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "connection.h"
#include "message.h"

int main(int argc , char *argv[]){
    if (argc != 3) {
        perror("Require server IP address and port as argument");
        exit(EXIT_FAILURE);
    }
    int sock;
    struct message msg;
    struct sockaddr_in server;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(atoi(argv[2]));

    // Perform handshake with server
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

    while(1){
        char fileName[1024];
        memset(&msg , 0 , sizeof(msg));
        printf("Enter message: ");
        if(!fgets(fileName, sizeof(msg.data), stdin)){
            break;
        }

        fileName[strcspn(fileName, "\n")] = '\0';
        HDR_SET_ACK(msg.flags , HDR_ACK_NONE);

        if (strcmp(msg.data, "quit") == 0) {
            break;
        }

        //if(sendto(sock, &msg, sizeof(msg), 0, (struct sockaddr*)&server, sizeof(server)) < 0){
        //    perror("sendto failed");
        //    break;
        //}

        //int len = recvfrom(sock, &msg, sizeof(msg), 0, NULL, NULL);
        //if (len > 0 && HDR_GET_ACK(msg.flags) == HDR_ACK_ACK){
        //    printf("ACK!\n");
        //}

        // After receive ACK from server start send file name to request file from server
        request_file(fileName, sock, server);
        
    }

    close(sock);
    return 0;
}
