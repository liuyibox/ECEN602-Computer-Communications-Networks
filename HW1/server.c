/*
** server.c -- a simple socket server
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT "6001"  // the port to connect to
#define BACKLOG 10   // number of pending connections
#define MAXDATASIZE 25  // the maximum data size

//signal handling
void sigchld_handler(int s)
{
    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

/* write n chars to the socket */
int writen(int fdes, const char *vptr, size_t len) {


    int lenleft; // chars left
    int lenwritten; // chars written
    const char *ptr;

    ptr = vptr;
    lenleft = len;

    while (lenleft > 0) {

        if ((lenwritten = write(fdes, ptr, lenleft)) <= 0) {

            if (lenwritten < 0 && errno == EINTR) {

                lenwritten = 0;

            } else {

                return -1;
            }
        }

        lenleft -= lenwritten;
        ptr += lenwritten;

    }


    return len;
}

/* read data from the client and echoes it back to the client */
void echo(int sockfd)
{
    char buffer[MAXDATASIZE];
    ssize_t len; // buffer size
    loop:

    while ((len = read(sockfd, buffer, MAXDATASIZE)) > 0) {

        writen(sockfd, buffer, len);

    }


    // when read an EOF, child process exits
    if (len == 0) {

        printf("a client is exiting...\n");

    } else if (len < 0 && errno == EINTR) {

        goto loop;

    } else if (len < 0) {

        perror("read");
    }
}

/* get sockaddr, IPv4 or IPv6: */
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {

        return &(((struct sockaddr_in*)sa)->sin_addr);  // IPv4
    }


    return &(((struct sockaddr_in6*)sa)->sin6_addr);  // IPv6
}


int main(int argc, char **argv)
{
    //here we initialize some variables
    int sockfd, new_fd;  // listen on socket descriptor sock_fd, new connection on descriptor socket new_fd
    char s[INET6_ADDRSTRLEN];
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    struct sigaction sa;
    socklen_t sin_size;
    int rv;
    int yes = 1;

    //initialize the hints structure
    memset(&hints, 0, sizeof hints);  // ensure the struct is empty
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;  // socket type
    hints.ai_flags = AI_PASSIVE;  // fill in IP

    // get address struct
    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }


    // loop and bind to the first
    for (p = servinfo; p != NULL; p = p->ai_next) {

        // construct a socket
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {

            perror("server: socket");
            continue;
        }

        //setup the socket
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {

            perror("setsockopt");
            exit(1);
        }

        // blind it to the port
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {

            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo); // done and free

    if (p == NULL)  {

        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    //server need to listen first
    if (listen(sockfd, BACKLOG) == -1) {

        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {

        perror("sigaction");
        exit(1);
    }

    //now server is ready to connect
    printf("server: waiting for connections...\n");

    while(1) {

        sin_size = sizeof their_addr;


        // accept a new connection
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

        //do we accept successfully?
        if (new_fd == -1) {

            perror("accept");
            continue;
        }

        //here we check if we have get the correct IP address
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);

        printf("server: connection at %s\n", s);

        if (!fork()) { // child process

            //we let the child hold the socket fd
            close(sockfd);
            echo(new_fd); // read data from the client and echoes it back to the client

            //after the echo function, we close the socket fd
            close(new_fd);


            exit(0);
        }

        close(new_fd);
    }
}
