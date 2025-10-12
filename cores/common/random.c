#ifndef RANDOM_H
#include "random.h"
#define RANDOM_H
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

/**
 * random_percent: Returns true with a given probability (percentage)
 * @percent: probability threshold (0.0 - 100.0)
 *
 * This function simulates randomness based on a given percentage.
 * - If percent <= 0.0, it always returns false (never occurs)
 * - If percent >= 100.0, it always returns true (always occurs)
 * - Otherwise, generates a random number between 0.0 and 100.0
 *   and returns true if the random number is less than the given percentage.
 * 
 * Useful for simulating packet loss, corruption, or duplication in network experiments.
 */
bool random_percent(double percent) {

    if (percent <= 0.0) return false;   // Always false if threshold <= 0
    if (percent >= 100.0) return true;  // Always true if threshold >= 100
    
    // Generate random value in range [0, 100)
    double random_value = ((double)rand() / RAND_MAX) * 100.0;

    // Print the generated value and threshold for debugging
    printf("Random value: %.2f, Threshold: %.2f\n", random_value, percent); 

    // Return true if the random value is below the percentage threshold
    return random_value < percent;
}
