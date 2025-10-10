#ifndef NETWORK_LIB_H
#define NETWORK_LIB_H

#if defined(_WIN32) || defined(_WIN64)
  // Windows sockets
  #define GET_SOCKET_ERROR() WSAGetLastError()
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef SOCKET socket_t;
  #define CLOSESOCKET closesocket
#else
  // POSIX sockets
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <sys/types.h>
  typedef int socket_t;
  #define CLOSESOCKET close
#endif

#endif