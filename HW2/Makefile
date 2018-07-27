# Enable gcc compiler 
CC = gcc
# Set -Wall as per requirement and -g for debug.
CFLAGS=-Wall -g
all: sbcpserver sbcpclient
 sbcpserver: Server.c
		$(CC) $(CFLAGS) Server.c -o Server

 sbcpclient: Client.c
		$(CC) $(CFLAGS) Client.c -o Client

 clean:
	rm -rf *o Server Client
