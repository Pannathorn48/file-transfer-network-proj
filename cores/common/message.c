#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "connection.h"
#include "message.h"
#include "checksum.h"

// Helper function to send a message without waiting for an ACK
int send_message(struct message *msg, int sock, struct sockaddr_in dest_addr)
{
    unsigned int dest_len = sizeof(dest_addr);
    size_t packet_size = sizeof(msg->checksum) + sizeof(msg->flags) + msg->data_length;

    printf("Sending packet with flags: %x, size: %zu\n", msg->flags, packet_size);

    if (sendto(sock, msg, packet_size, 0, (struct sockaddr *)&dest_addr, dest_len) < 0)
    {
        perror("sendto failed");
        return 1;
    }
    return 0;
}


void segment_file(const char* filename, int sock, struct sockaddr_in client, struct sockaddr_in server) { 
    char buffer[BUFFER_SIZE];
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    struct message msg;
    memset(&msg, 0, sizeof(msg));
    
    // Send metadata packet
    HDR_SET_META(msg.flags);
    char filename_copy[256];
    sprintf(filename_copy, "[c]%s", filename);
    strcpy(msg.data, filename_copy);
    msg.data_length = strlen(msg.data);
    // printf("Length of meta data: %d\n", msg.data_length);
    set_message_checksum(&msg);
    send_message(&msg, sock, client);

    size_t bytes_read;
    uint32_t seqNum = 0x0;

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        memset(&msg, 0, sizeof(msg));
        HDR_SET_SEQ(msg.flags, seqNum);
        memcpy(msg.data, buffer, bytes_read);
        msg.data_length = bytes_read;
        // printf("Length of data: %d\n", msg.data_length);
        set_message_checksum(&msg);
        send_message(&msg, sock, client);
        seqNum++;
    }

    // Send final packet with FIN flag
    memset(&msg, 0, sizeof(msg));
    HDR_SET_FIN(msg.flags);
    msg.data_length = 0;
    set_message_checksum(&msg);
    send_message(&msg, sock, client);

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

    while (1)
    {
        memset(&received_msg, 0, sizeof(received_msg));
        socklen_t server_len = sizeof(server);
        int len = recvfrom(sock, &received_msg, sizeof(received_msg), 0, (struct sockaddr *)&server, &server_len);
        
        if (len < 0) {
            perror("recvfrom failed");
            continue;
        }

        received_msg.data_length = len - (sizeof(received_msg.checksum) + sizeof(received_msg.flags));

        if (validate_message_checksum(&received_msg, len) != 0) {
            fprintf(stderr, "Checksum validation failed! Packet dropped.\n");
            continue;
        }

        if (HDR_GET_META(received_msg.flags)) {
            printf("RECEIVE META DATA\n");
            file = fopen(received_msg.data, "ab");
            if (!file) {
                perror("Failed to open file for writing");
                return 1;
            }
        }
        else {
            printf("RECEIVE DATA\n");
            fwrite(received_msg.data, 1, received_msg.data_length, file);
        }

        if (HDR_GET_FIN(received_msg.flags)) {
            printf("END\n");
            
            // Send final ACK after receiving FIN
            memset(&msg, 0, sizeof(msg));
            HDR_SET_ACK(msg.flags, HDR_ACK_ACK);
            msg.data_length = 0;
            set_message_checksum(&msg);
            send_message(&msg, sock, server);
            
            break;
        }

        // Send ACK for data packet
        memset(&msg, 0, sizeof(msg));
        HDR_SET_ACK(msg.flags, HDR_ACK_ACK);
        msg.data_length = 0;
        set_message_checksum(&msg);
        send_message(&msg, sock, server);
    }
    
    if (file) fclose(file);
    printf("Successfully received the file from the server\n");
    printf("File %s reassembled successfully.\n", fileName);
    return 0;
}