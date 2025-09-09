#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "connection.h"

int main(){
    int sock;
    struct message msg;
    struct sockaddr_in server;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(8080);

    while(1){
        memset(&msg , 0 , sizeof(msg));
        printf("Enter message: ");
        if(!fgets(msg.data, sizeof(msg.data), stdin)){
            break;
        }

        msg.data[strcspn(msg.data, "\n")] = '\0';
        msg.flags = msg.flags & (MSG_DATA | TYPE_MARKS);

        if (strcmp(msg.data, "quit") == 0) {
            break;
        }

        if(sendto(sock, &msg, sizeof(msg), 0, (struct sockaddr*)&server, sizeof(server)) < 0){
            perror("sendto failed");
            break;
        }

        int len = recvfrom(sock, &msg, sizeof(msg), 0, NULL, NULL);
        if (len > 0 && (msg.flags & (TYPE_MARKS ^ 255)) == MSG_ACK){
            printf("ACK!\n");
        }
    }

    close(sock);
    return 0;
}
