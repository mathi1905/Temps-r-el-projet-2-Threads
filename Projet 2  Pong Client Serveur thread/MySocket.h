#pragma once
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
typedef struct mysocket{
    int socket;
    struct sockaddr_in addr;
}mysocket;