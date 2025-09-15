#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "connection.h"

int send_file(char filePath[], int sock, struct sockaddr_in client)
{
    FILE *fp;
    struct message msg;
    unsigned int client_len = sizeof(client);

    fp = fopen(filePath, "rb");

    printf("Request file: %s\n", filePath);

    if (fp == NULL)
    {
        printf("File not found: %s\n", filePath);
        memset(&msg, 0, sizeof(msg));
        msg.header_flags |= HDR_ERR_FNF;
        msg.data_length = 0;
        if (sendto(sock, &msg, sizeof(msg), 0, (struct sockaddr *)&client, client_len) < 0)
        {
            perror("sendto failed");
        }
        return 1;
    }
    else
    {
        while (1)
        {
            memset(&msg, 0, sizeof(msg));
            int bytes_read = fread(msg.data, 1, sizeof(msg.data), fp);

            if (bytes_read > 0)
            {
                HDR_SET_ACK(msg.header_flags, HDR_ACK_NONE);
                msg.data_length = bytes_read;
                if (sendto(sock, &msg, sizeof(msg), 0, (struct sockaddr *)&client, client_len) < 0)
                {
                    perror("sendto failed");
                    break;
                }
            }

            if (bytes_read < (int)sizeof(msg.data))
            {
                memset(&msg, 0, sizeof(msg));
                HDR_SET_FIN(msg.header_flags);
                msg.data_length = 0;
                if (sendto(sock, &msg, sizeof(msg), 0, (struct sockaddr *)&client, client_len) < 0)
                {
                    perror("sendto failed");
                    break;
                }
                break;
            }
        }
        fclose(fp);

        int len = recvfrom(sock, &msg, sizeof(msg), 0, (struct sockaddr *)&client, &client_len);
        if (len < 0)
        {
            perror("recvfrom failed");
        }

        if (HDR_GET_ACK(msg.header_flags) & HDR_ACK_ACK)
        {
            printf("Successfully send the file to the client\n");
        }
    }
    return 0;
}
int request_file(char fileName[], int sock, struct sockaddr_in server)
{
    FILE *fp;
    struct message msg;

    memset(&msg, 0, sizeof(msg));

    char serverPath[1024] = "cores/server/";
    strncat(serverPath, fileName, sizeof(serverPath) - strlen(serverPath) - 1);
    strncpy(msg.data, serverPath, sizeof(msg.data));

    HDR_SET_ACK(msg.header_flags, HDR_ACK_NONE);
    msg.data_length = strlen(serverPath);

    if (sendto(sock, &msg, sizeof(msg), 0, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("sendto failed");
        return 1;
    }

    char clientPath[1024] = "cores/client/";
    strncat(clientPath, fileName, sizeof(clientPath) - strlen(clientPath) - 1);

    fp = fopen(clientPath, "wb");
    if (fp == NULL)
    {
        perror("Failed to create file");
        return 1;
        // close(sock);
        // exit(EXIT_FAILURE);
    }

    while (1)
    {
        memset(&msg, 0, sizeof(msg));
        int len = recvfrom(sock, &msg, sizeof(msg), 0, NULL, NULL);
        if (len < 0)
        {
            perror("recvfrom failed");
            continue;
        }
        if (!((msg.header_flags & HDR_ERR_MASK) ^ HDR_ERR_FNF))
        {
            memset(&msg, 0, sizeof(msg));
            HDR_SET_ACK(msg.header_flags, HDR_ACK_ACK);
            msg.data_length = 0;
            sendto(sock, &msg, sizeof(msg), 0,
                   (struct sockaddr *)&server, sizeof(server));
            printf("Failed received the file from the server, File not found\n");
            remove(clientPath);
            break;
        }
        fwrite(msg.data, 1, msg.data_length, fp);

        if (HDR_GET_FIN(msg.header_flags))
        {
            memset(&msg, 0, sizeof(msg));
            HDR_SET_ACK(msg.header_flags, HDR_ACK_ACK);
            msg.data_length = 0;
            sendto(sock, &msg, sizeof(msg), 0,
                   (struct sockaddr *)&server, sizeof(server));
            printf("Successfully received the file from the server\n");
            break;
        }
    }
    fclose(fp);
    return 0;
}
