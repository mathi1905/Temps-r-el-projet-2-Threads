#include <iostream>
#include <string>
#include <cstring>
#include <pthread.h>
#include <queue>
#include <SFML/Graphics.hpp>
#include <chrono>

#include "GameServer.h"
#include "bat.h"
#include "ball.h"
#include "Status.h"

using namespace std;

int windowWidth = 1024;
int windowHeight = 768;

int InitServer(GameServer& server);

int InitGame(Text& score,string fontPath , RectangleShape (&separators)[16]);

void HandleBall(Ball &ball, Bat &batC1, Bat &batC2, int &scoreC1, int &scoreC2);

int EventClient(Bat &batC1, Bat &batC2);

int SendScoreSeparator(Text& score, string fontPath, RectangleShape (&separators)[16],bool isClient1);

int SendDataToClient(GameClient *client, Ball &ball, Bat &batC1, Bat &batC2, int scoreC1, int scoreC2, bool isClient1);

void *FctThreadReceive(void *setting);
void handleThreadReceiveStatus(int status, char *Data, bool isClient1);
int receiveDataClient1(string& data);
int receiveDataClient2(string& data);

GameClient* client1=new GameClient();
GameClient* client2=new GameClient();

pthread_t threadRecv;

pthread_mutex_t mutexReceiveClient1 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condReceiveClient1 = PTHREAD_COND_INITIALIZER;
queue<string> receivedQueueClient1; 
bool receiveDataAvailableClient1=false;

pthread_mutex_t mutexReceiveClient2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condReceiveClient2 = PTHREAD_COND_INITIALIZER;
queue<string> receivedQueueClient2; 
bool receiveDataAvailableClient2=false;

bool erreurRcv = false;
pthread_mutex_t mutexErreurRcv = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condErreur = PTHREAD_COND_INITIALIZER;

int main()
{   /// Initialise le serveur avec 8080 comme port
    int port = 8080;

    GameServer server(port);
    int status= InitServer(server);
    if(status != OK)
    {
        cout << "(SERVEUR)ERREUR d'initialisation du serveur" << endl;
        return status;
    }

    //connexion client 1
    cout << "(SERVEUR)En attente du client 1" << endl;
    status = server.acceptClient(client1);
    if(status != OK)
    {
        cout << "(SERVEUR)ERREUR de connexion client " << endl;
        return status;
    }
    cout << "(SERVEUR)Client 1 connecté" << endl;

    //connexion client 2
    cout << "(SERVEUR)En attente du client 2" << endl;
    status = server.acceptClient(client2);
    if(status != OK)
    {
        cout << "(SERVEUR)ERREUR de connexion client " << endl;
        return status;
    }
    cout << "(SERVEUR)Client 2 connecté" << endl;

    int res=pthread_create(&threadRecv, NULL, FctThreadReceive,NULL);

    //création  raquette client 1
    Bat batC1 (0, windowHeight/2);
    //création  raquette client 2
    Bat batC2(windowWidth-batC1.getShape().getSize().x, windowHeight/2);
    //créer la balle
    Ball ball(windowWidth / 2, windowHeight/2);

    Text score;
    Font font;
    font.loadFromFile("OpenSans-Bold.ttf");
    score.setFont(font);
    score.setCharacterSize(75);
    score.setFillColor(sf::Color::White);
    score.setPosition(Vector2f((windowWidth/2)-100,0));

    // Création de la zone de séparation
    RectangleShape separators[16];
    int y_sepa = 0;
    for (int i = 0; i<16;i++){
        separators[i].setSize(Vector2f(20,32));
        separators[i].setPosition(Vector2f(windowWidth/2-10,y_sepa));

        y_sepa+=64;
    }
    
    const int tickRate = 32; // Taux de rafraîchissement en Hz
    const double tickPeriod_sec = 1.0 / (double)tickRate; // Durée d'une boucle en seconde (0.03125s pour 32Hz)

    int scoreC1 = 0;
    int scoreC2 = 0;
    
    //initialisation du jeu 
   status=InitGame(score, "OpenSans-Bold.ttf", separators);
    if(status != OK)
    {
        cout << "(SERVEUR)ERREUR d'initialisation du jeu" << endl;
        pthread_cancel(threadRecv); pthread_join(threadRecv, NULL);
        return status;
    }
    //démarrage de la balle
    ball.start();
    bool start = true;


//Boucle principale du jeu
    while (start==true)
    {   
        auto start = std::chrono::high_resolution_clock::now();//Démarre le chrono(capture le temps de début de la boucle)

        //On analyse les event des clients si il bouge la raquette ou pas
        status = EventClient(batC1, batC2);
        if (status != OK)
        {
            if (status == 99)
            {
                //cout << "(SERVEUR)Fin de connexion" << endl;
                break;
            }            
            pthread_cancel(threadRecv); pthread_join(threadRecv, NULL);
            return status;
        }

        // Gestion de la balle (collision, rebonds, etc.)
        HandleBall(ball, batC1, batC2, scoreC1, scoreC2);
        
        //envoie l'etat du jeu aux clients

        //On envoi les infos au client 1
        status= SendDataToClient(client1, ball, batC1, batC2, scoreC1, scoreC2, true);
        if(status != OK)
        {
            cout << "(SERVEUR)ERREUR d'envoi des positions ball et bats aux clients" << endl;
            pthread_cancel(threadRecv); pthread_join(threadRecv, NULL);
            return status;
        }
        //On envoi les infos au client 2
        status= SendDataToClient(client2, ball, batC1, batC2, scoreC1, scoreC2, false);
        if(status != OK)
        {
            cout << "(SERVEUR)ERREUR d'envoi des positions ball et bats aux clients" << endl;
            pthread_cancel(threadRecv); pthread_join(threadRecv, NULL);
            return status;
        }
        auto end = std::chrono::high_resolution_clock::now();//Capture le temps de fin de la boucle

        //Calcul le temps écoulé entre le début et la fin de la boucle
        std::chrono::duration<double, std::milli> TimeInterval_ms = end - start;

        //Si le temps écoulé est inférieur à la période de rafraîchissement, on attend le temps restant        
        if (TimeInterval_ms.count() < (tickPeriod_sec * 1000) )
        {
            struct timespec wait;
            wait.tv_sec = 0;
            wait.tv_nsec =(long)( (tickPeriod_sec * 1000000000) - (TimeInterval_ms.count() * 1000000) );
            nanosleep(&wait, NULL);
            cout <<endl<< "! (SERVEUR) Rafraichissement maintenu !" << endl;
        }
        else
        {
            cout <<endl<< "!!! (SERVEUR)ERREUR de rafraichissement(un des client a LAG) !!!" << endl;
        }
        
    }
    pthread_cancel(threadRecv);
    pthread_join(threadRecv, NULL);
    return 0;
}

//Initialisation du Serveur
int InitServer(GameServer& server)
{
    int status;
    status = server.initialize();
    switch (status)
    {
    case ALREADY_READY:
        cout << "(SERVEUR)ERREUR, Serveur deja initialisé" << endl;
        return status;
        break;

    case SOCKET_ERROR:
        cout << "(SERVEUR)ERREUR de creation de socket" << endl;
        return status;
        break;

    case BIND_ERROR:
        cout << "(SERVEUR)ERREUR de bind" << endl;
        return status;
        break;
    case LISTEN_ERROR:
        cout << "(SERVEUR)ERREUR de listen" << endl;
        return status;
        break;

    case OK:
        cout << "(SERVEUR)Serveur initialisé avec succes" << endl;
        break;

    default:
        return -1;
    }
    return status;
}


int InitGame(Text& score,string fontPath , RectangleShape (&separators)[16])
{
    int status;

    //client 1
    status = SendScoreSeparator(score, fontPath, separators, true);
    if(status != OK)
    {
        cout << "(SERVEUR)ERREUR d'initialisation de score & SEPARATOR pour client 1" << endl;
        return status;
    }
    //client 2
    status = SendScoreSeparator(score, fontPath, separators, false);
    if(status != OK)
    {
        cout << "(SERVEUR)ERREUR d'initialisation de score & SEPARATOR pour client 2" << endl;
        return status;
    }
    return OK;
}

//code repris du main de pong solo
void HandleBall(Ball &ball, Bat &batC1, Bat &batC2, int &scoreC1, int &scoreC2)
{
    // Handle ball hitting top or bottom
    if (ball.getPosition().top > windowHeight || ball.getPosition().top < 0)
    {
        // reverse the ball direction
        ball.reboundTopOrBot();
    }

    // Handle ball hitting left side
    if (ball.getPosition().left < 0)
    {
        ball.hitSide(windowWidth / 2, windowHeight / 2);
        scoreC2++;
        batC1.setYPosition(windowHeight / 2);
        batC2.setYPosition(windowHeight / 2);
    }

    // Handle ball hitting right side
    if (ball.getPosition().left > windowWidth)
    {
        ball.hitSide(windowWidth / 2, windowHeight / 2);
        scoreC1++;
        batC1.setYPosition(windowHeight / 2);
        batC2.setYPosition(windowHeight / 2);
    }

    // Has the ball hit the bat?
    if (ball.getPosition().intersects(batC1.getPosition()) || ball.getPosition().intersects(batC2.getPosition()))
    {
        ball.reboundBat();
    }

    batC1.update();
    batC2.update();
    ball.update();
}

int EventClient(Bat &batC1, Bat &batC2)
{
    string Data;
    int status = OK;

    // Client 1
    status = receiveDataClient1(Data);
    if (status == OK)
    {   //Si fermeture d'un client 
        if (Data=="STOP")
        {
            cout << "(SERVEUR)Fin de connexion demandée par client 1" << endl;
            status = client2->send((char *)"STOP");
            if (status != OK)
            {
                return status;
            }
            cout << "(SERVEUR)Fin de connexion envoyée au client 2" << endl;
            cout << "(SERVEUR)Fin de connexion" << endl;
            return 99;
        }

       // Client 1  (code repris du main de pong solo)
        //Si le client clique fleche de haut
        if (Data == "Up")
        {
            if (batC1.getPosition().top > 0)
                batC1.moveUp();

        }
        //Si le client clique fleche du bas
        else if (Data == "Down")
        {
            if (batC1.getPosition().top < windowHeight - batC1.getShape().getSize().y)
                batC1.moveDown();
            
        }
    }
    else
    {
        cout << "(SERVEUR)ERREUR de reception position bat client 1" << endl;
        return status;
    }

    // Client 2
    status = receiveDataClient2(Data);
    if (status == OK)
    {
        if (Data=="STOP")
        {
            cout << "(SERVEUR)Fin de connexion demandée par client 2" << endl;
            status = client1->send((char *)"STOP");
            if (status != OK)
            {
                return status;
            }
            cout << "(SERVEUR)Fin de connexion envoyée au client 1" << endl;
            cout << "(SERVEUR)Fin de connexion" << endl;
            return 99;
        }

        // Client 2  (code repris du main de pong solo)
        if (Data == "Up")
        {
            if (batC2.getPosition().top > 0)
                batC2.moveUp();
        }
        else if (Data == "Down")
        {
            if (batC2.getPosition().top < windowHeight - batC2.getShape().getSize().y)
                batC2.moveDown();
            
        }
    }
    else
    {
        cout << "(SERVEUR)ERREUR de reception position bat client 2" << endl;
        return status;
    }

    return OK;
}

int SendScoreSeparator(Text& score, string fontPath, RectangleShape (&separators)[16],bool isClient1)
{
    string Data;
    int status=OK;

    if(isClient1)
    {
        status=receiveDataClient1(Data);
        if(status != OK)
        {
            cout << "(SERVEUR)ERREUR de reception de la commande score & SEPARATOR du client 1" << endl;
            return status;
        }
    }
    else //si client 2
    {
        status=receiveDataClient2(Data);
        if(status != OK)
        {
            cout << "(SERVEUR)ERREUR de reception de la commande score & SEPARATOR du client 2" << endl;
            return status;
        }
    }

    ostringstream score_game;
    score_game << fontPath<<" " << score.getCharacterSize() << " " << score.getFillColor().toInteger() << " " << score.getPosition().x << " " << score.getPosition().y;
    
    ostringstream oss_separator; 

    for (int i = 0; i < 16; i++) 
    {//Ajoute les dimensions (largeur et hauteur) et les positions (x et y) de chaque separateur
        oss_separator << separators[i].getSize().x << " " << separators[i].getSize().y << " ";
        oss_separator << separators[i].getPosition().x << " " << separators[i].getPosition().y << " ";
    }

    if(isClient1)
    {    //Envoie la chaîne construite au client et stocke le résultat dans status.
        status = client1->send((char*) score_game.str().c_str());
        if(status != OK)
        {
            cout <<endl<< "(SERVEUR)ERREUR d'envoi GraphDATA score vers client 1" << endl;
            return status;
        }
        //Envoie la chaîne construite au client et stocke le résultat dans status.
        status = client1->send((char *)oss_separator.str().c_str());
        if (status != OK)
        {
            cout << endl<< "(SERVEUR)ERREUR d'envoi SEPARATOR vers client 1" << endl;
            return status;
        }
    }
    else //si client 2
    {
        status = client2->send((char*) score_game.str().c_str());
        if(status != OK)
        {
            cout <<endl<< "(SERVEUR)ERREUR d'envoi GraphDATA score vers client 2" << endl;
            return status;
        }
       
        status = client2->send((char *)oss_separator.str().c_str());
        if (status != OK)
        {
            cout << endl<< "(SERVEUR)ERREUR d'envoi SEPARATOR vers client 2" << endl;
            return status;
        }
    }
    return OK;
}

int SendDataToClient(GameClient *client, Ball &ball, Bat &batC1, Bat &batC2, int scoreC1, int scoreC2, bool isClient1)
{
    ostringstream data_game;
    if (isClient1)
    {
        data_game << ball.getPosition().left << " " << ball.getPosition().top << " ";
        data_game << scoreC1 << " " << scoreC2 << " ";
        data_game << batC1.getPosition().left << " " << batC1.getPosition().top << " ";
        data_game << batC2.getPosition().left << " " << batC2.getPosition().top;
    }
    else
    {//Les coordonnées sont ajustées pour qu'il se voit comme le joueur de gauche
        data_game << (windowWidth - ball.getPosition().left - ball.getShape().getSize().x) << " " << ball.getPosition().top << " ";
        data_game << scoreC2 << " " << scoreC1 << " ";
        data_game << (windowWidth - batC2.getPosition().left - batC2.getShape().getSize().x) << " " << batC2.getPosition().top << " ";
        data_game << (windowWidth - batC1.getPosition().left - batC1.getShape().getSize().x) << " " << batC1.getPosition().top;
    }
    data_game << endl;

    int status = client->send((char*)data_game.str().c_str());
    if (status != OK)
    {
        cout << "(SERVEUR)ERREUR d'envoi position ball et bats vers client " << (isClient1 ? "1" : "2") << endl;
        return status;

    }
    return status;
}

void *FctThreadReceive(void *setting)
{
    //Autorise le thread à être annulé en utilisant pthread_cancel.
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    while (1)
    {
        char Data[1024];
        int status;

        // Reception du client 1
        // Reçoit les données de manière non bloquante et stocke les données dans Data. Le timeout est fixé à 200 millisecondes
        status = client1->receiveNonBlocking(Data, 80);
        handleThreadReceiveStatus(status, Data, true);

        // Reception du client 2
        // Reçoit les données de manière non bloquante et stocke les données dans Data. Le timeout est fixé à 200 millisecondes
        status = client2->receiveNonBlocking(Data, 80);
        handleThreadReceiveStatus(status, Data, false);
    }
    pthread_exit(NULL);
}//FIN   FctThreadReceive


/* Gère la réception des données pour deux clients distincts dans un contexte multithread. 
/* Elle utilise des mutex et des variables de condition pour synchroniser l'accès aux files d'attente*/
void handleThreadReceiveStatus(int status, char *Data, bool isClient1)
{
    if (isClient1)
    {
        if (status == TIMEOUT)
        {
            pthread_mutex_lock(&mutexReceiveClient1);
            if (!receivedQueueClient1.empty())
            {
                receiveDataAvailableClient1 = true;
                pthread_cond_signal(&condReceiveClient1);
            }
            pthread_mutex_unlock(&mutexReceiveClient1);

        }// Erreur réception
        else if (status != OK)
        {
            pthread_mutex_lock(&mutexErreurRcv);
            erreurRcv = true;
            pthread_cond_signal(&condErreur);
            pthread_mutex_unlock(&mutexErreurRcv);
        }
        else //Réussite
        {
            pthread_mutex_lock(&mutexReceiveClient1);
            //Les données reçues sont converties en string et ajoutées à la file 
            string tmp(Data);
            receivedQueueClient1.push(tmp);

            receiveDataAvailableClient1 = true;
            pthread_cond_signal(&condReceiveClient1);
            pthread_mutex_unlock(&mutexReceiveClient1);
        }
    }
    else// Exaxtement le même déroulement mais pour CLIENT 2
    {
        if (status == TIMEOUT)
        {
            pthread_mutex_lock(&mutexReceiveClient2);
            if (!receivedQueueClient2.empty())
            {
                receiveDataAvailableClient2 = true;
                pthread_cond_signal(&condReceiveClient2);
            }
            pthread_mutex_unlock(&mutexReceiveClient2);
        }
        else if (status != OK)
        {
            cout << "(CLIENT threadReceive)ERREUR reception des données du serveur" << endl;
            pthread_mutex_lock(&mutexErreurRcv);
            erreurRcv = true;
            pthread_cond_signal(&condErreur);
            pthread_mutex_unlock(&mutexErreurRcv);
        }
        else
        {
            pthread_mutex_lock(&mutexReceiveClient2);
            string tmp(Data);
            receivedQueueClient2.push(tmp);
            receiveDataAvailableClient2 = true;
            pthread_cond_signal(&condReceiveClient2);
            pthread_mutex_unlock(&mutexReceiveClient2);
        }
    }
}//FIN handleThreadReceiveStatus

int receiveDataClient1(string& data)
{
    pthread_mutex_lock(&mutexReceiveClient1);

    //// On attend que des données soient disponibles ou qu'une erreur se produise
    while (!receiveDataAvailableClient1 && !erreurRcv) 
    {
        pthread_cond_wait(&condReceiveClient1, &mutexReceiveClient1);
    }

    if (erreurRcv)
    {
        pthread_mutex_unlock(&mutexReceiveClient1);
        cout<<endl<<"(CLIENT1) fct receiveData()) Erreur de Receive"<<endl;
        return ERROR;
    }

    if (!receivedQueueClient1.empty())
    {
        data =String( receivedQueueClient1.front() ); //récupère la donnée en tête de la file
        receivedQueueClient1.pop(); // Retire la donnée de la file d'attente.
    }
   
    receiveDataAvailableClient1 = false;
    pthread_mutex_unlock(&mutexReceiveClient1);
    return OK;
}

// idem mais Client 2
int receiveDataClient2(string& data)
{
    pthread_mutex_lock(&mutexReceiveClient2);
    while (!receiveDataAvailableClient2 && !erreurRcv) 
    {
        pthread_cond_wait(&condReceiveClient2, &mutexReceiveClient2);
    }

    if (erreurRcv)
    {
        pthread_mutex_unlock(&mutexReceiveClient2);
        cout<<endl<<"(CLIENT2) fct receiveData()) Erreur de Receive"<<endl;
        return ERROR;
    }

    if (!receivedQueueClient2.empty())
    {
        data =String( receivedQueueClient2.front() );
        receivedQueueClient2.pop();
    }
  
    receiveDataAvailableClient2 = false;
    pthread_mutex_unlock(&mutexReceiveClient2);
    return OK;
}