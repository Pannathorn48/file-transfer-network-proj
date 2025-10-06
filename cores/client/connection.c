#include "connection.h"
#include "message.h"
#include "checksum.h"
#include <signal.h>
#include <unistd.h>



int client_handle_handshake( int sock, struct message *msg , struct sockaddr_in server){
    unsigned int server_len = sizeof(server);
    int retry_count = 0;
    
    // Set timeout for handshake attempts
    struct timeval tv;
    tv.tv_sec = HANDSHAKE_TIMEOUT_SEC;
    tv.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt failed");
        return ERR_SOCKET_FAIL;
    }
    
    printf("Starting handshake process with max %d retries...\n", MAX_RETRY);
    
    // Retry loop for handshake
    while (retry_count < MAX_RETRY) {
        printf("Handshake attempt %d/%d\n", retry_count + 1, MAX_RETRY);
        
        // Step 1: Send META packet
        memset(msg, 0, sizeof(*msg));
        msg->data_length = 0;
        HDR_SET_ACK(msg->flags, HDR_ACK_NONE);
        HDR_SET_META(msg->flags);
        HDR_SET_SEQ(msg->flags, 0); // Initial sequence number

        set_message_checksum(msg);

        size_t handshake_size = sizeof(msg->checksum) + sizeof(msg->flags);

        if (sendto(sock, msg, handshake_size, 0, (struct sockaddr *)&server, server_len) < 0) {
            perror("sendto failed");
            retry_count++;
            if (retry_count >= MAX_RETRY) {
                printf("Max retries reached for sending META packet.\n");
                return ERR_SEND_FAIL;
            }
            printf("Retrying in 1 second...\n");
            sleep(1);
            continue;
        }
        printf("META sent, waiting for META-ACK...\n");

        // Step 2: Receive META-ACK
        memset(msg, 0, sizeof(*msg));
        int recv_len = recvfrom(sock, msg, sizeof(*msg), 0, (struct sockaddr *)&server, &server_len);
        if (recv_len < 0) {
            perror("recvfrom failed or timeout");
            retry_count++;
            if (retry_count >= MAX_RETRY) {
                printf("Max retries reached for receiving META-ACK.\n");
                return ERR_RECV_FAIL;
            }
            printf("Retrying in 1 second...\n");
            sleep(1);
            continue;
        }

        // Step 3: Validate META-ACK
        if (!HDR_GET_META(msg->flags) || HDR_GET_ACK(msg->flags) != HDR_ACK_ACK || HDR_GET_SEQ(msg->flags) != 0) {
            printf("Invalid handshake response received.\n");
            retry_count++;
            if (retry_count >= MAX_RETRY) {
                printf("Max retries reached for invalid handshake.\n");
                return ERR_INVALID_HANDSHAKE;
            }
            printf("Retrying in 1 second...\n");
            sleep(1);
            continue;
        }
        printf("META-ACK received, handshake successful.\n");

        // Step 4: Send final ACK
        memset(msg, 0, sizeof(*msg));
        msg->data_length = 0;
        HDR_SET_ACK(msg->flags, HDR_ACK_ACK); 
        HDR_CLR_META(msg->flags);            
        HDR_SET_SEQ(msg->flags, 1);           

        set_message_checksum(msg); 

        size_t handshake_ack_size = sizeof(msg->checksum) + sizeof(msg->flags);

        if (sendto(sock, msg, handshake_ack_size, 0, (struct sockaddr *)&server, server_len) < 0) {
            perror("Final ACK sendto failed");
            retry_count++;
            if (retry_count >= MAX_RETRY) {
                printf("Max retries reached for sending final ACK.\n");
                return ERR_SEND_FAIL;
            }
            printf("Retrying in 1 second...\n");
            sleep(1);
            continue;
        }
        printf("Final ACK sent, handshake completed successfully.\n");
        
        // Clear the socket timeout after successful handshake
        struct timeval tv_reset;
        tv_reset.tv_sec = 0;
        tv_reset.tv_usec = 0;
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv_reset, sizeof(tv_reset)) < 0) {
            perror("Warning: Failed to reset socket timeout");
            // Don't fail the handshake for this, just warn
        } else {
            printf("Socket timeout cleared after successful handshake.\n");
        }

        return 0; // Success
    }
    
    // If we reach here, all retries have been exhausted
    printf("Handshake failed after %d attempts.\n", MAX_RETRY);
    return ERR_MAX_RETRIES_REACHED; 
}



