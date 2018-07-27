#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>


// this struct is set to store the available client information
struct Client_Info {
    char clientname[16];
    int fd;
    int clientCount;
};

// attribute for the massage
struct Attribute {
    int type;
    int length;
    char payload[512];
};

// header struct for the protocol
struct Header {
    unsigned int vrsn : 9;
    unsigned int type : 7;
    int length;
};

// SimpleBroadCastProtocol message
struct Message {
    struct Header header;
    struct Attribute attribute[2];
};


// get sockaddr
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


// receive messages from server (FWD, ACK, NAK, ONLINE, OFFLINE)
int getServerMessage(int sockfd) {
    struct Message server_message;
    int nbytes=0;
    int status = 0;
    nbytes=read(sockfd,(struct Message *) &server_message,sizeof(server_message));
    char* msg0 = server_message.attribute[0].payload;
    char* msg1 = server_message.attribute[1].payload;
    int type0 = server_message.attribute[0].type;
    int type1 = server_message.attribute[1].type;

    if (server_message.header.type==3) {
        if((msg0!=NULL ||
            msg0!='\0') && (msg1!=NULL ||
                            msg1!='\0') && type0==4 && type1==2) {
            printf("Forwarded Message from %s is %s", msg1, msg0);
        }
        status=0;
    }
    if (server_message.header.type==5) {
        if ((msg0!=NULL ||
             msg0!='\0') && type0==1) {
            printf("NAK Message from Server is %s", msg0);
        }
        status=1;
    }
    if (server_message.header.type==6) {
        if ((msg0!=NULL ||
             msg0!='\0') && type0==2) {
            printf("OFFLINE Message %s is now OFFLINE", msg0);
        }
        status=0;
    }
    if (server_message.header.type==7) {
        if ((msg0!=NULL ||
             msg0!='\0') && type0==4) {
            printf("ACK Message from Server is %s", msg0);
        }
        status=0;
    }
    if (server_message.header.type==8) {
        if ((msg0!=NULL ||
             msg0!='\0') && type0==2) {
            printf("ONLINE Message %s is now ONLINE", msg0);
        }
        status=0;
    }

    return status;
}


// read client input and send messages to the server
void chat(int connectionDesc) {
    struct Attribute attr;
    struct Message mess;

    struct timeval tv;
    fd_set readfds;

    int nread = 0;
    char temp[512];

    tv.tv_sec = 2;
    tv.tv_usec = 0;

    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    select(STDIN_FILENO+1, &readfds, NULL, NULL, &tv);

    if (FD_ISSET(STDIN_FILENO, &readfds)) {
		nread = read(STDIN_FILENO, temp, sizeof(temp));
        if (nread > 0) {
	        temp[nread] = '\0';
        }
    
    	attr.type = 4;
    	strcpy(attr.payload,temp);
    	mess.attribute[0] = attr;
    	write(connectionDesc,(void *) &mess,sizeof(mess));
    } else {
        printf("Timed out.\n");
    }
}


// ./Client <username> <Server-IP-address> <port no.>
// clinet sends a join message to server
void join(int sockfd,const char *arg[]) {
    struct Header header;
    struct Attribute attr;
    struct Message mess;

    int status = 0;

    header.vrsn = '3';
    header.type = '2';
    attr.type = 2;
    attr.length = strlen(arg[1]) + 1;

    strcpy(attr.payload,arg[1]);
    mess.header = header;
    mess.attribute[0] = attr;

    write(sockfd,(void *) &mess,sizeof(mess));

    sleep(1);
    status = getServerMessage(sockfd);

    if (status == 1) {
        close(sockfd);
    }
}


// ./Client <username> <Server-IP-address> <port no.>
int main(int argc, char const *argv[]) {
    if(argc == 4) {
	    int sockfd;
        struct hostent* hret;
        struct sockaddr_in servaddr;
	    fd_set master;
	    fd_set read_fds;
	    FD_ZERO(&read_fds);
	    FD_ZERO(&master);

        struct addrinfo hints, *servinfo, *p;

		int yes = 0;
		int rv;
		char s[INET6_ADDRSTRLEN];
		
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        if ((rv = getaddrinfo(argv[2], argv[3], &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return 1;
        }

        // loop over all the results trying to connect
        for (p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                perror("client: socket");
                continue;
            }

            printf("socket file descriptor is %d\n", sockfd);

            if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                close(sockfd);
                perror("client: connect");
                continue;
            }

            break;
        }

        if (p == NULL) {
            fprintf(stderr, "client: failed to connect\n");
            return 2;
        }

        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);

        printf("client: connecting to %s\n", s);
        freeaddrinfo(servinfo);
        printf("connected to the server..\n");
        join(sockfd, argv);

        FD_SET(sockfd, &master);
        FD_SET(STDIN_FILENO, &master);
        for(;;) {
            read_fds = master;
            printf("\n");

            if (select(sockfd+1, &read_fds, NULL, NULL, NULL) == -1) {
                perror("select");
                exit(4);
            }

            if (FD_ISSET(sockfd, &read_fds)) {
                getServerMessage(sockfd);
            }

            if (FD_ISSET(STDIN_FILENO, &read_fds)) {
                chat(sockfd);
            }
        }
    }

    printf("\n Client close \n");
    return 0;
}
