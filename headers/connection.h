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
short has_NACK(const char* str);


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
