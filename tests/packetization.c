#include <stdio.h>
#include <stdlib.h>
#include "../cores/common/message.c"

void simulate_transmission(MessageQueue* senderMessages) {
    // Simulate sending and receiving messages
    // the parameter queue is the sender's message queue (from segment_file)
    Message* msg;
    msg = dequeue_message(senderMessages);
    // sync_message_queue(msg);
    if (receiverMessages == NULL) {
        receiverMessages = create_message_queue();
    }
    receiverMessages->size = 0;
    receiverMessages->front = 0;
    enqueue_message(receiverMessages, msg);
    while ((msg = dequeue_message(senderMessages)) != NULL) {
        enqueue_message(receiverMessages, msg);
    }
    printf("senderMessages' size: %zu\n", senderMessages->size);
    printf("receiverMessages' size: %zu\n", receiverMessages->size);
    printf("senderMessages' capacity: %zu\n", senderMessages->capacity);
    printf("receiverMessages' capacity: %zu\n", receiverMessages->capacity);
}

void simulate_send_and_receive_in_with_no_connection() {
    senderMessages = segment_file("Salary.pdf");
    simulate_transmission(senderMessages);
    // assemble_file("reconstructed.pdf", messages);
    assemble_file();
    
    senderMessages = segment_file("otonari_virgo.jpg");
    simulate_transmission(senderMessages);
    // assemble_file("newphoto.png", messages);
    assemble_file();
}

int main() {
    simulate_send_and_receive_in_with_no_connection();
    return 0;
}
