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