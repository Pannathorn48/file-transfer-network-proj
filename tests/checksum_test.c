// tests/checksum_test.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../headers/checksum.h" // <-- ADD THIS
#include "../headers/message.h"  // <-- ADD THIS

void test_checksum_integrity() {
    printf("--- Running Checksum Integrity Test ---\n");

    // Test Case 1: Valid checksum
    struct message msg1;
    memset(&msg1, 0, sizeof(msg1));
    strcpy(msg1.data, "Hello, World!");
    msg1.data_length = strlen(msg1.data);
    
    // Set checksum
    set_message_checksum(&msg1);
    
    // Validate checksum
    int result1 = validate_message_checksum(&msg1, sizeof(msg1.checksum) + sizeof(msg1.flags) + msg1.data_length);
    printf("Test 1 (Valid Checksum): %s\n", (result1 == 0) ? "PASSED" : "FAILED");
    
    // Test Case 2: Invalid checksum (corrupt data)
    struct message msg2;
    memset(&msg2, 0, sizeof(msg2));
    strcpy(msg2.data, "Hello, World!");
    msg2.data_length = strlen(msg2.data);

    // Set checksum and then modify data
    set_message_checksum(&msg2);
    msg2.data[0] = 'J'; // Corrupt the data
    
    // Validate checksum (should fail)
    int result2 = validate_message_checksum(&msg2, sizeof(msg2.checksum) + sizeof(msg2.flags) + msg2.data_length);
    printf("Test 2 (Invalid Checksum): %s\n", (result2 != 0) ? "PASSED" : "FAILED");
    
    printf("---------------------------------------\n");
}

int main() {
    test_checksum_integrity();
    return 0;
}