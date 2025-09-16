//#include <stdio.h>
//#include <stdint.h>
#include <stdlib.h>
#include "connection.h"
#ifndef MESSAGE_H
#define MESSAGE_H
#define BUFFER_SIZE 1024
#define PORT 8080
#define INITIAL_QUEUE_CAPACITY 10


// extern Message* senderMessage;
// extern Message* receiverMessage;

/**
    *   @brief segments a file into multiple messages
    *   This function reads a file and segments it into multiple messages,
    *   each containing a portion of the file's data. The first message contains
    *   metadata about the file, including its name. Subsequent messages contain
    *   chunks of the file's data, each with a sequence number for ordering.
    *   The messages are stored in a dynamically allocated message queue.
    *   @param filename: the name of the file to be segmented
    *   @param socket: the socket used to send file
    *   @param sockaddre_in: client socket
*/
void segment_file(const char* filename, int , struct sockaddr_in);


/**
    *   @brief assembles a file from received messages
    *   This function takes a queue of received messages and reassembles them into a single file.
    *   It expects the first message in the queue to contain metadata about the file, including its name.
    *   Subsequent messages contain chunks of the file's data, which are written to the output file in order.
    *   The function handles file opening, writing, and closing, and provides error handling for file operations.
    *   @return void
 */
void assemble_file();

int send_file(struct message *msg, int , struct sockaddr_in);
int request_file(char [], int, struct sockaddr_in);
#endif 
