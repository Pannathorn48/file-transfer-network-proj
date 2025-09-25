#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "connection.h"
#include "message.h"
#include "checksum.h"

// Helper function to send a message without waiting for an ACK
int send_message(struct message *msg, int sock, struct sockaddr_in dest_addr)
{
    unsigned int dest_len = sizeof(dest_addr);
    size_t packet_size = sizeof(msg->checksum) + sizeof(msg->flags) + msg->data_length;

    printf("Sending packet with flags: %x, size: %zu\n", msg->flags, packet_size);

    if (sendto(sock, msg, packet_size, 0, (struct sockaddr *)&dest_addr, dest_len) < 0)
    {
        perror("sendto failed");
        return 1;
    }
    return 0;
}