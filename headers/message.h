#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <netinet/in.h>
#include "connection.h"
#include <stdbool.h>

#define BUFFER_SIZE 1024
#define PORT 8080
#define WINDOW_SIZE 5
#define TIMEOUT_MSEC 3000


struct packet {
    bool received;
    struct message msg;
    long long sent_timestamp;
};

/**
 * @brief Sends a message over a socket.
 * This is a helper function to send a message to a destination address.
 *
 * @param msg       A pointer to the message to be sent.
 * @param sock      The socket file descriptor.
 * @param dest_addr The destination socket address structure.
 * @return Returns 0 on success, 1 on failure.
 */
int send_message(struct message *msg, int sock, struct sockaddr_in dest_addr);


/**
 * @brief segments a file into multiple messages
 * This function reads a file and segments it into multiple messages,
 * each containing a portion of the file's data.
 * @param filename: the name of the file to be segmented
 * @param sock: the socket used to send file
 * @param client: the client's socket address
 * @param packets: array of packets for implementing selective repeat
 */
void segment_file(const char* filename, int sock, struct sockaddr_in client, struct packet* packets);


/**
 * @brief Requests a file from a server and reassembles it.
 * This function sends a request for a file to the server and handles
 * the reception and reassembly of the file from incoming packets.
 * @param fileName: the name of the file to request
 * @param sock: the socket file descriptor
 * @param server: the server's socket address
 */
int request_file(char fileName[], int sock, struct sockaddr_in server);


/**
 * @brief Sends a NACK (Negative Acknowledgment) message to the destination.
 * This function constructs a message with the NACK flag set and sends it
 * to the specified destination address using the provided socket.
 * 
 * @param sock The socket file descriptor used for sending the NACK.
 * @param dest_addr The destination socket address to which the NACK is sent.
 */
void send_NACK(int sock, struct sockaddr_in dest_addr);


/**
 * @brief Waits for and processes a response from the client.
 * This function listens for a response packet (ACK or NACK) from the client.
 * - If the received checksum is invalid, it sends a NACK and retries until a valid packet is received.
 * - If a NACK is received from the client, it resends the original message.
 * - If an ACK is received, it checks the sequence number against the expected sequence.
 * 
 * @param msg The original message that was sent to the client (used for retransmission if needed).
 * @param client The client's socket address (source of the response).
 * @param sock The socket file descriptor used for communication.
 * 
 * @return int Status code:
 *         - 0 if a valid ACK is received for the expected sequence.
 *         - 1 if recvfrom failed during reception.
 */
int wait_response_from_client(struct message msg, struct sockaddr_in client, int sock, struct packet* packets);
#endif