#ifndef CONNECTION_H
#define CONNECTION_H


/* ============================================================================
 * 32-bit Header Layout (field: header_flags)
 *
 *  Bit indices (MSB -> LSB):
 *   31        30 29        28        27 26 25 24        23 ................. 0
 *  +-----------+-----------+---------+------------------+--------------------+
 *  | META_DATA | ACK CODE  |  FIN    |   STATUS CODE    |   SEQUENCE NUMBER  |
 *  |   (1b)    |  (2 bits) |  (1b)   |     (4 bits)     |      (24 bits)     |
 *  +-----------+-----------+---------+------------------+--------------------+
 *
 *  Field Groups:
 *    META_DATA (bit 31)          : 1 = this packet carries only metadata (e.g., filename, total parts)
 *    ACK CODE (bits 30..29)      : 00 = NONE, 01 = ACK, 10 = NACK, 11 = RESERVED (future)
 *    FIN (bit 28)                : 1 = final packet / stream completion
 *    STATUS CODE (bits 27..24)   : 4-bit application status / error code (0-15)
 *    SEQUENCE NUMBER (23..0)     : 24-bit unsigned sequence value (0 - 16,777,215)
 *
 *  Packing Order (MSB->LSB): | META_DATA | ACK | FIN | STATUS | SEQ |
 *
 *  Use the macros below to set/extract each sub-field safely.
 * ============================================================================ */

#include <stdint.h>

/* Bit masks */
#define HDR_FLAG_META        0x80000000u  /* bit 31 */
#define HDR_ACK_MASK         0x60000000u  /* bits 30-29 */
#define HDR_FLAG_FIN         0x10000000u  /* bit 28 */
#define HDR_STATUS_MASK      0x0F000000u  /* bits 27-24 */
#define HDR_SEQ_MASK         0x00FFFFFFu  /* bits 23-0  */
#define HDR_ERR_MASK         0x0F000000u  /* bits 23-0  */

/* Error flag */
#define HDR_FLAG_SUCC        0x00000000u /* 0000 */
#define HDR_ERR_FNF          0x08000000u /* 1000 */
#define HDR_ERR_SUCC         0x09000000u /* 1001 */
#define HDR_ERR_NPM          0x0A000000u /* 1010 */

/* Shifts */
#define HDR_STATUS_SHIFT     24

/* ACK codes (masked by HDR_ACK_MASK) */
#define HDR_ACK_NONE         0x00000000u  /* 00 */
#define HDR_ACK_ACK          0x20000000u  /* 01 */
#define HDR_ACK_NACK         0x40000000u  /* 10 */
#define HDR_ACK_RESERVED     0x60000000u  /* 11 (unused) */

/* Helper macros */
#define HDR_GET_META(h)      (((h) & HDR_FLAG_META) != 0u)
#define HDR_SET_META(h)      do { (h) |= HDR_FLAG_META; } while(0)
#define HDR_CLR_META(h)      do { (h) &= ~HDR_FLAG_META; } while(0)

#define HDR_GET_ACK(h)       ((h) & HDR_ACK_MASK)
#define HDR_SET_ACK(h,a)     do { (h) = ((h) & ~HDR_ACK_MASK) | ((a) & HDR_ACK_MASK); } while(0)

#define HDR_GET_FIN(h)       (((h) & HDR_FLAG_FIN) != 0u)
#define HDR_SET_FIN(h)       do { (h) |= HDR_FLAG_FIN; } while(0)
#define HDR_CLR_FIN(h)       do { (h) &= ~HDR_FLAG_FIN; } while(0)

#define HDR_GET_STATUS(h)    ( (uint8_t)(((h) & HDR_STATUS_MASK) >> HDR_STATUS_SHIFT) )
#define HDR_SET_STATUS(h,s)  do { (h) = ((h) & ~HDR_STATUS_MASK) | (((uint32_t)(s) & 0x0Fu) << HDR_STATUS_SHIFT); } while(0)

#define HDR_GET_SEQ(h)       ( (h) & HDR_SEQ_MASK )
#define HDR_SET_SEQ(h,seq)   do { (h) = ((h) & ~HDR_SEQ_MASK) | ((uint32_t)(seq) & HDR_SEQ_MASK); } while(0)

struct message {
    uint16_t checksum;
    uint32_t flags; /* Packed 32-bit header described above */
    char data[1024];       /* Payload buffer */
    unsigned short data_length; /* Number of valid bytes in data */
} __attribute__((packed));

/* Example unpack pattern:
 *  uint8_t  status = HDR_GET_STATUS(msg->header_flags);
 *  uint32_t seq    = HDR_GET_SEQ(msg->header_flags);
 *  int is_meta     = HDR_GET_META(msg->header_flags);
 */

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


 int init_client(char *ip , char* port);


#endif