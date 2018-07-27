#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//this struct is set to store the available client information
struct Client_Info
{
    char clientname[16];
    int fd;
	
    int clientCount;
	
};

//attribute for the massage
struct Attribute
{
    int type;
    int length;
    char payload[512];
};

//header struct for the protocol
struct Header{
    unsigned int vrsn : 9;
    unsigned int type : 7;
    int length;
};

//SimpleBroadCastProtocol message
struct Message
{
    struct Header header;
    struct Attribute attribute[2];
};


//here we set 3 global variables
struct Client_Info *clients;			
int currentClientCount = 0;				//current client number
int maxClients = 0;						//maximum clients allowed


//NAK is sent bacause a new connection
void sendACK(int sk){

    struct Message ACK;
    struct Header ACK_Header;
    struct Attribute ACK_Attribute;
    int i = 0;
    char temp[180];					//temp is used to preserve list of clients

    ACK_Header.vrsn=3;
    ACK_Header.type=7;
    

    ACK_Attribute.type = 4;

	//first store the client number
    temp[0] = (char)(((int)'0')+ currentClientCount);
    temp[1] = ' ';
    temp[2] = '\0';
	
	//we store the list of client names
    for(i =0; i<currentClientCount-1; i++)
    {
        strcat(temp,clients[i].clientname);
        if(i != currentClientCount-2)
        strcat(temp, ",");
    }
	
	//we are assembling an ACK message
    ACK_Attribute.length = strlen(temp)+1;
    strcpy(ACK_Attribute.payload, temp);
    ACK.header = ACK_Header;
    ACK.attribute[0] = ACK_Attribute;

    write(sk,(void *) &ACK,sizeof(ACK));
}

//NAK is sent due to maximum users (code -- 2) or existing username (code -- 1)
void sendNAK(int sk,int code){

    struct Message NAK;
    struct Header NAK_Header;
    struct Attribute NAK_Attribute;
    char temp[32];

    NAK_Header.vrsn =3;
    NAK_Header.type =5;

    NAK_Attribute.type = 1;

    //1 - Inv username, 2--maximum users
	if(code == 2){
		
		strcpy(temp,"maximum clients\n");
		
		NAK_Attribute.length = strlen(temp);
		strcpy(NAK_Attribute.payload, temp);

		NAK.header = NAK_Header;
		NAK.attribute[0] = NAK_Attribute;
		
		write(sk,(void *) &NAK,sizeof(NAK));
		
	}else if(code == 1) {
		
        strcpy(temp,"Invalid username");
		NAK_Attribute.length = strlen(temp);
		
		strcpy(NAK_Attribute.payload, temp);

		NAK.header = NAK_Header;
		NAK.attribute[0] = NAK_Attribute;

		write(sk,(void *) &NAK,sizeof(NAK));

		close(sk);
    }

}


//checks if username does not exist already
int username_check(char a[]){
    int i = 0;
    int ret = 0;
    for(i = 0; i < currentClientCount ; i++){
        if(!strcmp(a,clients[i].clientname)){		//if the two names are the same
            ret = 1;
            break;
        }
    }
    return ret;
}

//The connection check function is used to check if the connection
//should be available
int connection_check(int fd){

	//we check if the clients are maximum right here
	if(currentClientCount == maxClients){
		sendNAK(fd, 2);
		return 2;					// 2 for maximum clients #
	}

    struct Message join;
    struct Attribute join_attribute;
    char temp[16];

    read(fd,(struct Message *) &join,sizeof(join));
    
    join_attribute = join.attribute[0];
    strcpy(temp, join_attribute.payload);
	
	//here we check it the user name is an existing username	
    int rv = username_check(temp);
	
    if(rv == 1)							//return value is 1 because the two names are the same
    {
		
        printf("\nClient already exists.");
        sendNAK(fd, 1); // 1 for client already exists 
		
    } else	{
		
		//return rv is 0 because there are no name conflicts
        strcpy(clients[currentClientCount].clientname, temp);
        clients[currentClientCount].fd = fd;
        clients[currentClientCount].clientCount = currentClientCount;
        currentClientCount = currentClientCount + 1;
        sendACK(fd);
    }
    return rv;							//0 -- no name conflicts and no maximum clients
}											//1 -- name conflicts


int main(int argc, char const *argv[])
{
    /* code */
    struct Message clientMessage, forward_msg, online_msg, offline_msg;
    struct Attribute clientAttribute;

	int connect_socket;
    unsigned int len;
    int ret;

	struct sockaddr_in *cli;

    
    fd_set master;
    fd_set read_fds;
    int fdmax;
	int listener;     // listening socket descriptor
	
    int temp;
    int i=0,y;
    int j=0;
    int x=0;
	
    int nbytes;
    int yes=1;        // for setsockopt() SO_REUSEADDR, below
	int rv;
	struct addrinfo hints, *ai, *p;
    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);
	
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, argv[2], &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) { 
            continue;
        }
        
		
        // lose the pesky "address already in use" error message
        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }
	

    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    freeaddrinfo(ai); 
	
	maxClients=atoi(argv[3]);

    clients= (struct ClientInformation *)malloc(maxClients*sizeof(struct Client_Info));
    cli=(struct sockaddr_in *)malloc(maxClients*sizeof(struct sockaddr_in));
			
    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }

    // add the listener to the master set
    FD_SET(listener, &master);


    fdmax = listener; 
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 50000;

    for(;;)
    {
	
        read_fds = master;
        if(select(fdmax+1, &read_fds, NULL, NULL, &tv) == -1){
            perror("select");
            exit(4);
        }

        for(i=0 ; i<=fdmax ; i++){
            if(FD_ISSET(i, &read_fds)){
                if(i == listener){
                    //New Connections

						//Add them to the client address array cli
						len = sizeof(cli[currentClientCount]);
						
						connect_socket = accept(listener,(struct sockaddr *)&cli[currentClientCount],&len);
						
						if(connect_socket < 0){
							printf("\nServer accept failed.");
							ret = -1;
						}
						else {
							temp = fdmax;
							FD_SET(connect_socket, &master);
							if(connect_socket > fdmax){
								fdmax = connect_socket;
							}   
							int connection_check_value = connection_check(connect_socket);		//connection_check_value = 0 if join successful
							
							//for each connection, we check it first
							if(connection_check_value == 0){					//if join successful   
								//Send an ONLINE Message to all the clients except this
								printf("\nServer accepted the client : %s",clients[currentClientCount-1].clientname);
								online_msg.header.vrsn=3;
								online_msg.header.type=8;
								online_msg.attribute[0].type=2;
								strcpy(online_msg.attribute[0].payload,clients[currentClientCount-1].clientname);	
					
								//we notifies clients about the new clients online
								for(j = 0; j <= fdmax; j++) {
									// send to everyone!
									if (FD_ISSET(j, &master)) {
										// except the listener and ourselves
										if (j != listener && j != connect_socket) {
											if ((write(j,(void *) &online_msg,sizeof(online_msg))) == -1)  {
												printf("error when server send online msgs\n");
											}
										}
									}       
								}
								ret = connect_socket;
								//after connection_check value == 1 bacause we have username conflicts
							} else if(connection_check_value == 1)	{
								
								printf("Client username already exists\n");
								fdmax = temp;
								FD_CLR(connect_socket, &master);
								
								//after connection_check value == 2 bacause we already have the maximum client #
							} else if(connection_check_value == 2) {
								
								printf("maximum clients number\n");
								fdmax = temp;
								FD_CLR(connect_socket, &master);	
							}  
						}
//					}
                }else{
					
					//here we check if we can read some bytes, if the number of bytes read <= 0
					if ((nbytes=read(i,(struct MessageSBCP *) &clientMessage,sizeof(clientMessage))) <= 0){
						
						//if the length of bytes == 0, the socket is closed
						if (nbytes == 0){
							
							printf("\nSocket %d hung up\n", i);
                            
							//we extract the username corresponding to the closed socket fd
							for(y=0;y<currentClientCount;y++){
								if(clients[y].fd==i){
									offline_msg.attribute[0].type=2;
									strcpy(offline_msg.attribute[0].payload,clients[y].clientname);	
								}
							}
                                
                            offline_msg.header.vrsn=3;
                            offline_msg.header.type=6;
                                
							//we send the offline msgs to other users who are not offline	
                            for(j = 0; j <= fdmax; j++) {
	                    	    
        	            	    if (FD_ISSET(j, &master)) {
        	            	        
        	            	        if (j != listener && j != i){
        	                    	    if ((write(j,(void *) &offline_msg,sizeof(offline_msg))) == -1){
        	                            	perror("send");
        	                            }
        	                        }
        	                    }       
        	                }
						}else{
							perror("recv");
						}
						
						//either the length of bytes <0 or == 0, we close the socket
						close(i); 
						FD_CLR(i, &master); 
			
						//the client left, so we update our database
						for(x=i;x<currentClientCount;x++){
							clients[x]=clients[x+1];
						}
						
                        currentClientCount--;
						
					} else {
                        
						//we assembly the msg for forwarding
                    	clientAttribute = clientMessage.attribute[0];
						forward_msg=clientMessage;
						forward_msg.header.type=3;
						forward_msg.attribute[1].type=2;

						int payloadLength=0;
                        char temp[16];
                        payloadLength=strlen(clientAttribute.payload);
                        strcpy(temp,clientAttribute.payload);
                        temp[payloadLength]='\0';

                    	printf("Payload is %s",temp);

						//for each msg needs forwarding, we extract its username
						for(y=0;y<currentClientCount;y++){
							
							if(clients[y].fd==i){
								strcpy(forward_msg.attribute[1].payload,clients[y].clientname);	
							}
						}
						
						//the msgs are forwarded to clients except 
						//the client who send the msg to the server
                    	for(j = 0; j <= fdmax; j++){
                    	    
                    	    if (FD_ISSET(j, &master)){
								
                    	        
                    	        if (j != listener && j != i){
									
                            	    if ((write(j,(void *) &forward_msg,nbytes)) == -1){
                                    	perror("send");
                                    }
                                }
                            }       
						}
                    }
                }
            }
        }
    }
    
    close(listener);

    return 0;
}
