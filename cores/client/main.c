#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(){
    int sock;
    char buffer[1024];
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
        printf("Enter message: ");
        fgets(buffer, 1024, stdin);

        buffer[strcspn(buffer, "\n")] = '\0';

        if (strcmp(buffer, "quit") == 0) {
            break;
        }


        sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr*)&server, sizeof(server));

        int len = recvfrom(sock, buffer, sizeof(buffer)-1, 0, NULL, NULL);
        if (len > 0) {
            buffer[len] = '\0';
            printf("Server replied: %s\n", buffer);
        }
    }

    close(sock);
    return 0;
}
