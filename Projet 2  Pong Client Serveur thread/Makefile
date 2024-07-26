all: serveur client

serveur: serveur.o GameServer.o GameClient.o bat.o ball.o
	g++ -o serveur serveur.o GameServer.o GameClient.o bat.o ball.o -lsfml-graphics -lsfml-window -lsfml-system -pthread

client: client.o GameClient.o bat.o ball.o
	g++ -o client client.o GameClient.o bat.o ball.o -lsfml-graphics -lsfml-window -lsfml-system -pthread
	
serveur.o: serveur.cpp
	g++ -c serveur.cpp -pthread

GameServer.o: GameServer.cpp GameServer.h
	g++ -c GameServer.cpp -pthread

GameClient.o: GameClient.cpp GameClient.h
	g++ -c GameClient.cpp -pthread

bat.o: bat.cpp bat.h
	g++ -c bat.cpp -pthread

ball.o: ball.cpp ball.h
	g++ -c ball.cpp -pthread

clean:
	rm *.o serveur client
