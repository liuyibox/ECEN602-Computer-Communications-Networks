all: client server

client: client.C
	g++ -o client client.C -pthread
server: server.c
	g++ -o server server.c

clean:
	rm server client
