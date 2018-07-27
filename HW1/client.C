/*
client.C --a simple echo server
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <netdb.h>
#include<fstream>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
using namespace std;

#define PORT "6001"
#define MAXDATASIZE 25 

int main(int argc, char* argv[]){


	//value initialization
	char* test_echo = (char*) "echo";
	char* exit = (char*) "exit";
	char input_begin[MAXDATASIZE];
	char* echo;
	char* check_exit;
	fgets(input_begin, MAXDATASIZE,stdin);
	echo = strtok(input_begin, " ");

	//if it receives echo command
	if(strcmp(echo, test_echo) == 0 ) {

		//we allocate values for socket programming	
		int sockfd, numbytes;  
		char send_buf[MAXDATASIZE], recv_buf[MAXDATASIZE];
		struct addrinfo hints, *servinfo, *p;
		int rv;
		char s[INET6_ADDRSTRLEN];

		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;

		//vanilla process of connecting to server
		if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
			return 1;
		}

		for(p = servinfo; p != NULL; p = p->ai_next) {
			if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
				continue;
			}
			if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
				close(sockfd);
				continue;
			}
			break;
		}

		if (p == NULL) {
			fprintf(stderr, "client: failed to connect\n");
			return 2;
		}

		//setting up the socket address
		struct sockaddr* p_sa = (struct sockaddr*)(p->ai_addr);
		if(p_sa->sa_family == AF_INET){
			struct in_addr *sin_addr = &(((struct sockaddr_in*)p_sa)->sin_addr);
			inet_ntop(p->ai_family, sin_addr,s, sizeof s);
		}else{
			struct in6_addr *sin6_addr = &(((struct sockaddr_in6*)p_sa)->sin6_addr);
			inet_ntop(p->ai_family, sin6_addr,s, sizeof s);
		}

		printf("client: connecting to %s\n", s);
		freeaddrinfo(servinfo);
		printf("Welcome to our echo client.\n");

		//looping to receive msgs from command line
		for(;;){

			//receive msgs from command line
			printf("\nclient ");
			if(fgets(send_buf, MAXDATASIZE, stdin) == NULL){
				printf("you ended echo\n");
				break;
			}

			//if the command is exiting the execution
			check_exit = strtok(send_buf, "\n");			
			if(strcmp(send_buf,exit) == 0){
				printf("exiting...\n");
				break;
			}
			
			//send the msgs to the server
			if((numbytes = send(sockfd, send_buf, MAXDATASIZE-1, 0)) == -1){
				printf("error when sending...\n ");
				abort();
			}

			//receive the msgs from the server
			if ((numbytes = recv(sockfd,recv_buf, MAXDATASIZE-1, 0)) == -1) {
			 	printf("error when receiving...");
				abort();
			}
		
			//specify the end of the buffer
			recv_buf[numbytes] = '\0';

			//print out the echoed msg from server
			printf("received: ");
			for(int j = 0; j < strlen(recv_buf); j++){
				if(recv_buf[j] == '\n')
					break;
				printf("%c",recv_buf[j]);
			}

			bzero(send_buf, sizeof send_buf ); bzero(recv_buf, sizeof recv_buf);
		}

		close(sockfd);
	}
	return 0;
}
