#pragma once
#include "GameClient.h"
#define LISTEN_CLIENTS_QUEUE_SIZE 5
class GameServer{
    private:
        int port;
        int port_to_give_receive, port_to_give_send;
        int status;
        mysocket sock;
        
    public:
        GameServer(int);
        int initialize();
        int acceptClient(GameClient*);
	~GameServer();
};
