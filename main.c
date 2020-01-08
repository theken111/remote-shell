/* host_2.c*/

/******************************************************************
 * Remote Shell - Client Program                                  *
 ******************************************************************/

#include <curses.h>
#include <ncurses.h>
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
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <pthread.h>



#define MAX_SLEEP               10
#define BUFFER_SIZE             256
#define DEFAULT_SERVER_PORT		5000
#define DEFAULT_PROTOCOL        0
#define RESULT_BUFFER           65500
#define RCV_TIMEOUT             1
#define RS_PROMPT_CHAR          "$"
#define LOGIN_CMD               "login"

FILE *LaunchLogWindow()
{
    char *name = tempnam(NULL, NULL);
    char cmd[256];

    mkfifo(name, 0777);
    if(fork() == 0)
    {
    sprintf(cmd, " xterm -e cat %s", name);
    system(cmd);


   exit(0);
   }

    return fopen(name, "w+");
}

void error(const char* msg)
{
    perror(msg);
    exit(1);
}

void input(const char* arg)
{
    printf("usage: %s -h <host> -p <port> ", arg);
    exit(0);
}

void instruction()
{

    printf("|     Remote Shell - Client     |\n");

}

int receive_msg(int fd, char* buf)
{
    long int n,total=0;

    do {
        n = recv(fd,&buf[total],RESULT_BUFFER-total-1,0);
        total += n;
        if(total >= RESULT_BUFFER-1) {
            printf("Buffer overflow.\n");
            exit(1);
        }
    } while(n > 0);

    return total;
}

int kbhit(void)
{
  struct termios oldt, newt;
  int ch;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  if(ch != EOF)
  {
    ungetc(ch, stdin);
    return 1;
  }

  return 0;
}

int main(int argc, char* argv[])
{


    int ret;
    int sockfd, serverLen, total;
    struct sockaddr_in serverINETAddress;   /* Server Inernet address */
    struct sockaddr* serverSockAddrPtr;     /* Ptr to server address */
    struct hostent* server;                 /* hostent structure */

    char command[BUFFER_SIZE], host[100], buf[RESULT_BUFFER];


    int port = DEFAULT_SERVER_PORT, result;
    unsigned long inetAddress;  /* 32-bit IP address */

    /* Ignore SIGINT and SIGQUIT */
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);

    printf("Nhap dia chi server\n");
    gets(host);


    serverSockAddrPtr = (struct sockaddr*) &serverINETAddress;
    serverLen = sizeof(serverINETAddress);
    bzero((char *) &serverINETAddress, serverLen);
    serverINETAddress.sin_family=AF_INET;
    serverINETAddress.sin_port=htons(port);

    inetAddress = inet_addr(host);      /* get internet address */
    if(inetAddress != INADDR_NONE) {    /* is an IP address */
        serverINETAddress.sin_addr.s_addr = inetAddress;
    }
    else {      /* is a "hostname" */
        server = gethostbyname(host);
        if(server == NULL) {
            fprintf(stderr, "ERROR, no such host\n");
            exit(0);
        }
        bcopy((char*)server->h_addr,
                (char*)&serverINETAddress.sin_addr.s_addr, server->h_length);
    }

    sockfd = socket(AF_INET,SOCK_STREAM,DEFAULT_PROTOCOL);  /* create socket */
    if (sockfd < 0) error("ERROR opening socket");

    /* Loop until connection is made with the server OR timeout */
    int nsec=1;
    do {
        result = connect(sockfd, serverSockAddrPtr, serverLen);
        if(result == -1) { sleep(1); nsec++; }
    } while(result == -1 && nsec <= MAX_SLEEP);

    if(nsec > MAX_SLEEP) error("Connection timeout");

    /* Set socket to timeout on recv if no data is available */
    struct timeval tv;
    tv.tv_sec = RCV_TIMEOUT;    /* set timeout */
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
    (struct timeval*) &tv, sizeof(struct timeval));

    instruction();      /* Display instructions */



    while(1) {
        printf("%s ", RS_PROMPT_CHAR);
        bzero(command, sizeof(command));
        bzero(buf, sizeof(buf));
        fgets(command, BUFFER_SIZE, stdin);
        if (strcmp(command, "exit\n") == 0) break;


    if (strcmp(command, "top\n") == 0){ FILE *log = LaunchLogWindow();
                                        if( fork()==0){
                                        while(1){
                                                  if(send(sockfd,"top -n 1",sizeof(command),0) < 0)
                                                  error("ERROR sending message");
                                                  total = receive_msg(sockfd, buf);
                                                  fflush(log);
                                                  if(total>0 ) {
                                                      buf[total]='\0';
                                                      printf("%s",command);
                                                      fprintf(log,"%s\n", buf);
                                                      }
                                                  exit(0);
                                                  }


                                        }
                                       }
     else{
          if(send(sockfd,command,sizeof(command),0) < 0)
                           {error("ERROR sending message");}
          /* Receive message from server */
          total = receive_msg(sockfd, buf);
          if(total>0 ) {
          buf[total]='\0';
          printf("%s\n", buf);
          }
    }

    }

    close (sockfd);

    return 0;
}


