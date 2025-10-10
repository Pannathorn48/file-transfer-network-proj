#ifndef UTILS_H
#define UTILS_H

#include <stdarg.h>
#include <stdio.h>
#include "network_lib.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#define PINK "175"
#define RED "31"
#define BLU "34"
#define CYN "36"
#define GRN "32"
#define L_GRN "118"
#define BRED "31"
#define ORG "214"
#define YLW "226"

void printLnColor(char* color, char* format, ...);

    
long long current_time_ms();

void set_recv_timeout(int sock, int timeout_msec);

#endif