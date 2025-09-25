#include "connection.h"
#include "message.h"
#include "checksum.h"


int client_handle_handshake(int sock, struct message *msg , struct sockaddr_in server){
    unsigned int server_len = sizeof(server);
    
    memset(msg, 0, sizeof(*msg));
    msg->data_length = 0;
    HDR_SET_ACK(msg->flags, HDR_ACK_NONE);
    HDR_SET_META(msg->flags);
    HDR_SET_SEQ(msg->flags, 0); // Initial sequence number

    set_message_checksum(msg);

    size_t handshake_size = sizeof(msg->checksum) + sizeof(msg->flags);

    if (sendto(sock, msg, handshake_size, 0, (struct sockaddr *)&server, server_len) < 0) {
        perror("sendto failed");
        return ERR_SEND_FAIL;
    }
    printf("META sent, waiting for META-ACK...\n");

    // Set 30 sec timeout for handshake
    struct timeval tv;
    tv.tv_sec = 30;
    tv.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt failed");
        return ERR_SOCKET_FAIL;
    }

    memset(msg, 0, sizeof(*msg));
    int recv_len = recvfrom(sock, msg, sizeof(*msg), 0, (struct sockaddr *)&server, &server_len);
    if (recv_len < 0) {
        perror("recvfrom failed or timeout");
        return ERR_RECV_FAIL;
    }

    if (!HDR_GET_META(msg->flags) || HDR_GET_ACK(msg->flags) != HDR_ACK_ACK || HDR_GET_SEQ(msg->flags) != 0) {
        return ERR_INVALID_HANDSHAKE;
    }
    printf("META-ACK received, handshake successful.\n");



    memset(msg, 0, sizeof(*msg));
    msg->data_length = 0;
    HDR_SET_ACK(msg->flags, HDR_ACK_ACK); 
    HDR_CLR_META(msg->flags);            
    HDR_SET_SEQ(msg->flags, 1);           

    set_message_checksum(msg); 


    size_t handshake_ack_size = sizeof(msg->checksum) + sizeof(msg->flags);

    if (sendto(sock, msg, handshake_ack_size, 0, (struct sockaddr *)&server, server_len) < 0) {
        perror("Final ACK sendto failed");
        return ERR_SEND_FAIL;
    }

    return 0; 
}