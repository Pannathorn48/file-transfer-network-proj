#ifdef _WIN32
    #include <windows.h>
    long long current_time_ms() {
        FILETIME ft;
        ULARGE_INTEGER uli;
        GetSystemTimeAsFileTime(&ft);
        uli.LowPart = ft.dwLowDateTime;
        uli.HighPart = ft.dwHighDateTime;
        return uli.QuadPart / 10000; 
    }
#else
    #include <sys/time.h>
    long long current_time_ms() {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000LL + tv.tv_usec / 1000;
    }
#endif

#include <stdarg.h>

#define PINK "175"
#define RED "31"
#define BLU "34"
#define CYN "36"
#define GRN "32"
#define L_GRN "118"
#define BRED "31"
#define ORG "214"
#define YLW "226"

void printLnColor(char* color, char* format, ...) {
    va_list args;
    va_start(args, format);
    
    printf("\033[38;5;%sm", color);
    vprintf(format, args);
    printf("\033[m\n");
    
    va_end(args);
}