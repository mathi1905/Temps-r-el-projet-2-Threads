#include "GameServer.h"
#include <iostream>
GameServer::GameServer(int port){
    this->port = port;
    this->port_to_give_receive = port+1;
    this->port_to_give_send = port+1001;
    this->status = NOT_CONFIGURED;

}
int GameServer::initialize(){
    if(this->status==READY)
        return ALREADY_READY;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1){
        return SOCKET_ERROR;
    }
	int tru = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &tru, sizeof(int));
    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    serveraddr.sin_port = htons(this->port);
    if (bind(sock, (const struct sockaddr*)&serveraddr, sizeof(serveraddr))<0){
        return BIND_ERROR;
    }
    if(listen(sock, LISTEN_CLIENTS_QUEUE_SIZE)<0){
        return LISTEN_ERROR;
    }
    this->status = READY;
    this->sock.socket = sock;
    this->sock.addr = serveraddr;
    return OK;

}

int GameServer::acceptClient(GameClient* gameClient){
    if(this->status != READY){
        return NOT_READY;
    }    
    socklen_t len;
    int new_socket = accept(this->sock.socket, (struct sockaddr*)&this->sock.addr, &len);
    if(new_socket == -1){
        return ACCEPT_ERROR;
    }
    struct sockaddr_in client_addr_send, client_addr_receiver;
    memset(&client_addr_send, 0, sizeof(client_addr_send));
    memset(&client_addr_receiver, 0, sizeof(client_addr_receiver));
    char buffer[MAXLINE];
    int n = read(new_socket, &buffer, MAXLINE);
    if(n<=0){
        if(n==0){
            return SOCKET_SHUTDOWN;
        }
        else{
            close(new_socket);
            return READ_ERROR;
        }
    }
    buffer[n] = '\0';
    if(strcmp(buffer, "JOIN")==0){
        std::string to_send = std::to_string(this->port_to_give_send)+";"+std::to_string(this->port_to_give_receive);
        if(send(new_socket, to_send.c_str(), to_send.length(),0)<0){
            close(new_socket);
            return SEND_ERROR;
        }
        close(new_socket);
        client_addr_send.sin_port = htons(port_to_give_send);
        client_addr_receiver.sin_port = htons(port_to_give_receive);
        client_addr_receiver.sin_addr.s_addr = INADDR_ANY;
        int sock_receiver = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock_receiver==-1){
            return SOCKET_ERROR;
        }
        if(bind(sock_receiver, (const struct sockaddr*)&client_addr_receiver, sizeof(client_addr_receiver))<0){
            return BIND_ERROR;
        }
        // Test pour le sock send
        int sock_send = socket(AF_INET, SOCK_DGRAM, 0);
        if(sock_send==-1){
            return SOCKET_ERROR;
        }
        mysocket ms_send;
        ms_send.socket = sock_send;
        ms_send.addr = client_addr_send;
        mysocket ms_receiver;
        ms_receiver.socket = sock_receiver;
        ms_receiver.addr = client_addr_receiver;

        //gameClient = new GameClient(ms_receiver, ms_send);
        gameClient->setReceiveSocket(ms_receiver);
        gameClient->setSendSocket(ms_send);
        gameClient->setClientReady();
        this->port_to_give_receive++;
        this->port_to_give_send++;
        return OK;

    }
    return ERROR;


}
GameServer::~GameServer(){
	if(this->status == READY){
		close(this->sock.socket);
	}
}
