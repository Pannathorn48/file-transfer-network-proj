#include "utils.h"

#ifdef _WIN32
/**
 * current_time_ms (Windows version): Returns the current system time in milliseconds.
 * Uses Windows API GetSystemTimeAsFileTime, which returns time in 100-nanosecond intervals
 * since January 1, 1601. Divides by 10000 to convert to milliseconds.
 */
long long current_time_ms() {
    FILETIME ft;
    ULARGE_INTEGER uli;
    GetSystemTimeAsFileTime(&ft);          // Get system time as FILETIME
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return uli.QuadPart / 10000;           // Convert 100-ns intervals to milliseconds
}
#else
/**
 * current_time_ms (POSIX version): Returns current system time in milliseconds.
 * Uses gettimeofday(), which returns seconds and microseconds since epoch.
 */
long long current_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);               // Get current time
    return tv.tv_sec * 1000LL + tv.tv_usec / 1000; // Convert to milliseconds
}
#endif

/**
 * set_recv_timeout: Sets the receive timeout for a socket.
 * @sock: socket descriptor
 * @timeout_msec: timeout in milliseconds (0 means no timeout)
 *
 * Works differently on Windows and POSIX:
 * - Windows: uses DWORD and setsockopt with SO_RCVTIMEO
 * - POSIX: uses struct timeval and setsockopt with SO_RCVTIMEO
 */
void set_recv_timeout(int sock, int timeout_msec) {
    #ifdef _WIN32
        DWORD timeout = timeout_msec; 
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
            perror("setsockopt(SO_RCVTIMEO) failed");
        }
    #else
        struct timeval tv;
        if (timeout_msec == 0) {              // No timeout
            tv.tv_sec  = 0;
            tv.tv_usec = 0;
        } else {                              // Convert milliseconds to seconds + microseconds
            tv.tv_sec  = timeout_msec / 1000;
            tv.tv_usec = (timeout_msec % 1000) * 1000;
        }
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            perror("setsockopt(SO_RCVTIMEO) failed");
        }
    #endif
}

/**
 * printLnColor: Prints formatted text in terminal with a specific color, followed by a newline.
 * @color: terminal color code string (for ANSI 256-color)
 * @format: printf-style format string
 * Supports variable arguments like printf.
 *
 * Usage example:
 *   printLnColor("196", "Error: %s", "Something went wrong");
 */
void printLnColor(char* color, char* format, ...) {
    va_list args;
    va_start(args, format);
    
    printf("\033[38;5;%sm", color);   // Set text color using ANSI escape
    vprintf(format, args);            // Print formatted string
    printf("\033[m\n");               // Reset color and add newline
    
    va_end(args);
}
