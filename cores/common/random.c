#ifndef RANDOM_H
#include "random.h"
#define RANDOM_H
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

bool random_percent(double percent) {
    srand(time(NULL));

    if (percent <= 0.0) return false;
    if (percent >= 100.0) return true;
    
    double random_value = ((double)rand() / RAND_MAX) * 100.0;
    
    return random_value < percent;
}