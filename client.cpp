#include <iostream>
#include <SFML/Graphics.hpp>
#include <string>
#include <pthread.h>
#include <queue>
#include "GameClient.h"
#include "bat.h"
#include "ball.h"

using namespace std;
using namespace sf;

int scoreC1 = 0;
int scoreC2 = 0;
int windowWidth = 1024;
int windowHeight = 768;

// initialisation du terrain 
int InitScore(Text& score, Font& font);
int InitSeparator(RectangleShape (&separators)[16]);
int InitPlayground(Text& score, Font& font, RectangleShape (&separators)[16]);

int SendEvent(bool focus);

void ExtractData(string AllData, Ball& ball, Bat& batC1, Bat& batC2, stringstream& score_game);

void DrawPlayground(Text &score, RectangleShape (&separators)[16], RenderWindow &window,stringstream& score_game, Ball& ball, Bat& batC1, Bat& batC2);

int StopConnection();

void *FctThreadReceive(void *setting);
int receiveData(string& data);

void *FctThreadSend(void *setting);
void FctSendData(const string& data);


pthread_t threadSend;
pthread_t threadRecv;

pthread_mutex_t mutexReceive = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condReceive = PTHREAD_COND_INITIALIZER;
queue<string> receivedQueue;
bool receiveDataAvailable=false;

pthread_mutex_t mutexSend = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condSend = PTHREAD_COND_INITIALIZER;
bool sendDataAvailable=false;
string sendData;
int threadStatus=OK;

bool erreurRcv = false;
pthread_mutex_t mutexErreurRcv = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condErreur = PTHREAD_COND_INITIALIZER;

GameClient* client=new GameClient();
int forLag = 0;

int main(int argc, char *argv[])
{
    if (argc == 2)
        forLag = atoi(argv[1]);

    
    char* ipAdresse = const_cast<char*>("127.0.0.1");
    int port = 8080;


    //client va se connecter au serveur
    int status = client->join(ipAdresse, port);
    if (status != OK)
    {
        cout << "(CLIENT)ERREUR de connexion au serveur" << endl;
        return status;
    }


    cout << "(CLIENT)Connecxion au serveur REUSSI" << endl;
    string TitreFenetre="Pong Client "+to_string(forLag);
    
    RenderWindow window(VideoMode(windowWidth, windowHeight), TitreFenetre.c_str());

    // create a ball
    Ball ball(windowWidth / 2, windowHeight / 2);
    // create  bats
    Bat batC1(10, windowHeight / 2);
    Bat batC2(windowWidth - 10, windowHeight / 2);
    
    stringstream score_game;
    Text score;
    Font font;
    RectangleShape separators[16];

// Création des threads
    int res=pthread_create(&threadRecv, NULL, FctThreadReceive,NULL);
    res=pthread_create(&threadSend, NULL, FctThreadSend,NULL);

   //initialisation du terrain 
    status= InitPlayground(score, font, separators);
    if(status != OK)
    {
        cout<<"(CLIENT)ERREUR initialisation du terrain"<<endl;
        pthread_cancel(threadRecv); pthread_join(threadRecv, NULL);
        pthread_cancel(threadSend); pthread_join(threadSend, NULL);
        return status;
    }

    bool focus;
    while (window.isOpen())
    {
        Event event;

        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
            {
                status=StopConnection();
                if(status != OK)
                {
                    pthread_cancel(threadRecv); pthread_join(threadRecv, NULL);
                    pthread_cancel(threadSend); pthread_join(threadSend, NULL);    
                    return status; 
                }
                else
                {
                    window.close(); 
                    cout<<endl<<"(CLIENT)Fenetre fermée" << endl; 
                    pthread_cancel(threadRecv); pthread_join(threadRecv, NULL); 
                    pthread_cancel(threadSend); pthread_join(threadSend, NULL);
                    return 0; 
                }
            }
            //Si fenetre active
            else if(event.type == sf::Event::GainedFocus) 
                focus=true;
            //si fenetre inactive
            else if(event.type == sf::Event::LostFocus) 
                focus=false;
        }

        status= SendEvent(focus);
        if(status != OK)
        {
            if(status == 99)
            {
                window.close();
                break;
            }
            cout<<"(CLIENT)ERREUR envoi des evenements au serveur"<<endl;
            pthread_cancel(threadRecv); pthread_join(threadRecv, NULL);
            pthread_cancel(threadSend); pthread_join(threadSend, NULL);
            return status;
        }

        //reception DATA du serveur
        string AllData;
        status= receiveData(AllData);
        if(status != OK)
        {
            cout<<"(CLIENT)ERREUR reception des données du serveur"<<endl;
            cout<<endl<< AllData << endl;
            pthread_cancel(threadRecv); pthread_join(threadRecv, NULL);
            pthread_cancel(threadSend); pthread_join(threadSend, NULL);
            return status;
        }

        if(AllData=="STOP")
        {
            cout<<"(CLIENT)Fin de connexion Recu(STOP)" << endl;
            cout << "(CLIENT)Fin de connexion confirmée" << endl;
            break;
        }
        
        ExtractData(AllData, ball, batC1, batC2, score_game);
        DrawPlayground(score, separators, window, score_game, ball, batC1, batC2);
    }

    pthread_cancel(threadRecv);pthread_join(threadRecv, NULL);
    pthread_cancel(threadSend); pthread_join(threadSend, NULL);

    return 0;
}

int InitScore(Text& score, Font& font)
{
    int status;
    FctSendData("score");
    status= threadStatus;
    if(status!= OK)
    {
        cout<<"(CLIENT)ERREUR envoi score  au serveur"<<endl;
        return status;
    }

    string score_game;
    status=receiveData(score_game);
    if(status != OK)
    {
        cout<<"(CLIENT)ERREUR reception des données score du serveur"<<endl;
        return status;
    }

    istringstream data(score_game);
    string fontFilename;
    int characterSize;  Uint32 fillColor;
    float positionX, positionY;

    data >> fontFilename >> characterSize >> fillColor >> positionX >> positionY;
    font.loadFromFile(fontFilename);
    score.setFont(font);
    score.setCharacterSize(characterSize);
    score.setFillColor(sf::Color(fillColor));
    score.setPosition(Vector2f(positionX, positionY));
    return OK;
}

int InitSeparator(RectangleShape (&separators)[16])
{
    string separator;
    int status=receiveData(separator);
    if(status != OK)
    {
        cout<<"(CLIENT)ERREUR reception des données SEPARATOR du serveur"<<endl;
        return status;
    }

    istringstream data(separator);

    for (int i = 0; i<16;i++)
    {
        float sizeX, sizeY, positionX, positionY;
        data >> sizeX >> sizeY >> positionX >> positionY;
        separators[i].setSize(Vector2f(sizeX, sizeY));
        separators[i].setPosition(Vector2f(positionX, positionY));
    }
    return OK;

}

int InitPlayground(Text& score, Font& font, RectangleShape (&separators)[16])
{
    int status= InitScore(score, font);
    if(status != OK)
    {
        cout<<"(CLIENT)ERREUR initialisation du score"<<endl;
        return status;
    }
    status= InitSeparator(separators);
    if(status != OK)
    {
        cout<<"(CLIENT)ERREUR initialisation du SEPARATOR"<<endl;
        return status;
    }
    return OK;
}

//Code repris et modifié du main.cpp  pong solo
int SendEvent(bool focus)
{
    int status;
    if (focus)//si la fenetre est en focus
    {
        if (Keyboard::isKeyPressed(sf::Keyboard::Escape))
        {
            status=StopConnection();
            if(status != OK){return status;}
            else{return 99;}
        }
        if (Keyboard::isKeyPressed(Keyboard::Up))
        {
            FctSendData("Up");
            status= threadStatus;
            if (status != OK)
            {
                cout << "(CLIENT)ERREUR envoi de la position du bat au serveur (Up)" << endl;
                return status;
            }
        }
        else if (Keyboard::isKeyPressed(Keyboard::Down))
        {
            FctSendData("Down");
            status= threadStatus;
            if (status != OK)
            {
                cout << "(CLIENT)ERREUR envoi de la position du bat au serveur (Down)" << endl;
                return status;
            }
        }
        else //si pas de mouvement
        {
            FctSendData("NOT");
            status= threadStatus;
            if (status != OK)
            {
                cout << "(CLIENT)ERREUR envoi de la position du enemyBat au serveur (NOT)" << endl;
                return status;
            }
        }
    }
    else //si la fenetre n'est pas celle principale, n'est pas donc celle "focus"
    {
        FctSendData("NOT");
        status= threadStatus;
        if (status != OK)
        {
            cout << "(CLIENT)ERREUR envoi de la position du enemyBat au serveur (NOT)" << endl;
            return status;
        }
    }
    return OK;
}

void ExtractData(string AllData, Ball& ball, Bat& batC1, Bat& batC2, stringstream& score_game)
{

    istringstream data(AllData); // permet d'extraire les variable de la chaine de caractère
    float ballLeft, ballTop,
        batC1Left, batC1Top,
        batC2Left, batC2Top;

//>> Utilisé pour extraire les valeurs de data et les stocker dans les variables correspondantes.
    data >> ballLeft >> ballTop >> scoreC1 >> scoreC2 >> batC1Left >> batC1Top >> batC2Left >> batC2Top;

    ball.setPosition(ballLeft, ballTop);
    batC1.setPosition(batC1Left, batC1Top);
    batC2.setPosition(batC2Left, batC2Top);
    score_game.str("");

    score_game << scoreC1 << "\t" << scoreC2;
}

// code repris du main.cpp de pong solo
void DrawPlayground(Text &score, RectangleShape (&separators)[16], RenderWindow &window,stringstream& score_game, Ball& ball, Bat& batC1, Bat& batC2)
{
    score.setString(score_game.str());

    // Clear everything from the last frame
    window.clear(Color(0, 0, 0, 255));
    // draw everything
    window.draw(batC1.getShape());
    window.draw(ball.getShape());
    window.draw(batC2.getShape());

    // Draw our score
    window.draw(score);

    // draw separator
    for (int i = 0; i < 16; i++)
    {
        window.draw(separators[i]);
    }
    // Show everything we just drew
    window.display();
}

int StopConnection()
{
    FctSendData("STOP");
    int status= threadStatus;
    if(status != OK)
    {
        cout<<"(CLIENT)ERREUR envoi AU REVOIR au serveur (STOP)"<<endl;
        return status;
    }
    else
    {
        cout<<"(CLIENT)Message envoyé au serveur (STOP)"<<endl;
        cout << "(CLIENT)Fin de connexion " << endl;
        return OK;
    }
}

////Cette fonction FctThreadReceive est conçue pour recevoir des données via un thread dédié
void *FctThreadReceive(void *setting)
{   
    //Autorise le thread à être annulé en utilisant pthread_cancel.
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    while (1)
    {
        char Data[1024];
        // Reçoit les données de manière non bloquante et stocke les données dans Data. Le timeout est fixé à 120 millisecondes
        int status = client->receiveNonBlocking(Data, 120);

        struct timespec wait;
        // Conversion des millisecondes en nanosecondes
        wait.tv_sec = 0;
        wait.tv_nsec = (long)(forLag *1000000);
        nanosleep(&wait, NULL);// Utilisation de nanosleep pour attendre cette durée.

        if (status == TIMEOUT)
        {   //verrouille le mutex mutexReceive
            pthread_mutex_lock(&mutexReceive);
            if (!receivedQueue.empty()) // si la queue receivedQueue n'est pas vide
            {   //elle signale condReceive que des données sont disponibles
                receiveDataAvailable = true;
                pthread_cond_signal(&condReceive);
            }
            pthread_mutex_unlock(&mutexReceive); //Déverrouille ensuite le mutex.
        }
        else if (status != OK)
        {
            cout << "(CLIENT threadReceive)ERREUR reception des données du serveur" << endl;
            pthread_mutex_lock(&mutexErreurRcv); //verrouille mutexErreurRcv
            erreurRcv = true;
            //signale condErreur et déverrouille le mutex.
            pthread_cond_signal(&condErreur);
            pthread_mutex_unlock(&mutexErreurRcv);
            break;
        }
        else //Si les données sont reçues correctement (
        {
            pthread_mutex_lock(&mutexReceive);
            string tmp(Data); //place les données reçues dans tmp
            receivedQueue.push(tmp);  //place les données reçues dans une chaîne de caractères tmp
            receiveDataAvailable = true;
            pthread_cond_signal(&condReceive);
            pthread_mutex_unlock(&mutexReceive);
        }
    }
    pthread_exit(NULL);
}//FIN FctThreadReceive

//récupère les données reçues par un client à partir d'une queue
int receiveData(string& data)
{
    pthread_mutex_lock(&mutexReceive);
    //Utilisation de la condition pthread_cond_wait pour attendre que receiveDataAvailable devienne vrai ou que erreurRcv soit défini
    while (!receiveDataAvailable && !erreurRcv) 
    {
        pthread_cond_wait(&condReceive, &mutexReceive);
    }

    if (erreurRcv) //il y a une erreur on le signale
    {
        pthread_mutex_unlock(&mutexReceive);
        cout<<endl<<"(CLIENT fct receiveData()) Erreur de Receive"<<endl;
        return ERROR;
    }
    //Si la queue  contient des données
    if (!receivedQueue.empty())
    {
        data = receivedQueue.front();//copie la première entrée (front()) dans data
        receivedQueue.pop(); //puis retire cette entrée de la queue
    }
    receiveDataAvailable = false;
    pthread_mutex_unlock(&mutexReceive);
    return OK;
}//FIN receiveData

//Cette fonction FctThreadSend est conçue pour envoyer des données via un thread dédié
void *FctThreadSend(void *setting)
{   //Autorise le thread à être annulé en utilisant pthread_cancel.
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    while (1)
    {
        string data;
        pthread_mutex_lock(&mutexSend);
        
        //Utilisation de la condition pthread_cond_wait pour attendre que sendDataAvailable devienne vrai
        while (!sendDataAvailable)
        {
            pthread_cond_wait(&condSend, &mutexSend);
        }
        //Une fois que les données sont disponibles, elles sont copiées depuis sendData dans data.
        data = String(sendData);

        //Envoie les données via le client
        int status = client->send((char *)data.c_str());

        if (status != OK)
        {
            // Gestion erreur send
            cout << "(CLIENT threadSend)ERREUR sending data: " << status << endl;
            pthread_mutex_unlock(&mutexSend);
            continue; //continue la boucle pour essayer à nouveau
        }
        else
        {
            // Réussite send
            threadStatus=OK;
        }
//On met à faux pour indiquer que les données ont été envoyées avec succès et que le thread est prêt à recevoir de nouvelles données à envoyer.
        sendDataAvailable = false;

        pthread_mutex_unlock(&mutexSend);

        // Verrouille à nouveau le mutex pour mettre à jour threadStatus
        pthread_mutex_lock(&mutexSend);
        threadStatus = status;

        pthread_mutex_unlock(&mutexSend);
    }
}//FIN  FctThreadSend

//Cette fonction permet donc de placer des données dans sendData
//puis de signaler à un autre thread (qui attend sur condSend) qu'il peut commencer à traiter ces données.
//Cela garantit que l'envoi des données est synchronisé et sûr, même dans un environnement multi-thread.
void FctSendData(const string& data)
{
    pthread_mutex_lock(&mutexSend);
    sendData = String(data);
    sendDataAvailable = true;
    pthread_cond_signal(&condSend);
    pthread_mutex_unlock(&mutexSend);
}