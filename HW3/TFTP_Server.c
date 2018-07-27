#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>



#define CLIENT_MAX 4
#define DATA_SIZE_MAX 512



struct RRQPacket {

    uint16_t opcode;
    char *file_name;
    uint8_t end_of_file;
    char mode[512];
    uint8_t end_of_file1;

};



struct client {

    int client_fd;
    int block_no;
    FILE* fp;

};



struct client client_list[CLIENT_MAX];
int client_counter = 0;
FILE *fp = NULL;



int createSocket() {

    fflush(stdout);

    int client_fd;

    int port_num = 0;

    struct sockaddr_in client_addr;

    client_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (client_fd < 0) {

        printf("Error opening socket.\n");

    }

    int opt_val = 1;

    setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, (const void *) &opt_val ,sizeof(int));

    bzero((char *) &client_addr, sizeof(client_addr));

    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    client_addr.sin_port = htons((unsigned short)port_num);

    if (bind(client_fd, (struct sockaddr *) &client_addr, sizeof(client_addr)) < 0) {

        printf("Error binding for clients.\n");

    } else {

        printf("Bind successfully!\n"); // the server binds clients successfully
        printf(">\n");

    }

    return client_fd;

}



int sendError(int error_code, char *send_buf, int sock_fd, struct sockaddr_in client_addr, int client_len) {

    int n;

    int len_err;
    int len;

    *(int *)(send_buf) = htons(5);
    *(int *)(send_buf+2) =  htons(error_code);

    char error_message_1[] = "FILE NOT FOUND\0";

    if (error_code == 1) {
        len_err = strlen(error_message_1);
        strncpy(send_buf+4, error_message_1, len_err);
        len = 4 + len_err;
    } else {
        return 0;
    }

    send_buf[len] = '\0';
    n = sendto(sock_fd, send_buf, len, 0, (struct sockaddr *) &client_addr, client_len ) ;

    return 0;

}



void sendPacket(FILE **fp, int block_num, char *send_buf, int sock_fd, struct sockaddr_in client_addr, int client_len) {

    int n;
    int res = 0;

    int opcode = 3;

    memset(send_buf, 0, 516);

    *(int *)send_buf = htons(opcode);
    *(int *)(send_buf + 2) = htons(block_num);

    char *buf;

    buf = (char*) malloc (DATA_SIZE_MAX); // allocate memory for the file

    res = fread (buf,1,DATA_SIZE_MAX,*fp); // put file to buffer

    if (res >= 0) {

        buf[res]='\0';
        printf("Result is %d.\n", res);

    }

    memcpy(send_buf + 4, buf, res);

    if (feof(*fp)) {

        fclose(*fp);
        *fp = NULL ;
        fflush(stdout);

    }

    n = sendto(sock_fd, send_buf, res + 4, 0, (struct sockaddr *) &client_addr, client_len ) ;

}



struct client* searchClient(int client_fd) {

    // searches an existent client in the client list

    int i = 0;

    for (i = 0; i < client_counter; i++) {

        if (client_fd == client_list[i].client_fd) {

            return &client_list[i];

        }

    }
    return NULL;
}



void addClient(int client_fd,FILE *fp) {

    // add a new client to the client list

	client_list[client_counter].client_fd=client_fd;
    client_list[client_counter].fp=fp;

    fflush(stdout);

    client_list[client_counter].block_no=1;
    client_counter++;

}



int clientRequest(int sock_fd) {

    // handles client request

	struct sockaddr_in client_addr;
	struct hostent *hostp;

    int n;
    int client_fd = 0;

    int len;
    socklen_t client_len;

    char send_buf[516] = {'\0'};
    char *buff;
    char *host_addrp;

    buff = (char *)malloc(sizeof(struct RRQPacket));
    client_len = sizeof(client_addr);

	// receive a UDP datagram from a client

	memset(buff, 0, 516);
	n = recvfrom(sock_fd, buff, sizeof(struct RRQPacket), 0, (struct sockaddr *) &client_addr, &client_len);
    printf("Got a client request...\n");

	uint16_t opcode;
	char file_name[512];
	uint16_t ack_num;

	memset(&opcode, 0, sizeof(uint16_t));
	memcpy(&opcode, buff, sizeof(uint16_t));
		    		    
	opcode = ntohs(opcode);

	if (opcode == 1) {

        if ((client_fd = createSocket()) > 0) {

//			printf("Client got socket descriptor: %d.\n",client_fd);

        }
                         
		memset(&file_name, 0, 512);
		memcpy(&file_name, buff + sizeof(uint16_t), 512);
		len = strlen(file_name);
		file_name[len]='\0';



		// determine which client sent the datagram

		hostp = gethostbyaddr((const char *)&client_addr.sin_addr.s_addr,
		sizeof(client_addr.sin_addr.s_addr), AF_INET);

		host_addrp = inet_ntoa(client_addr.sin_addr);


		fflush(stdout);
		printf("Filename %s: %s.\n", host_addrp, file_name);
		fflush(stdout);


		fp = fopen(file_name, "rb");
		addClient(client_fd,fp);
        struct client *client1;
        client1 = searchClient(client_fd);


        FILE *fp1;
		fp1 = fopen(file_name, "rb");
        if (fp1 != NULL) {

			fseek (fp1 , 0 , SEEK_END);
			fclose(fp1);

        }

		if ((client1->fp) == NULL) {

			n = sendError(1, send_buf, sock_fd, client_addr, client_len);
			printf("**File not found!**\n");

		} else {

			client1->block_no = 1;

            sendPacket(&(client1->fp), client1->block_no, send_buf, client1->client_fd, client_addr, client_len);
			if ((client1->fp) == NULL) {

				fflush(stdout);

			}
		}
	} else if (opcode == 4) {

        memset(&ack_num,0,sizeof(uint16_t));
		memcpy(&ack_num,buff + sizeof(uint16_t),sizeof(uint16_t));

		ack_num = htons(ack_num);

		fflush(stdout);

		printf("Ack num from client: %d.\n",ack_num);
		fflush(stdout);
        struct client *client2;
        client2 = searchClient(sock_fd);

  		if (client2 != NULL) {

            if (ack_num == client2->block_no) {

                fflush(stdout);
                client2->block_no = (client2->block_no+1) % 65536;
                fflush(stdout);

                if(client2->fp == NULL) {

                    printf("End of file transfer\n");
                    printf("***************************\n");
//                    fflush(stdout);

                } else if(client2->fp != NULL) {

                    if (feof(client2->fp)) {

                        fclose(client2->fp);
                        fp= NULL ;
                        fflush(stdout);

                    } else {

                        fflush(stdout);
                        sendPacket(&client2->fp, client2->block_no, send_buf, sock_fd, client_addr, client_len);

                    }
                }
            }
		}
	}

    return client_fd;
}



int main(int argc, char **argv) {

    int sock_fd;
    int client_fd;

    int port_no;

    struct sockaddr_in serveraddr;
	  
    int opt_val;

    fd_set master;
    fd_set read_fds;

    int select_val;


    int i;
    int fd_max;
    struct hostent* hret;

    // if the command line input is not ./Server <server_ip_address> <port_no.>

    if (argc != 3) {

        fprintf(stderr, "usage: %s <IP> <port>\n", argv[0]);
	    exit(1);

    }

    port_no = atoi(argv[2]);

    sock_fd = socket(AF_INET, SOCK_DGRAM, 0); // create the socket

    if (sock_fd < 0) {

        printf("ERROR on opening the socket.\n");

    }

    printf("\nSocket opened successfully!\n");
    fflush(stdout);


    opt_val = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (const void *) &opt_val, sizeof(int));


    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;

    hret = gethostbyname(argv[1]);
    memcpy(&serveraddr.sin_addr.s_addr, hret->h_addr,hret->h_length);
    serveraddr.sin_port = htons((unsigned short)port_no);


    if (bind(sock_fd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {

        printf("ERROR on binding.\n");

    } else {

        printf("Bind successfully!\n");

    }
	  
    FD_SET(sock_fd, &master);
    fd_max = sock_fd;


    // wait for a packet

    while (1) {

        read_fds = master;


        if ((select_val = select(fd_max + 1, &read_fds, NULL, NULL, NULL)) == -1) {

		    perror("select");
		    exit(4);

        }

        for (i = 0; i <= fd_max; i++) {

            if (FD_ISSET(i, &read_fds)) {

                if (i == sock_fd) {

                    // set up a new client connection

                    client_fd = clientRequest(i);
					FD_SET(client_fd, &master);

                    if (client_fd > fd_max) {

                        fd_max = client_fd;

                    }

                } else {

                    client_fd = clientRequest(i);

                }

            }

        }

	}

}
