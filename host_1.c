/* host_1 */

/******************************************************************
 * Remote Shell - Server Program                                  *
                                     *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUFFER_SIZE                     256
#define QUEUE_LENGTH                    5
#define DEFAULT_SERVER_PORT             5000
#define LOGIN_CMD                       "login"
#define FALSE                           0
#define TRUE                            1

void error(const char* msg)
{
    perror(msg);
    exit(1);    /* Abort */
}

void input(const char* arg)
{
    printf("usage: %s -p <port> -c </full/path/to/commands_file> ", arg);
    exit(0);
}

void instruction(int p)
{

    printf("| Server listening on port %d |\n", p);

}

int main(int argc, char* argv[])
{
    int sockfd, newsockfd, serverLen, clientLen;
    int port = DEFAULT_SERVER_PORT;                     /* port number */
    char buffer[BUFFER_SIZE], temp_buf[BUFFER_SIZE];    /* Buffers */
    char* cmd_file_pathname = NULL;                     /* Commands filename */

    struct sockaddr_in serverINETAddress;               /* Server Internet address */
    struct sockaddr* serverSockAddrPtr;	                /* Ptr to server address */
    struct sockaddr_in clientINETAddress;               /* Client Internet address */
    struct sockaddr* clientSockAddrPtr;                 /* Ptr to client address */

    /* Ignore death-of-child signals to prevent zombies */
    signal (SIGCHLD, SIG_IGN);


    /* Create the server Internet socket */
    sockfd = socket(AF_INET, SOCK_STREAM, /* DEFAULT_PROTOCOL */ 0);
    if(sockfd < 0) error("ERROR opening socket");

    /* Fill in server socket address fields */
    serverLen = sizeof(serverINETAddress);
    bzero((char*)&serverINETAddress, serverLen);            /* Clear structure */
    serverINETAddress.sin_family = AF_INET;                 /* Internet domain */
    serverINETAddress.sin_addr.s_addr = htonl(INADDR_ANY);  /* Accept all */
    serverINETAddress.sin_port = htons(port);               /* Server port number */
    serverSockAddrPtr = (struct sockaddr*) &serverINETAddress;

    /* Bind to socket address */
    if(bind(sockfd, serverSockAddrPtr, serverLen) < 0)
        error("ERROR on binding");

    /* Set max pending connection queue length */
    if(listen(sockfd, QUEUE_LENGTH) < 0) error("ERROR on listening");

    clientSockAddrPtr = (struct sockaddr*) &clientINETAddress;
    clientLen = sizeof(clientINETAddress);

    instruction(port);  /* Display instructions */

    while(1) {  /* Loop forever */
        /* Accept a client connection */
        newsockfd = accept(sockfd,/*NULL*/clientSockAddrPtr,/*NULL */&clientLen);
        if(newsockfd < 0) error("ERROR on accept");

        printf("Server %d: connection %d accepted\n", getpid(), newsockfd);

        int logged = FALSE; /* not logged in */

        if(fork() == 0) {   /* Create a child to serve client */
            /* Perform redirection */
            int a = dup2(newsockfd, STDOUT_FILENO);
            int b = dup2(newsockfd, STDERR_FILENO);
            if(a < 0 || b < 0) error("ERROR on dup2");

            while(1) {
                bzero(buffer, sizeof(buffer));

                /* Receive */
                if(recv(newsockfd, buffer, sizeof(buffer), 0) < 0)
                    error("ERROR on receiving");



                if (strcmp(buffer, "exit\n") == 0) {    /* Exit */
                    close(newsockfd);
                    exit(0);
                }

                /* Tokenize command */
                bzero(temp_buf, sizeof(temp_buf));
                strcpy(temp_buf, buffer);
                char* p = strtok(temp_buf, " ");    /* token (cmd) */

                if(strcmp(p, LOGIN_CMD)==0) continue;   /* Ignore LOGIN_CMD */



                /* Manage 'cd' */
                if(strcmp(p, "cd")== 0 ) {
                   p = strtok(NULL, " ");  /* This is the new path */
                    p[strlen(p)-1] = '\0';  /* replace newline termination char */

                    /* Change current working directory */
                    if(chdir(p) < 0) perror("error");
                }
                else system(buffer);    /* Issue a command */
                     }
        }
        else close (newsockfd); /* Close newsocket descriptor */
    }

    close(sockfd);

    return 0;   /* Terminate */
}
