#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "connection.h"
#include "message.h"
#include "checksum.h"
#include "./utils.c"
#include <errno.h>

void set_recv_timeout(int sock, int timeout_msec) {
    #ifdef _WIN32
        DWORD timeout = timeout_msec; 
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
            perror("setsockopt(SO_RCVTIMEO) failed");
        }
    #else
        struct timeval tv;
        if (timeout_msec == 0) {
            tv.tv_sec  = 0;
            tv.tv_usec = 0;
        } else {
            tv.tv_sec  = timeout_msec / 1000;
            tv.tv_usec = (timeout_msec % 1000) * 1000;
        }
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            perror("setsockopt(SO_RCVTIMEO) failed");
        }
    #endif
}


short window_count = 0;

void send_NACK(int sock, struct sockaddr_in dest_addr) {
    struct message msg_NACK;
    memset(&msg_NACK, 0, sizeof(msg_NACK));
    HDR_SET_ACK(msg_NACK.flags, HDR_ACK_NACK);
    msg_NACK.data_length = 0;
    set_message_checksum(&msg_NACK);
    send_message(&msg_NACK, sock, dest_addr);
}

int wait_response_from_client(struct message msg, struct sockaddr_in client, int sock, struct packet* packets) {
        set_recv_timeout(sock, 5000);
        struct message msg_response;
        memset(&msg_response, 0, sizeof(msg_response));
        socklen_t client_len = sizeof(client);
        int received_len = recvfrom(sock, &msg_response, sizeof(msg_response), 0, (struct sockaddr *)&client, &client_len);
        // if (errno == EWOULDBLOCK || errno == EAGAIN) {
        //     errno = 0;
        //     printf("Receive timeout!!\n");
        // }

        if (received_len < 0)
        {
            perror("recvfrom failed");
            set_recv_timeout(sock, 0);
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
            // if (HDR_GET_SEQ(msg_response.flags) < seq) {
            //     continue;
            // }
            if (packets == NULL) {
                // if (HDR_GET_SEQ(msg_response.flags) < seq) {
                //     continue;
                // }
                set_recv_timeout(sock, 0);
                return 0;
            }
            for (int i = 0; i < window_count; i++) {
                if (HDR_GET_SEQ(packets[i].msg.flags) == HDR_GET_SEQ(msg_response.flags)) {
                    if (packets[i].received) {
                        set_recv_timeout(sock, 0);
                        return 0;
                    }
                    packets[i].received = true;
                    set_recv_timeout(sock, 0);
                    return 0;
                }
            }
        }
        set_recv_timeout(sock, 0);
        return 0;
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

void segment_file(const char *filename, int sock, struct sockaddr_in client, struct packet* packets)
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
    // char filename_copy[256];
    // sprintf(filename_copy, "[c]%s", filename);
    strcpy(msg.data, filename);
    msg.data_length = strlen(msg.data);
    // printf("Length of meta data: %d\n", msg.data_length);
    set_message_checksum(&msg);
    send_message(&msg, sock, client);

    wait_response_from_client(msg, client, sock, NULL);

    size_t bytes_read = 1;
    uint32_t seqNum = 0x000000001u;
    bool dropped = false;
    while (1)
    {   
        if (bytes_read <= 0) break;
        window_count = 0;
        // read and keep in buffer
        for (int i = 0; i < WINDOW_SIZE && (bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0 ; i++) {
            // printf("Read %zu bytes from file for packet %d\n", bytes_read, i);
            memset(&msg, 0, sizeof(msg));
            HDR_SET_SEQ(msg.flags, seqNum);
            memcpy(msg.data, buffer, bytes_read);
            msg.data_length = bytes_read;
            // printf("Packet %d data length: %d\n", i, msg.data_length);
            // printf("%s\n", msg.data);
            
            set_message_checksum(&msg);
            struct packet pkt = {false, msg, current_time_ms()};
            packets[i] = pkt;
            seqNum++;
            window_count++;
        }

        for (int i = 0; i < window_count; i++) {
            printf("Sending packet with sequence number: %d\n", seqNum);
            packets[i].sent_timestamp = current_time_ms();
            /* drop packet test */
            // if (HDR_GET_SEQ(packets[i].msg.flags) % 351 == 0 && !dropped) {
            //     printf("Simulating packet drop for packet %d\n", HDR_GET_SEQ(packets[i].msg.flags));
            //     dropped = true;
            //     continue;
            // } else{
            //     dropped = false;
            // }

            send_message(&(packets[i].msg), sock, client);
        }

        
        while (1) {
            short all_acked = 0;
            // printf("all acked before wait: %d\n", all_acked);
            wait_response_from_client(msg, client, sock, packets);
            for (int i = 0; i < window_count; i++) {
                if (!packets[i].received && (current_time_ms() - packets[i].sent_timestamp >= TIMEOUT_MSEC)) {
                    printf("Timeout for packet %d, resending...\n", HDR_GET_SEQ(packets[i].msg.flags));
                    packets[i].sent_timestamp = current_time_ms();
                    send_message(&(packets[i].msg), sock, client);
                }

                // if (packets[i].received) {
                //     all_acked++;
                // }
            }
            for (int i = 0; i < window_count; i++) {
                if (packets[i].received) all_acked++;
            }
            // printf("all acked before exit while: %d\n", all_acked);
            if (all_acked == window_count) break;
        }

        // send_message(&msg, sock, client);
        
        
        // seqNum++;
        // if (seqNum == 10) {
        //     send_message(&msg, sock, client);
        // }

        // Waiting for client send ACK if client send NACK server will while loop send packet until client send ACK
        // if (wait_response_from_client(msg, client, sock, seqNum)) continue;
        

    }

    // Send final packet with FIN flag
    memset(&msg, 0, sizeof(msg));
    HDR_SET_SEQ(msg.flags, seqNum);
    HDR_SET_FIN(msg.flags);
    msg.data_length = 0;
    set_message_checksum(&msg);
    send_message(&msg, sock, client);

    wait_response_from_client(msg, client, sock, NULL);

    fclose(file);
    printf("File %s segmented and sent.\n", filename);
}

int request_file(char fileName[], int sock, struct sockaddr_in server)
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
    bool data_started = false;

    struct packet packets[WINDOW_SIZE];
    short packet_count = 0;
    while (1)
    {
        memset(&received_msg, 0, sizeof(received_msg));
        socklen_t server_len = sizeof(server);
        int len = recvfrom(sock, &received_msg, sizeof(received_msg), 0, (struct sockaddr *)&server, &server_len);

        // Validate the checksum of the incoming request
        received_msg.data_length = len - (sizeof(received_msg.checksum) + sizeof(received_msg.flags));
        printf("%d\n", received_msg.data_length);

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

            char filename_to_be_saved[1024];
            char filename_only[896];
            char file_extension[32];
            
            char *dot = strrchr(received_msg.data, '.');


            bool has_extension = (dot != NULL && dot != received_msg.data);
            strncpy(filename_to_be_saved, received_msg.data, sizeof(received_msg.data) - 1);
            filename_to_be_saved[sizeof(received_msg.data) - 1] = '\0';

            if (has_extension) {
                strncpy(filename_only, received_msg.data, dot - received_msg.data);
                snprintf(file_extension, sizeof(file_extension), "%s", dot);
                filename_only[dot - received_msg.data] = '\0';
            } else {
                strncpy(filename_only, received_msg.data, sizeof(filename_only) - 1);
                filename_only[sizeof(filename_only) - 1] = '\0';
            }


            short suffix_identifier = 1;
            while (access(filename_to_be_saved, F_OK) == 0) {
                if (has_extension)
                    snprintf(filename_to_be_saved, sizeof(filename_to_be_saved), "%s_(%d)%s", filename_only, suffix_identifier, file_extension);
                else
                    snprintf(filename_to_be_saved, sizeof(filename_to_be_saved), "%s_(%d)", filename_only, suffix_identifier);

                printf("File already exists. Trying new name: %s\n", filename_to_be_saved);
                suffix_identifier++;
            }
            printf("Saving file as: %s\n", filename_to_be_saved);

            file = fopen(filename_to_be_saved, "ab");

            if (!file)
            {
                perror("Failed to open file for writing");
                return 1;
            }
            lastSEQ = 0;
        }
        else
        {
            if (packet_count == WINDOW_SIZE)
            {
                for (int i = 0; i < WINDOW_SIZE; i++)
                {
                    if (packets[i].received && packets[i].msg.data_length > 0)
                    {
                        printf("Writing packet with sequence number: %d to file (%d)\n", HDR_GET_SEQ(packets[i].msg.flags), packets[i].msg.data_length);
                        fwrite(packets[i].msg.data, 1, packets[i].msg.data_length, file);  
                    }
                    packets[i].received = false;
                }
                packet_count = 0;
            }
            if (HDR_GET_STATUS(received_msg.flags) == HDR_STATUS_FNF)
            {
                fprintf(stderr, "Error: File not found on server.\n");
                return 1;
            }
            printf("RECEIVE DATA\n");
            // for (int i = 0; i < WINDOW_SIZE; i++) {
            //     if (HDR_GET_SEQ(packets[i].msg.flags) != HDR_GET_SEQ(received_msg.flags)) {

            //     }
            // }
            struct packet pkt = {true, received_msg, current_time_ms()};
            packets[((HDR_GET_SEQ(pkt.msg.flags) - 1) % 5)] = pkt;
            packet_count++;
        }

        if (HDR_GET_FIN(received_msg.flags))
        {
            printf("END\n");
            
            for (int i = 0; i < packet_count; i++)
            {
                if (packets[i].received && packets[i].msg.data_length > 0)
                {
                    printf("Writing packet with sequence number: %d to file (%d)\n", HDR_GET_SEQ(packets[i].msg.flags), packets[i].msg.data_length);
                    fwrite(packets[i].msg.data, 1, packets[i].msg.data_length, file);  
                }
                packets[i].received = false;
            }
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

        /* ACK Drop test */
        // if (HDR_GET_SEQ(received_msg.flags) % 350 == 0) {
        //     continue;
        // }
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