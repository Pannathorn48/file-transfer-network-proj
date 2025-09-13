#include "message.h"
#include <string.h>

MessageQueue* create_message_queue() {
    MessageQueue* queue = (MessageQueue*)malloc(sizeof(MessageQueue));
    queue->head = (Message**)malloc(INITIAL_QUEUE_CAPACITY * sizeof(Message*));
    queue->size = 0;
    queue->front = 0;
    queue->capacity = INITIAL_QUEUE_CAPACITY;
    return queue;
}

void enqueue_message(MessageQueue* queue, Message* msg) {
    size_t oldCapacity = queue->capacity;
    if (queue->size >= queue->capacity) {
        queue->capacity *= 2;
        queue->head = (Message**)realloc(queue->head, queue->capacity * sizeof(Message*));
        if (!queue->head) {
            perror("Failed to reallocate memory for message queue");
            exit(1);
        }
        memset(&queue->head[oldCapacity], 0, (queue->capacity - oldCapacity) * sizeof(Message*));
    }
    queue->head[queue->size++] = msg;
}

Message* dequeue_message(MessageQueue* queue) {
    if (queue->front >= queue->size) {
        return NULL;
    }
    Message* msg = queue->head[queue->front++];
    // queue->front++;
    return msg;
}

MessageQueue* senderMessages = NULL;
MessageQueue* recieverMessages = NULL;


MessageQueue* segment_file(const char* filename) {
    char buffer[BUFFER_SIZE];
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        return NULL;
    }
    if (senderMessages == NULL) {
        senderMessages = create_message_queue();
    }
    senderMessages->size = 0;
    senderMessages->front = 0;
    Message* metadata = (Message*)malloc(sizeof(Message));
    metadata->flags = 0x10000000; // Metadata flag
    // metadata->dataLen = strlen(filename) + 4; // +1 for null terminator and 3 for [c]
    // memcpy(metadata->data, filename, metadata->dataLen);
    sprintf(metadata->data, "[c]%s", filename);
    enqueue_message(senderMessages, metadata);
    Message* tmp = NULL;
    size_t bytesRead;
    uint32_t seqNum = 0x0;
    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        tmp = (Message*)malloc(sizeof(Message));
        if (!tmp) {
            perror("Failed to allocate memory for message");
            fclose(file);
            return NULL;   
        }
        tmp->flags = seqNum & 0xFFFFFF; // Store only the lower 24 bits
        memcpy(tmp->data, buffer, bytesRead);
        tmp->dataLen = bytesRead;
        enqueue_message(senderMessages, tmp);
        seqNum++;
    }
    // (senderMessages->head)[0]->dataLen = senderMessages->capacity;
    fclose(file);
    return senderMessages;
}
void sync_message_queue(Message* msg) {
    if (!msg || !(msg->flags & 0x10000000)) {
        perror("Invalid metadata message for sync");
        return;
    }

    if (recieverMessages == NULL) {
        recieverMessages = create_message_queue();
    }

    recieverMessages->capacity = msg->dataLen;

    // while(recieverMessages->size >= recieverMessages->capacity) {
    //     recieverMessages->capacity *= 2;
    //     recieverMessages->head = (Message*)realloc(recieverMessages->head, recieverMessages->capacity * sizeof(Message));
    // }
    recieverMessages->front = 0;
    recieverMessages->size = 0;
    enqueue_message(recieverMessages, msg);

}

void assemble_file() {
    Message* msg;
    recieverMessages->front = 0;
    msg = dequeue_message(recieverMessages);
    if (!msg || !(msg->flags & 0x10000000)) {
        perror("Invalid metadata message for assembling file");
        return;
    }
    const char* filename = msg->data;
    FILE *file = fopen(filename, "a");
    if (!file) {
        perror("Failed to open file for writing");
        return;
    }
    printf("Reassembling file from %zu messages...\n", recieverMessages->size - 1);
    printf("Queue's capacity: %zu\n", recieverMessages->capacity);
    while ((msg = dequeue_message(recieverMessages)) != NULL) {
        fwrite(msg->data, 1, msg->dataLen, file);
        // free(msg);
    }
    fclose(file);
    recieverMessages->size = 0;
    recieverMessages->front = 0;
    printf("File %s reassembled successfully.\n", filename);
}