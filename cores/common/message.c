#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "connection.h"
#include "message.h"
#include "checksum.h"

void send_NACK(int sock, struct sockaddr_in dest_addr) {
    struct message msg_NACK;
    memset(&msg_NACK, 0, sizeof(msg_NACK));
    HDR_SET_ACK(msg_NACK.flags, HDR_ACK_NACK);
    msg_NACK.data_length = 0;
    set_message_checksum(&msg_NACK);
    send_message(&msg_NACK, sock, dest_addr);
}

int wait_response_from_client(struct message msg, struct sockaddr_in client, int sock, uint32_t seq) {
    while(1) {
        struct message msg_response;
        memset(&msg_response, 0, sizeof(msg_response));
        socklen_t client_len = sizeof(client);
        int received_len = recvfrom(sock, &msg_response, sizeof(msg_response), 0, (struct sockaddr *)&client, &client_len);
        if (received_len < 0)
        {
            perror("recvfrom failed");
            return 1;
        }
        // Calculate the data length from the received packet size
        msg_response.data_length = received_len - (sizeof(msg_response.checksum) + sizeof(msg_response.flags));

        // Validate the checksum of the incoming request
        while (validate_message_checksum(&msg_response, received_len) != 0)
        {
            send_NACK(sock, client);

            fprintf(stderr, "Checksum ACK NACK validation failed! Packet dropped.\n");
            int received_len = recvfrom(sock, &msg_response, sizeof(msg_response), 0, (struct sockaddr *)&client, &client_len);
            if (received_len < 0)
            {
                perror("recvfrom failed");
                continue;
            }
        }

        while (!(HDR_GET_ACK(msg_response.flags) ^ HDR_ACK_NACK))
        {
            printf("Receive NACK resend packet: %d\n", HDR_GET_SEQ(msg_response.flags));
            send_message(&msg, sock, client);
            memset(&msg_response, 0, sizeof(msg_response));
            int received_len = recvfrom(sock, &msg_response, sizeof(msg_response), 0, (struct sockaddr *)&client, &client_len);
            if (received_len < 0)
            {
                perror("recvfrom failed");
                continue;
            }
        }

        // Handle duplicate ACKs in case repeated ACKs are received.
        if (!(HDR_GET_ACK(msg_response.flags) ^ HDR_ACK_ACK)) {
            printf("Receive ACK sequence number: %d\n", HDR_GET_SEQ(msg_response.flags));
            if (HDR_GET_SEQ(msg_response.flags) < seq) {
                continue;
            }
        }
        return 0;
    }
}

// Helper function to send a message without waiting for an ACK
int send_message(struct message *msg, int sock, struct sockaddr_in dest_addr)
{
    unsigned int dest_len = sizeof(dest_addr);
    size_t packet_size = sizeof(msg->checksum) + sizeof(msg->flags) + msg->data_length;

    printf("Sending packet with flags: %x, size: %zu, sequence number: %d\n", msg->flags, packet_size, HDR_GET_SEQ(msg->flags));

    if (sendto(sock, msg, packet_size, 0, (struct sockaddr *)&dest_addr, dest_len) < 0)
    {
        perror("sendto failed");
        return 1;
    }
    return 0;
}

void segment_file(const char *filename, int sock, struct sockaddr_in client, struct sockaddr_in server)
{
    struct message msg;
    memset(&msg, 0, sizeof(msg));

    char buffer[BUFFER_SIZE];
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        perror("Failed to open file");
        msg.data_length = 0;
        HDR_SET_STATUS(msg.flags, HDR_STATUS_FNF);
        set_message_checksum(&msg);
        send_message(&msg, sock, client);
        return;
    }

    // Send metadata packet
    HDR_SET_META(msg.flags);
    char filename_copy[256];
    sprintf(filename_copy, "[c]%s", filename);
    strcpy(msg.data, filename_copy);
    msg.data_length = strlen(msg.data);
    // printf("Length of meta data: %d\n", msg.data_length);
    set_message_checksum(&msg);
    send_message(&msg, sock, client);

    wait_response_from_client(msg, client, sock, 0);

    size_t bytes_read;
    uint32_t seqNum = 0x0;

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0)
    {
        memset(&msg, 0, sizeof(msg));
        HDR_SET_SEQ(msg.flags, seqNum);
        memcpy(msg.data, buffer, bytes_read);
        msg.data_length = bytes_read;
        // printf("Length of data: %d\n", msg.data_length);
        set_message_checksum(&msg);
        send_message(&msg, sock, client);
        // seqNum++;
        // if (seqNum == 10) {
        //     send_message(&msg, sock, client);
        // }

        // Waiting for client send ACK if client send NACK server will while loop send packet until client send ACK
        if (wait_response_from_client(msg, client, sock, seqNum)) continue;
        

        seqNum++;
    }

    // Send final packet with FIN flag
    memset(&msg, 0, sizeof(msg));
    HDR_SET_SEQ(msg.flags, seqNum);
    HDR_SET_FIN(msg.flags);
    msg.data_length = 0;
    set_message_checksum(&msg);
    send_message(&msg, sock, client);

    wait_response_from_client(msg, client, sock, 0);

    fclose(file);
    printf("File %s segmented and sent.\n", filename);
}

int request_file(char fileName[], int sock, struct sockaddr_in server, struct sockaddr_in client_addr)
{
    struct message msg;
    memset(&msg, 0, sizeof(msg));

    strncpy(msg.data, fileName, sizeof(msg.data));
    msg.data_length = strlen(msg.data);
    HDR_SET_ACK(msg.flags, HDR_ACK_NONE);

    set_message_checksum(&msg);

    printf("REQUEST: %s\n", msg.data);
    send_message(&msg, sock, server);

    struct message received_msg;
    FILE *file = NULL;
    uint32_t lastSEQ = 0x00000000u;

    while (1)
    {
        memset(&received_msg, 0, sizeof(received_msg));
        socklen_t server_len = sizeof(server);
        int len = recvfrom(sock, &received_msg, sizeof(received_msg), 0, (struct sockaddr *)&server, &server_len);

        // Validate the checksum of the incoming request
        received_msg.data_length = len - (sizeof(received_msg.checksum) + sizeof(received_msg.flags));

        while (validate_message_checksum(&received_msg, len) != 0)
        {
            fprintf(stderr, "Checksum validation failed! Packet dropped.\n");
            send_NACK(sock, server);
            // continue;
            int received_len = recvfrom(sock, &received_msg, sizeof(received_msg), 0, (struct sockaddr *)&server, &server_len);
            if (received_len < 0)
            {
                perror("recvfrom failed");
                continue;
            }
        }

        // if (lastSEQ == 20) {
        //     fprintf(stderr, "Checksum validation failed! Packet dropped.\n");
        //     send_NACK(sock, server);
        //     // continue;
        //     int received_len = recvfrom(sock, &received_msg, sizeof(received_msg), 0, (struct sockaddr *)&server, &server_len);
        //     if (received_len < 0)
        //     {
        //         perror("recvfrom failed");
        //         continue;
        //     }
        // }

        // Check if duplicate client send ACK to server
        if (!(HDR_GET_SEQ(received_msg.flags) ^ lastSEQ) && !(HDR_GET_META(received_msg.flags))) {
            struct message msg_ACK;
            memset(&msg_ACK, 0, sizeof(msg_ACK));
            HDR_SET_SEQ(msg_ACK.flags, lastSEQ);
            HDR_SET_ACK(msg_ACK.flags, HDR_ACK_ACK);
            msg.data_length = 0;
            set_message_checksum(&msg_ACK);
            send_message(&msg_ACK, sock, server);
            fprintf(stderr, "Duplicate packet detected! Packet dropped\n");
            continue;
        }

        if (!(HDR_GET_META(received_msg.flags)))
        {
            lastSEQ = HDR_GET_SEQ(received_msg.flags);
        }

        if (len < 0)
        {
            perror("recvfrom failed");
            continue;
        }

        if (HDR_GET_META(received_msg.flags))
        {
            printf("RECEIVE META DATA\n");
            file = fopen(received_msg.data, "ab");
            if (!file)
            {
                perror("Failed to open file for writing");
                return 1;
            }
            lastSEQ = 1;
        }
        else
        {
            if (HDR_GET_STATUS(received_msg.flags) == HDR_STATUS_FNF)
            {
                fprintf(stderr, "Error: File not found on server.\n");
                return 1;
            }
            printf("RECEIVE DATA\n");
            fwrite(received_msg.data, 1, received_msg.data_length, file);
        }

        if (HDR_GET_FIN(received_msg.flags))
        {
            printf("END\n");

            // Send final ACK after receiving FIN
            memset(&msg, 0, sizeof(msg));
            HDR_SET_ACK(msg.flags, HDR_ACK_ACK);
            HDR_SET_SEQ(msg.flags, lastSEQ);
            msg.data_length = 0;
            set_message_checksum(&msg);
            send_message(&msg, sock, server);

            break;
        }

        // Send ACK for data packet
        memset(&msg, 0, sizeof(msg));
        HDR_SET_ACK(msg.flags, HDR_ACK_ACK);
        HDR_SET_SEQ(msg.flags, lastSEQ);
        msg.data_length = 0;
        set_message_checksum(&msg);
        send_message(&msg, sock, server);
    }

    if (file)
        fclose(file);
    printf("Successfully received the file from the server\n");
    printf("File %s reassembled successfully.\n", fileName);
    return 0;
}