// cores/common/checksum.c
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include "connection.h"
#include "error.h"

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

// Helper function to calculate the pseudo-header sum
uint32_t add_pseudo_header_sum(struct sockaddr_in *src_addr, struct sockaddr_in *dest_addr, uint16_t udp_len) {
    uint32_t sum = 0;
    
    // Add source and destination IP addresses
    sum += (src_addr->sin_addr.s_addr >> 16) & 0xFFFF;
    sum += src_addr->sin_addr.s_addr & 0xFFFF;
    sum += (dest_addr->sin_addr.s_addr >> 16) & 0xFFFF;
    sum += dest_addr->sin_addr.s_addr & 0xFFFF;

    // Add protocol and UDP length
    sum += htons(IPPROTO_UDP);
    sum += htons(udp_len);

    return sum;
}

// Function to set the checksum on a message
void set_message_checksum(struct message *msg, struct sockaddr_in *src_addr, struct sockaddr_in *dest_addr) {
    // The length of the entire UDP datagram (flags + data)
    uint16_t total_udp_len = sizeof(msg->flags) + msg->data_length;

    // Calculate the pseudo-header sum
    uint32_t sum = add_pseudo_header_sum(src_addr, dest_addr, total_udp_len);

    // Temporarily zero out the checksum field to calculate it
    msg->checksum = 0;

    // Calculate checksum over the flags and data
    sum += calculate_checksum((const void *)&msg->flags, sizeof(msg->flags));
    sum += calculate_checksum((const void *)&msg->data, msg->data_length);

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    msg->checksum = (uint16_t)~sum;
}

// Function to validate the checksum of a received message
int validate_message_checksum(struct message *msg, int received_len, struct sockaddr_in *src_addr, struct sockaddr_in *dest_addr) {
    // Store the original checksum
    uint16_t original_checksum = msg->checksum;

    // Temporarily zero out the checksum field to calculate it
    msg->checksum = 0;

    // The length of the UDP payload (flags + data)
    uint16_t total_udp_len = received_len - sizeof(msg->checksum);

    // Calculate the pseudo-header sum
    uint32_t sum = add_pseudo_header_sum(src_addr, dest_addr, total_udp_len);

    // Calculate checksum over flags and data
    sum += calculate_checksum((const void *)&msg->flags, sizeof(msg->flags));
    sum += calculate_checksum((const void *)&msg->data, msg->data_length);

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    uint16_t calculated_checksum = (uint16_t)~sum;

    // Restore the original checksum
    msg->checksum = original_checksum;

    // Compare the calculated checksum with the original
    return (calculated_checksum == original_checksum) ? 0 : ERR_CHECKSUM_FAIL;
}