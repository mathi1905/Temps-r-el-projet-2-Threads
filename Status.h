#pragma once
#define CONFIGURED 1
#define READY 2
#define OK 3
#define ERROR -1
#define NOT_CONFIGURED -2
#define ALREADY_READY -3
#define NOT_READY -4
#define TIMEOUT -5

// Network programming error
#define BIND_ERROR -5
#define SOCKET_ERROR -6
#define INET_PTON_ERROR_ADDR_NOT_VALID -7
#define INET_PTON_ERROR -8
#define CONNECT_ERROR -9
#define SEND_ERROR -10
#define ACCEPT_ERROR -11
#define SOCKET_SHUTDOWN -12
#define READ_ERROR -13
#define RECV_EMPTY -14
#define RECV_ERROR -15
#define LISTEN_ERROR -16
#define ERROR_SELECT -17


