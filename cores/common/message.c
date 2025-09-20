// #include "../../headers/message.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "connection.h"
#include "message.h"

typedef struct message Message;

Message *senderMessage = NULL;
Message *receiverMessage = NULL;

void segment_file(const char *filename, int socket, struct sockaddr_in client)
{
    char buffer[BUFFER_SIZE];
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        perror("Failed to open file");
        return;
    }

    // metadata->flags = 0x10000000; // Metadata flag
    if (senderMessage == NULL)
    {
        senderMessage = (Message *)malloc(sizeof(Message));
        if (!senderMessage)
        {
            perror("Failed to allocate memory for message");
            fclose(file);
            return;
        }
    }
    HDR_SET_META(senderMessage->flags);

    sprintf(senderMessage->data, "[c]%s", filename);
    send_file(senderMessage, socket, client);

    size_t bytesRead;
    uint32_t seqNum = 0x0;

    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0)
    {
        HDR_SET_SEQ(senderMessage->flags, seqNum);
        memcpy(senderMessage->data, buffer, bytesRead);
        senderMessage->data_length = bytesRead;
        send_file(senderMessage, socket, client);
        seqNum++;
    }

    fclose(file);
}


int send_file(Message *msg, int sock, struct sockaddr_in client)
{
    unsigned int client_len = sizeof(client);

    printf("MESSAGE FLAG: %x\n", msg->flags);
    if (HDR_GET_META(msg->flags))
    {
        printf("SEND META FLAG \n");
    }
    else if (msg->data_length < 1024)
    {
        printf("SEND FIN FLAG \n");
        HDR_SET_FIN(msg->flags);
    }
    else
    {
        printf("SEND DATA FLAG \n");
        HDR_SET_ACK(msg->flags, HDR_ACK_NONE);
    }

    if (sendto(sock, msg, sizeof(*msg), 0, (struct sockaddr *)&client, client_len) < 0)
    {
        perror("sendto failed");
        return 1;
    }

    int len = recvfrom(sock, msg, sizeof(*msg), 0, (struct sockaddr *)&client, &client_len);
    if (len < 0)
    {
        perror("recvfrom failed");
        return 1;
    }

    if (!(HDR_GET_ACK(msg->flags) ^ HDR_ACK_ACK))
    {
        printf("Successfully send the file to the client\n");
    }
    printf("%x\n", msg->flags);
    return 0;
}

int request_file(char fileName[], int sock, struct sockaddr_in server)
{
    struct message msg;

    memset(&msg, 0, sizeof(msg));

    strncpy(msg.data, fileName, sizeof(msg.data));

    HDR_SET_ACK(msg.flags, HDR_ACK_NONE);
    msg.data_length = strlen(fileName);

    printf("REQUEST: %s\n", msg.data);
    if (sendto(sock, &msg, sizeof(msg), 0, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("sendto failed");
        return 1;
    }

    if (receiverMessage == NULL)
    {
        receiverMessage = (Message *)malloc(sizeof(Message));
    }

    int len = recvfrom(sock, receiverMessage, sizeof(*receiverMessage), 0, NULL, NULL);

    printf("DATA FLAG: %x\n", receiverMessage->flags);
    if (len < 0)
    {
        perror("recvfrom failed");
        return 1;
    }

    if (!receiverMessage || !(HDR_GET_META(receiverMessage->flags)))
    {
        perror("Invalid metadata message for assembling file");
        return 1;
    }

    printf("RECEIVE META DATA \n");
    FILE *file = fopen(receiverMessage->data, "ab");
    if (!file)
    {
        perror("Failed to open file for writing");
        return 1;
    }
    // printf("Still good?\n");
    memset(receiverMessage, 0, sizeof(*receiverMessage));
    // printf("Still good? Hello?\n");
    HDR_SET_ACK(msg.flags, HDR_ACK_ACK);
    // printf("Still good? Hello? Answer me.\n");
    msg.data_length = 0;
    sendto(sock, &msg, sizeof(msg), 0,
           (struct sockaddr *)&server, sizeof(server));
    // printf("Still good? Hello? Answer me. Please Please?\n");

    while (1)
    {
        // printf("Still good? Hello? Answer me. Please Please? TALK TO ME!\n");
        len = recvfrom(sock, receiverMessage, sizeof(*receiverMessage), 0, NULL, NULL);
        // printf("Still good? Hello? Answer me. Please Please? TALK TO ME! recvfrom??\n");
        // *receiverMessage = msg;
        // printf("DATA FLAG: %x\n", receiverMessage->flags);
        if (len < 0)
        {
            perror("recvfrom failed");
            continue;
        }

        printf("RECEIVE DATA \n");
        fwrite(receiverMessage->data, 1, receiverMessage->data_length, file);

        if (HDR_GET_FIN(receiverMessage->flags))
        {
            printf("END \n");
            break;
        }

        memset(&msg, 0, sizeof(msg));
        memset(receiverMessage, 0, sizeof(*receiverMessage));
        HDR_SET_ACK(msg.flags, HDR_ACK_ACK);
        msg.data_length = 0;
        sendto(sock, &msg, sizeof(msg), 0,
               (struct sockaddr *)&server, sizeof(server));
    }
    HDR_SET_ACK(msg.flags, HDR_ACK_ACK);
    msg.data_length = 0;
    sendto(sock, &msg, sizeof(msg), 0,
           (struct sockaddr *)&server, sizeof(server));
    fclose(file);
    printf("Successfully received the file from the server\n");
    printf("File %s reassembled successfully.\n", fileName);
    return 0;
}
