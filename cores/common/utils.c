#include "utils.h"

#ifdef _WIN32
    long long current_time_ms() {
        FILETIME ft;
        ULARGE_INTEGER uli;
        GetSystemTimeAsFileTime(&ft);
        uli.LowPart = ft.dwLowDateTime;
        uli.HighPart = ft.dwHighDateTime;
        return uli.QuadPart / 10000; 
    }
#else
    long long current_time_ms() {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000LL + tv.tv_usec / 1000;
    }
#endif


void set_recv_timeout(int sock, int timeout_msec) {
    #ifdef _WIN32
        DWORD timeout = timeout_msec; 
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
            perror("setsockopt(SO_RCVTIMEO) failed");
        }
    #else
        struct timeval tv;
        if (timeout_msec == 0) {
            tv.tv_sec  = 0;
            tv.tv_usec = 0;
        } else {
            tv.tv_sec  = timeout_msec / 1000;
            tv.tv_usec = (timeout_msec % 1000) * 1000;
        }
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            perror("setsockopt(SO_RCVTIMEO) failed");
        }
    #endif
}


void printLnColor(char* color, char* format, ...) {
    va_list args;
    va_start(args, format);
    
    printf("\033[38;5;%sm", color);
    vprintf(format, args);
    printf("\033[m\n");
    
    va_end(args);
}