#include "GameClient.h"
#include <iostream>
GameClient::GameClient(){
    this->client_status = NOT_CONFIGURED;
}
GameClient::GameClient(mysocket receive_socket, mysocket send_socket){
    this->receive_socket = receive_socket;
    this->send_socket = send_socket;
    this->client_status = READY;
}
GameClient::~GameClient(){
    if(this->client_status==READY){
        close(this->receive_socket.socket);
        close(this->send_socket.socket);
    }
}

int GameClient::join(char* addr, int port){
    if (this->client_status == READY){
        return ALREADY_READY;
    }
    int sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_tcp == -1){
        return SOCKET_ERROR;
    }
    struct sockaddr_in sin_tcp;
    memset(&sin_tcp, 0, sizeof(sin_tcp));
    sin_tcp.sin_family = AF_INET;
    sin_tcp.sin_port = htons(port);
    int status=inet_pton(AF_INET, addr, &sin_tcp.sin_addr);
    if (status<=0)
    {
        if(status==0) return INET_PTON_ERROR_ADDR_NOT_VALID;
        else return INET_PTON_ERROR;
    }
    if(connect(sock_tcp, (struct sockaddr*)&sin_tcp, sizeof(sin_tcp))<0){
        close(sock_tcp);
        return CONNECT_ERROR;
    }
    const char* join = "JOIN";
    if(::send(sock_tcp, join, strlen(join), 0)==-1){
        close(sock_tcp);
        return SEND_ERROR; // A changer pour une gestion des erreurs plus fines
    }
    int valcount = read(sock_tcp, this->buffer, MAXLINE);
    if(valcount <=0){
        close(sock_tcp);
        if(valcount==0) return SOCKET_SHUTDOWN;
        else return READ_ERROR; 
    }
    this->buffer[valcount] = '\0';
    std::stringstream stream;
    stream=std::stringstream(std::string(buffer));; 
    int port_receive, port_to_send;
    char delimiter;
    stream >> port_receive >> delimiter>>port_to_send;
    if(port_receive==-1 || port_to_send == -1){
        return ERROR;
    }
    // Reception
    struct sockaddr_in sockaddr_recept;
    int sock_recept = socket(AF_INET,SOCK_DGRAM, 0);
    if(sock_recept==-1){
        return SOCKET_ERROR;
    }
    memset(&sockaddr_recept, 0, sizeof(sockaddr_recept));
    sockaddr_recept.sin_family= AF_INET;
    sockaddr_recept.sin_port = htons(port_receive);
    sockaddr_recept.sin_addr.s_addr = INADDR_ANY;
    this->receive_socket.socket = sock_recept;
    this->receive_socket.addr = sockaddr_recept;
    if(bind(sock_recept, (const struct sockaddr *) &sockaddr_recept, sizeof(sockaddr_recept))<0){
        close(sock_recept);
        return BIND_ERROR;
    }
    // ENvoie
    struct sockaddr_in sockaddr_send;
    int sock_send = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock_send==-1){
        close(sock_tcp);
        close(sock_recept);
        return SOCKET_ERROR;
    }
    memset(&sockaddr_send, 0, sizeof(sockaddr_send));
    sockaddr_send.sin_family = AF_INET;
    sockaddr_send.sin_port = htons(port_to_send);
    sockaddr_send.sin_addr.s_addr = INADDR_ANY;
    this->send_socket.socket = sock_send;
    this->send_socket.addr = sockaddr_send;

    this->client_status = READY;
    close(sock_tcp);

    return OK;

}

int GameClient::send(char* msg){
    if(this->client_status!=READY){
        return NOT_READY;
    }
    if(sendto(this->send_socket.socket, msg, strlen(msg), MSG_CONFIRM, (const struct sockaddr*)&this->send_socket.addr, sizeof(this->send_socket.addr))==-1){
        return SEND_ERROR;
    }
    return OK;
}
int GameClient::receive(char* msg_received){
    int n;
    socklen_t len;
    len = sizeof(this->receive_socket.addr);
    n = recvfrom(this->receive_socket.socket,(char*)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *)&this->receive_socket.addr,&len);
    if(n==-1){
        return RECV_ERROR;
    }
    if(n==0){
        return RECV_EMPTY;
    }
    this->buffer[n] = '\0';
    strcpy(msg_received, this->buffer);
    return OK;
}
//nouveau
int GameClient::receiveNonBlocking(char* msg, int microseconds)
{
	fd_set reading;
	struct timeval timeout;
	int rc, max_sock;

	FD_ZERO(&reading);

	int r;
	int max_sd = this->receive_socket.socket+1;

	timeout.tv_sec = 0;
	timeout.tv_usec = microseconds;
	FD_SET(this->receive_socket.socket, &reading);


	rc = select(FD_SETSIZE, &reading, NULL, NULL, &timeout);
	if(rc<0)
	{
		printf("ERROR SELECT GC");
		return ERROR_SELECT;
	}
	if(rc == 0){
		return TIMEOUT;
	}
	if(FD_ISSET(this->receive_socket.socket, &reading))
	{
		// Un message est disponible
		return this->receive(msg);
	}
	return ERROR;
	
}

void GameClient::setReceiveSocket(mysocket receive_sock){
    this->receive_socket = receive_sock;
}

void GameClient::setSendSocket(mysocket send_sock){
    this->send_socket = send_sock;
}
void GameClient::setClientReady(){
    this->client_status = READY;
}
