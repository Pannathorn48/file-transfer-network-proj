#include "standard_lib.h"
#include "network_lib.h"
#include <stdbool.h>
#include <errno.h>
#include "connection.h"
#include "message.h"
#include "checksum.h"
#include "random.h"
#include "utils.h"

// Global variable to keep track of the current window size
short window_count = 0;

/**
 * send_NACK: Sends a NACK (Negative Acknowledgment) message to the client
 * @sock: socket descriptor
 * @dest_addr: destination address (client)
 * @seqNum: sequence number of the message that failed validation
 */
void send_NACK(int sock, struct sockaddr_in dest_addr , u_int32_t seqNum) {
    struct message msg_NACK;
    memset(&msg_NACK, 0, sizeof(msg_NACK));
    HDR_SET_ACK(msg_NACK.flags, HDR_ACK_NACK); // Set NACK flag
    HDR_SET_SEQ(msg_NACK.flags, seqNum);       // Set the sequence number
    msg_NACK.data_length = 0;                  // No data in NACK
    set_message_checksum(&msg_NACK);           // Compute checksum
    send_message(&msg_NACK, sock, dest_addr);  // Send NACK
}

/**
 * wait_response_from_client: Waits for a response (ACK/NACK) from the client
 * @msg: original message sent
 * @client: client address
 * @sock: socket descriptor
 * @packets: array of packets in the window for potential retransmission
 * Returns 0 on success, -1 on error
 */
int wait_response_from_client(struct message msg, struct sockaddr_in client, int sock, struct packet* packets) {
        // Set socket receive timeout
        set_recv_timeout(sock, TIMEOUT_MSEC + 2000);
        struct message msg_response;
        memset(&msg_response, 0, sizeof(msg_response));
        socklen_t client_len = sizeof(client);
        
        // Receive response from client
        int received_len = recvfrom(sock, &msg_response, sizeof(msg_response), 0, (struct sockaddr *)&client, &client_len);

        if (received_len < 0)
        {
            perror("recvfrom failed");
            set_recv_timeout(sock, 0);
            return -1;
        }

        // Calculate actual data length of the received message
        msg_response.data_length = received_len - (sizeof(msg_response.checksum) + sizeof(msg_response.flags));

        // Check checksum; if invalid, send NACK and wait for retransmission
        while (validate_message_checksum(&msg_response, received_len) != 0)
        {
            send_NACK(sock, client , HDR_GET_SEQ(msg.flags));

            fprintf(stderr, "Checksum ACK NACK validation failed! Packet dropped.\n");
            int received_len = recvfrom(sock, &msg_response, sizeof(msg_response), 0, (struct sockaddr *)&client, &client_len);
            if (received_len < 0)
            {
                perror("recvfrom failed");
                continue;
            }
        }

        // Handle NACK received from client
        while (!(HDR_GET_ACK(msg_response.flags) ^ HDR_ACK_NACK))
        {
            struct message* resend_msg = NULL;
            if (HDR_GET_META(msg.flags) || HDR_GET_SEQ(msg.flags) == HDR_GET_SEQ(msg_response.flags)) {
                resend_msg = &msg;               // Resend original message
                set_message_checksum(resend_msg);
            } else if (packets != NULL) {
                resend_msg = NULL;
                // Search for the packet in the window by sequence number
                for (int i = 0; i < window_count; i++) {
                    if (HDR_GET_SEQ(packets[i].msg.flags) == HDR_GET_SEQ(msg_response.flags)) {
                        resend_msg = &(packets[i].msg);
                        break;
                    }
                }
                if (resend_msg == NULL) {
                    fprintf(stderr, "No matching packet found to resend for sequence number: %d\n", HDR_GET_SEQ(msg_response.flags));
                    set_recv_timeout(sock, 0);
                    return 0;
                }
                set_message_checksum(resend_msg);
            } else {
                fprintf(stderr, "No packets array provided for resending data packet.\n");
                set_recv_timeout(sock, 0);
                return 0;
            }
            printf("Receive NACK resend packet: %d\n", HDR_GET_SEQ(msg_response.flags));
            send_message(resend_msg, sock, client);
            memset(&msg_response, 0, sizeof(msg_response));
            int received_len = recvfrom(sock, &msg_response, sizeof(msg_response), 0, (struct sockaddr *)&client, &client_len);
            if (received_len < 0)
            {
                perror("recvfrom failed");
                continue;
            }
        }

        // Handle duplicate ACKs received from client
        if (!(HDR_GET_ACK(msg_response.flags) ^ HDR_ACK_ACK)) {
            printf("Receive ACK sequence number: %d\n", HDR_GET_SEQ(msg_response.flags));
            if (packets == NULL) {
                set_recv_timeout(sock, 0);
                return 0;
            }
            for (int i = 0; i < window_count; i++) {
                if (HDR_GET_SEQ(packets[i].msg.flags) == HDR_GET_SEQ(msg_response.flags)) {
                    if (packets[i].received) {
                        set_recv_timeout(sock, 0);
                        return 0;
                    }
                    packets[i].received = true;  // Mark packet as acknowledged
                    set_recv_timeout(sock, 0);
                    return 0;
                }
            }
        }
        set_recv_timeout(sock, 0);
        return 0;
}

/**
 * send_message: Sends a message through the socket to the destination
 * @msg: pointer to message
 * @sock: socket descriptor
 * @dest_addr: destination address
 * Returns 0 on success, 1 on failure
 */
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

/**
 * segment_file: Reads a file, segments it into packets, and sends them using sliding window
 * @filename: name of the file to send
 * @sock: socket descriptor
 * @client: client address
 * @packets: array of packets for windowing
 */
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
        HDR_SET_STATUS(msg.flags, HDR_STATUS_FNF); // Set file-not-found status
        HDR_SET_SEQ(msg.flags, 1);
        set_message_checksum(&msg);
        send_message(&msg, sock, client);
        return;
    }

    // Send metadata packet containing filename
    HDR_SET_META(msg.flags);
    strcpy(msg.data, filename);
    msg.data_length = strlen(msg.data);
    set_message_checksum(&msg);

    send_message(&msg, sock, client);

    // Wait for ACK of metadata, resend if timeout or failure
    while(wait_response_from_client(msg, client, sock, NULL) == -1) {
        printf("Resending META packet...\n");
        send_message(&msg, sock, client);
    }

    size_t bytes_read = 1;
    uint32_t seqNum = 0x000000001u;

    // Main loop to read file, segment into packets, and send
    while (1)
    {   
        if (bytes_read <= 0) break;
        window_count = 0;

        // Read data into buffer and create packets for window
        for (int i = 0; i < WINDOW_SIZE && (bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0 ; i++) {
            memset(&msg, 0, sizeof(msg));
            HDR_SET_SEQ(msg.flags, seqNum);
            memcpy(msg.data, buffer, bytes_read);
            msg.data_length = bytes_read;
            
            set_message_checksum(&msg);
            struct packet pkt = {false, msg, current_time_ms()};
            packets[i] = pkt;
            seqNum++;
            window_count++;
        }

        // Send packets in the window
        for (int i = 0; i < window_count; i++) {
            packets[i].sent_timestamp = current_time_ms();

            // Simulate packet loss
            if (random_percent(TEST_SENDER_PACKET_LOSS_PERCENTAGE)){
                printLnColor(ORG, "Simulating packet loss for packet with sequence number: %d", HDR_GET_SEQ(packets[i].msg.flags));
                continue;
            }

            // Simulate packet corruption
            if (random_percent(TEST_SENDER_PACKET_CORRUPTION_PERCENTAGE)){
                printLnColor(ORG, "Simulating packet corruption for packet with sequence number: %d", HDR_GET_SEQ(packets[i].msg.flags));
                packets[i].msg.checksum ^= 0xFFFF;
            }

            // Simulate duplicate packet
            if (random_percent(TEST_SENDER_DUPLICATE_PACKET_PERCENTAGE)){
                printLnColor(ORG, "Send duplicate packet for packet with sequence number: %d", HDR_GET_SEQ(packets[i].msg.flags));
                send_message(&(packets[i].msg), sock, client);
            }
            send_message(&(packets[i].msg), sock, client);
        }

        // Wait for ACKs and handle retransmissions
        while (1) {
            short all_acked = 0;
            wait_response_from_client(msg, client, sock, packets);
            for (int i = 0; i < window_count; i++) {
                if (!packets[i].received && (current_time_ms() - packets[i].sent_timestamp >= TIMEOUT_MSEC)) {
                    printf("Timeout for packet %d, resending...\n", HDR_GET_SEQ(packets[i].msg.flags));
                    packets[i].sent_timestamp = current_time_ms();
                    send_message(&(packets[i].msg), sock, client);
                }

            }
            for (int i = 0; i < window_count; i++) {
                if (packets[i].received) all_acked++;
            }
            if (all_acked == window_count) break; // All packets in window acknowledged
        }

    }

    // Send final FIN packet to signal end of transmission
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

/**
 * request_file: Requests a file from the server and handles reassembly
 * @fileName: name of the file to request
 * @sock: socket descriptor
 * @server: server address
 * Returns 0 on success, 1 on failure
 */
int request_file(char fileName[], int sock, struct sockaddr_in server)
{
    bool is_meta_received = false;
    struct message msg;
    char filename_to_be_saved[1024];

    uint32_t memo[WINDOW_SIZE]; // Track sequence numbers for window
    for (int i = 0  ; i < WINDOW_SIZE ; i++) memo[i] = i + 1;

    memset(&msg, 0, sizeof(msg));

    // Send request message to server
    strncpy(msg.data, fileName, sizeof(msg.data));
    msg.data_length = strlen(msg.data);
    HDR_SET_ACK(msg.flags, HDR_ACK_NONE);

    set_message_checksum(&msg);

    printf("REQUEST: %s\n", msg.data);
    send_message(&msg, sock, server);

    struct message received_msg;
    FILE *file = NULL;
    uint32_t lastSEQ = 0x00000000u;

    struct packet packets[WINDOW_SIZE];
    short packet_count = 0;

    // Loop to receive packets from server
    while (1)
    {
        memset(&received_msg, 0, sizeof(received_msg));
        socklen_t server_len = sizeof(server);
        int len = recvfrom(sock, &received_msg, sizeof(received_msg), 0, (struct sockaddr *)&server, &server_len);

        // Calculate data length
        received_msg.data_length = len - (sizeof(received_msg.checksum) + sizeof(received_msg.flags));
        printf("%d\n", received_msg.data_length);

        // Validate checksum, request retransmission if failed
        while (validate_message_checksum(&received_msg, len) != 0)
        {
            fprintf(stderr, "Checksum validation failed! Packet dropped.\n");
            send_NACK(sock, server , HDR_GET_SEQ(received_msg.flags));

            int received_len = recvfrom(sock, &received_msg, sizeof(received_msg), 0, (struct sockaddr *)&server, &server_len);
            if (received_len < 0)
            {
                perror("recvfrom failed");
                continue;
            }
        }

        // Handle duplicate packets
        if (HDR_GET_SEQ(received_msg.flags) < memo[(HDR_GET_SEQ(received_msg.flags) - 1) % WINDOW_SIZE ] && !(HDR_GET_META(received_msg.flags))) {
            struct message msg_ACK;
            memset(&msg_ACK, 0, sizeof(msg_ACK));
            HDR_SET_SEQ(msg_ACK.flags, HDR_GET_SEQ(received_msg.flags));
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

        // Handle metadata packet (filename)
        if (HDR_GET_META(received_msg.flags))
        {
            if(is_meta_received){
                printf("Duplicate META packet received, ignoring...\n");
                struct message msg_ACK;
                memset(&msg_ACK, 0, sizeof(msg_ACK));
                HDR_SET_SEQ(msg_ACK.flags, 0);
                HDR_SET_ACK(msg_ACK.flags, HDR_ACK_ACK);
                msg.data_length = 0;
                set_message_checksum(&msg_ACK);
                send_message(&msg_ACK, sock, server); 
                continue;
            }
            is_meta_received = true;
            printf("RECEIVE META DATA\n");

            char filename_only[896];
            char file_extension[32];
            
            // find the last '.' in the filename to separate extension
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

            // Ensure file does not overwrite existing files
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
            // Save received data packet into window buffer
            struct packet pkt = {true, received_msg, current_time_ms()};
            packets[((HDR_GET_SEQ(pkt.msg.flags) - 1) % WINDOW_SIZE)] = pkt;
            memo[(HDR_GET_SEQ(pkt.msg.flags) - 1) % WINDOW_SIZE] = memo[(HDR_GET_SEQ(pkt.msg.flags) - 1) % WINDOW_SIZE] + WINDOW_SIZE;
            packet_count++;

            // Write packets to file when window is full
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

            // Handle file-not-found status
            if (HDR_GET_STATUS(received_msg.flags) == HDR_STATUS_FNF)
            {
                fprintf(stderr, "Error: File not found on server.\n");
                return 1;
            }
            printf("RECEIVE DATA\n");
        }

        // Handle FIN packet from server
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
            // Send final ACK to server
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
        if (random_percent(TEST_RECEIVER_PACKET_LOSS_PERCENTAGE)){
            printLnColor(ORG, "Simulating ACK loss for packet with sequence number: %d", lastSEQ);
            continue; // Simulate ACK loss by skipping the send
        }
        HDR_SET_SEQ(msg.flags, lastSEQ);
        msg.data_length = 0;
        set_message_checksum(&msg);
        send_message(&msg, sock, server);
    }

    if (file)
        fclose(file);
    printf("Successfully received the file from the server\n");
    printLnColor(L_GRN, "File %s reassembled successfully.", fileName);
    printLnColor(L_GRN, "Saving received file as %s\n", filename_to_be_saved);
    return 0;
}
