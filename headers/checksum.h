// headers/checksum.h

#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <stdint.h>
#include <netinet/in.h>
#include "connection.h"

// Corrected function signatures to take a pointer to struct message
void set_message_checksum(struct message *msg);

int validate_message_checksum(struct message *msg, int received_len);

#endif