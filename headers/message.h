#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <netinet/in.h>
#include "connection.h"

#define BUFFER_SIZE 1024
#define PORT 8080

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
 * @param server: the server's socket address (for checksum)
 */
void segment_file(const char* filename, int sock, struct sockaddr_in client, struct sockaddr_in server);


/**
 * @brief Requests a file from a server and reassembles it.
 * This function sends a request for a file to the server and handles
 * the reception and reassembly of the file from incoming packets.
 * @param fileName: the name of the file to request
 * @param sock: the socket file descriptor
 * @param server: the server's socket address
 * @param client_addr: the client's socket address (for checksum)
 */
int request_file(char fileName[], int sock, struct sockaddr_in server, struct sockaddr_in client_addr);


#endif