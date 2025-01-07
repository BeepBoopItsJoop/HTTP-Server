#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <sys/types.h>
// #include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>

#define PORT "3490"
#define BACKLOG 10

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

int main(void) {
     struct addrinfo hints, *servinfo;
     int rv;

     memset(&hints, 0, sizeof(hints));
     hints.ai_family = AF_UNSPEC;
     hints.ai_socktype = SOCK_STREAM;
     hints.ai_flags = AI_PASSIVE;

     if((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
          fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
          return 1;
     }

     int sockfd;
     struct addrinfo* p;
     for(p = servinfo; p != NULL; p = p->ai_next) {
          sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
          if (sockfd == -1) {
               perror("server: socket");
               continue;
          }

          int yes = 1; 
          int setres = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
          if (setres == -1) {
               perror("setsockopt");
               exit(1);
          }
          
          int bindres = bind(sockfd, p->ai_addr, p->ai_addrlen);
          if(bindres == -1) {
               close(sockfd);
               perror("server: bind");
               continue;;
          }     

          break;    
     }

     freeaddrinfo(servinfo);

     if(p == NULL) {
          fprintf(stderr, "server: failed to bind\n");
          exit(1);
     }

     int lisres = listen(sockfd, BACKLOG);
     if(lisres == -1) {
          perror("listen");
          exit(1);
     }

     struct sigaction sa;
     sa.sa_handler = sigchld_handler; // reap all dead processes
     sigemptyset(&sa.sa_mask);
     sa.sa_flags = SA_RESTART;
     if (sigaction(SIGCHLD, &sa, NULL) == -1) {
          perror("sigaction");
          exit(1);
    }

    printf("server: waiting for connections...\n");

}

