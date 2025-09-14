#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "connection.h"

#define BUFFER_SIZE 1024
#define PORT 8080
#define INITIAL_QUEUE_CAPACITY 10

typedef struct message Message;

// typedef struct node_t{
//     Message* msg;
//     struct node_t* next;
// } MessageNode;

typedef struct queue_t{
    Message** head;
    size_t size;
    size_t capacity;
    size_t front;
} MessageQueue;

/**
    *   @brief segments a file into multiple messages
    *   This function reads a file and segments it into multiple messages,
    *   each containing a portion of the file's data. The first message contains
    *   metadata about the file, including its name. Subsequent messages contain
    *   chunks of the file's data, each with a sequence number for ordering.
    *   The messages are stored in a dynamically allocated message queue.
    *   @param filename: the name of the file to be segmented
    *   @return a pointer to the message queue (dynamic array) containing the segmented messages
*/
MessageQueue* segment_file(const char* filename);


/**
    *   @brief assembles a file from received messages
    *   This function takes a queue of received messages and reassembles them into a single file.
    *   It expects the first message in the queue to contain metadata about the file, including its name.
    *   Subsequent messages contain chunks of the file's data, which are written to the output file in order.
    *   The function handles file opening, writing, and closing, and provides error handling for file operations.
    *   @return void
 */
void assemble_file();


/**
    *  @brief creates a new message queue
    *   This function initializes a new message queue with a specified initial capacity (default 10).
    *   It allocates memory for the queue structure and its internal array of message pointers.
    *   The queue is set to be empty initially, with size and front index set to zero.
    *   @return a pointer to the newly created message queue
 */
MessageQueue* create_message_queue();


/**
    *   @brief adds a message to the end of the queue
    *   This function adds a new message to the end of the specified message queue.
    *   If the queue is full, it dynamically resizes the internal array to accommodate more messages.
    *   The queue grows in capacity by doubling its size when needed.
    *   The function updates the size of the queue accordingly.
    *   @param queue: a pointer to the message queue where the message will be added
    *   @param msg: a pointer to the message to be added to the queue
    *   @return void
 */
void enqueue_message(MessageQueue* queue, Message* msg);


/**
    *   @brief synchronizes the receiver's message queue with metadata (sync queue with sender's queue)
    *   This function initializes or updates the receiver's message queue based on a metadata message.
    *   It checks if the provided message is valid and contains the appropriate metadata flag.
    *   If the receiver's message queue is not already created, it initializes a new queue.
    *   The function sets the queue's capacity based on the data length of the metadata message,
    *   resets the front index and size of the queue, and enqueues the metadata message as the first entry.
    *   @param msg: a pointer to the metadata message used for synchronization
    *   @return void
 */
void sync_message_queue(Message* msg);


/**
    *  @brief removes and returns the message at the front of the queue
    *   This function returns the message at the front of the specified message queue, and move pointer to the next message.
    *   If the queue is empty (i.e., the front index is greater than or equal to the size),
    *   it returns NULL to indicate that there are no messages to dequeue.
    *   The function increments the front index to point to the next message in the queue.
    *   @param queue: a pointer to the message queue from which a message will be dequeued
    *   @return a pointer to the dequeued message, or NULL if the queue is empty
 */
Message* dequeue_message(MessageQueue* queue);

