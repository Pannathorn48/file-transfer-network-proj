#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include "connection.h"
#include "error.h"
#include "random.h"

#ifndef COLOR_H
#define COLOR_H
#define RED "\e[0;31m"
#define BLU "\e[0;34m"
#define reset "\e[0m"
#define CYN "\e[0;36m"
#define GRN "\e[0;32m"
#define BRED "\e[1;31m"
#endif


// Generic function to calculate one's complement checksum
uint16_t calculate_checksum(const void *data, size_t len) {
    uint32_t sum = 0;
    const uint16_t *p = (const uint16_t *)data;

    while (len > 1) {
        sum += *p++;
        len -= 2;
    }

    if (len == 1) {
        sum += *(const uint8_t *)p;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)~sum;
}

// Function to set the checksum on a message without a pseudo-header
void set_message_checksum(struct message *msg) {
    // Temporarily zero out the checksum field to calculate it
    msg->checksum = 0;

    // Calculate checksum over the entire message structure (flags + data)
    uint16_t calculated_checksum = calculate_checksum((const void *)&msg->flags, sizeof(msg->flags) + msg->data_length);
    // if (random_percent(3)){
    //     // Simulate a checksum error with a 3% probability
    //     printf("Simulating checksum error for packet with sequence number: %d\n", HDR_GET_SEQ(msg->flags));
    //     msg->data[0] ^= 0xFF; // Corrupt the first byte of data
    // }

    msg->checksum = calculated_checksum;
}

// Function to validate the checksum of a received message
int validate_message_checksum(struct message *msg, int received_len) {
    // Store the original checksum
    uint16_t original_checksum = msg->checksum;

    // Temporarily zero out the checksum field to calculate it
    msg->checksum = 0;

    // Calculate the checksum over the flags and data
    uint16_t calculated_checksum = calculate_checksum((const void *)&msg->flags, received_len - sizeof(msg->checksum));

    // Restore the original checksum
    msg->checksum = original_checksum;

    // Compare the calculated checksum with the original
    if (calculated_checksum != original_checksum){
        printf("%sChecksum validation failed! Packet dropped.%s\n", RED , reset);
    }
    return (calculated_checksum == original_checksum) ? 0 : ERR_CHECKSUM_FAIL;
}