#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "connection.h"
#include "message.h"



int send_file(Message *msg, int sock, struct sockaddr_in client)
{
    unsigned int client_len = sizeof(client);

    // fp = fopen(filePath, "rb");

    // printf("Request file: %s\n", msg->data);

    // int bytes_read = fread(msg.data, 1, sizeof(msg.data), fp);
    printf("MESSAGE FLAG: %x\n", msg->flags);
    if (HDR_GET_META(msg->flags))
    {
        printf("SEND META FLAG \n");
        // if (sendto(sock, msg, sizeof(msg), 0, (struct sockaddr *)&client, client_len) < 0)
        // {
        //     perror("sendto failed");
        //     return 1;
        // }
    }
    else if (msg->data_length < 1024)
    {
        printf("SEND FIN FLAG \n");
        HDR_SET_FIN(msg->flags);
        // if (sendto(sock, msg, sizeof(msg), 0, (struct sockaddr *)&client, client_len) < 0)
        // {
        //     perror("sendto failed");
        //     return 1;
        // }
    }
    else
    {
        printf("SEND DATA FLAG \n");
        HDR_SET_ACK(msg->flags, HDR_ACK_NONE);
    }
    
    if (sendto(sock, msg, sizeof(msg), 0, (struct sockaddr *)&client, client_len) < 0)
    {
        perror("sendto failed");
        return 1;
    }

    int len = recvfrom(sock, msg, sizeof(msg), 0, (struct sockaddr *)&client, &client_len);
    if (len < 0)
    {
        perror("recvfrom failed");
        return 1;
    }

    if (!(HDR_GET_ACK(msg->flags) ^ HDR_ACK_ACK))
    {
        printf("Successfully send the file to the client\n");
    }
    return 0;
}
int request_file(char fileName[], int sock, struct sockaddr_in server)
{
    struct message msg;

    memset(&msg, 0, sizeof(msg));

    // char serverPath[1024] = "cores/server/";
    // strncat(serverPath, fileName, sizeof(serverPath) - strlen(serverPath) - 1);

    strncpy(msg.data, fileName, sizeof(msg.data));

    HDR_SET_ACK(msg.flags, HDR_ACK_NONE);
    msg.data_length = strlen(fileName);

    printf("REQUEST: %s\n", msg.data);
    if (sendto(sock, &msg, sizeof(msg), 0, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("sendto failed");
        return 1;
    }

    // char clientPath[1024] = "cores/client/";
    // strncat(clientPath, fileName, sizeof(clientPath) - strlen(clientPath) - 1);

    // fp = fopen(clientPath, "wb");
    // if (fp == NULL)
    // {
    //     perror("Failed to create file");
    //     return 1;
    //     // close(sock);
    //     // exit(EXIT_FAILURE);
    // }

    while (1)
    {
        Message* newMsg = (Message*)malloc(sizeof(Message));
        int len = recvfrom(sock, newMsg, sizeof(newMsg), 0, NULL, NULL);
        // *newMsg = msg;
        printf("DATA FLAG: %x\n", newMsg->flags);
        if (len < 0)
        {
            perror("recvfrom failed");
            continue;
        }

        if (HDR_GET_META(newMsg->flags))
        {
            printf("RECEIVE META DATA \n");
            sync_message_queue(newMsg);
        }
        else
        {
            printf("RECEIVE DATA \n");
            enqueue_message(receiverMessages, newMsg);
        }

        if (HDR_GET_FIN(newMsg->flags))
        {
            printf("END \n");
            break;
        }
        memset(&msg, 0, sizeof(msg));
        HDR_SET_ACK(msg.flags, HDR_ACK_ACK);
        msg.data_length = 0;
        sendto(sock, &msg, sizeof(msg), 0,
               (struct sockaddr *)&server, sizeof(server));
        printf("Successfully received the file from the server\n");
    }
    assemble_file();
    // fclose(fp);
    return 0;
}
