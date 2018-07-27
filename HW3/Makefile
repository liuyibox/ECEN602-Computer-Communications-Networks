# Enable gcc compiler 
CC = gcc
# Set -Wall as per requirement and -g for debug.
CFLAGS=-Wall -g
all: tftpserver
 tftpserver: TFTP_Server.c
		$(CC) $(CFLAGS) TFTP_Server.c -o Server

 clean:
	rm -rf *o Server
