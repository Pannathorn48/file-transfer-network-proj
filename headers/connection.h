// Basic message type identifiers
#define MSG_ACK 1
#define MSG_NACK 2
#define MSG_DATA 3

#include <stdint.h>

struct message {
    uint8_t msg_type;
    char data[1024];
}__attribute__((packed));


/**
 * @brief Checks a message for an ACK or NACK pattern.
 *
 * This function searches the input string for patterns indicating
 * an Acknowledge (ACK) or Negative Acknowledge (NACK).
 *
 * @param str :  The input string to be checked.
 * @return Returns:
 *   - 0 if an ACK is found.
 *   - 1 if a NACK is found.
 *   - -1 if neither an ACK nor a NACK is found.
 */
short validate_header(struct message* msg);


/**
 * @brief Create new socket with given port
 *
 * This function check port number (invalid or in-use)
 * and create new socket with valid port only
 *
 * @param port : port number as a string
 * @param server : server infomation struct
 * @param client : client infomation struct
 * @return Returns:
 *   - negative number if an error occur
 *   -A socket file descriptor (positive number)
 */
 int new_connection(char* port , struct sockaddr_in* server);
