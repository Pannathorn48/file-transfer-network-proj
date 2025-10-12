#ifndef NETWORK_HEADERS_H
#define NETWORK_HEADERS_H

#include "standard_lib.h"
#include "network_lib.h"
#include "connection.h"
#include "error.h"
#include "checksum.h"


#endif /* NETWORK_HEADERS_H */

int new_connection(char* port , struct sockaddr_in* server){
    int port_number = atoi(port);
    /* Convert the provided port string to an integer. Validate range: 1..65535. */
    if (port_number == 0 || port_number > 65535 || port_number < 0){
        return ERR_INVALID_PORT;
    }

    int sock = socket(AF_INET, SOCK_DGRAM , 0);

    if (sock < 0){
        perror("Socket creation failed");
        return ERR_SOCKET_FAIL;
    }

    /*
     * Prepare the server address structure (IPv4).
     * - Zero the structure
     * - Use AF_INET (IPv4)
     * - Bind to all available interfaces (INADDR_ANY)
     * - Store the network-byte-order port using htons()
     */
    memset(server, 0, sizeof(*server));
    server->sin_family = AF_INET;
    server->sin_addr.s_addr = INADDR_ANY;
    server->sin_port = htons(port_number);

    if(bind(sock, (struct sockaddr*)server, sizeof(*server)) < 0){
        perror("Bind failed");
        close(sock);
        return ERR_PORT_IN_USE;
    }

    printf("Server is listening on port %d ...\n", port_number);
    return sock;
}

int server_handle_handshake(int sock, struct message *msg , struct sockaddr_in client){
    unsigned int client_len = sizeof(client);
    /*
     * The server-side handshake handler responds to a client's initial META
     * packet. The expected flow (from server perspective):
     *  1. Receive META from client (this function is typically called after recvfrom detected META)
     *  2. Send a META-ACK back to the client (ACK set, META set, seq=0)
     *  3. Wait for the client's final ACK (ACK set, META cleared, seq=1)
     *  4. Validate and return success or an error code on failure
     */

    /* Step 1: Build and send the META-ACK packet back to the client */
    memset(msg, 0, sizeof(*msg));           // Zero message struct
    msg->data_length = 0;                   // No payload in handshake packets
    HDR_SET_ACK(msg->flags, HDR_ACK_ACK);   // Set ACK field to indicate acknowledgment
    HDR_SET_META(msg->flags);               // Mark as META packet (metadata-only)
    HDR_SET_SEQ(msg->flags, 0);             // Sequence number 0 for handshake

    set_message_checksum(msg);              // Compute and populate checksum for integrity

    size_t handshake_size = sizeof(msg->checksum) + sizeof(msg->flags);

    if (sendto(sock, msg, handshake_size, 0, (struct sockaddr *)&client, client_len) < 0) {
        /* If the send fails, report and return a send error code */
        perror("sendto failed");
        return ERR_SEND_FAIL;
    }

    /* Step 2: Configure a receive timeout so the server doesn't block forever
     * while waiting for the client's final ACK. We set a 30-second timeout.
     */
    #if defined(_WIN32) || defined(_WIN64)
        DWORD timeout = 30000; 
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) < 0) {
            fprintf(stderr, "setsockopt failed: %d\n", WSAGetLastError());
            return ERR_SEND_FAIL;
        }
    #else
        struct timeval tv;
        tv.tv_sec = 30;   // seconds
        tv.tv_usec = 0;   // microseconds
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            /* If we cannot set the timeout, return a socket error */
            perror("setsockopt failed");
            return ERR_SOCKET_FAIL;
        }
    #endif

    /* Step 3: Wait for the client's final ACK packet (with seq=1)
     * - We re-use the same `msg` buffer to receive the response.
     * - If recvfrom returns < 0, a timeout or socket error occurred.
     */
    memset(msg, 0, sizeof(*msg));
    int recv_len = recvfrom(sock, msg, sizeof(*msg), 0, (struct sockaddr *)&client, &client_len);
    if (recv_len < 0) {
        perror("recvfrom failed or timeout");
        return ERR_RECV_FAIL; // Propagate receive failure
    }

    /* Step 4: Restore socket to blocking behavior (clear SO_RCVTIMEO)
     * - We attempt to remove the timeout so normal server loops behave
     *   as expected after the handshake completes.
     */
    #if defined(_WIN32) || defined(_WIN64)
        DWORD timeout_reset = 0; 
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout_reset, sizeof(timeout_reset));
    #else
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    #endif

    /* Step 5: Validate the received ACK packet
     * - Ensure the ACK field is set to ACK
     * - Ensure the sequence number matches the expected handshake completion seq (1)
     */
    if (HDR_GET_ACK(msg->flags) != HDR_ACK_ACK || HDR_GET_SEQ(msg->flags) != 1) {
        fprintf(stderr, "Invalid ACK received during handshake\n");
        return ERR_INVALID_HANDSHAKE;
    }

    /* If validation passed, handshake is complete. Log the client's address and port. */
    printf("Handshake completed with client %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
    return 0;
}
