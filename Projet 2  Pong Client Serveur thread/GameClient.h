#pragma once
#include <string.h>
#include <sstream>
#include "MySocket.h"
#include "Status.h"

#define MAXLINE 1024
class GameClient{
    private:
        mysocket receive_socket;
        mysocket send_socket;
        int client_status;
        char buffer[MAXLINE];
    public:
        GameClient();
        GameClient(mysocket, mysocket);
        ~GameClient();
        int join(char*, int);
        int send(char*);
        int receive(char*);
	    int receiveNonBlocking(char*, int);//nouveau
        void setReceiveSocket(mysocket);
        void setSendSocket(mysocket);
        void setClientReady();
};
