#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define PORT "3490"
#define BACKLOG 10
#define BUFF_SIZE 1024

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void* get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int sendall(int s, const char* buf, size_t* len) {
     int totalsent = 0;
     int bytesleft = *len;
     int n;

     while(totalsent < *len) {
          n = send(s, buf+totalsent, bytesleft, 0);
          if(n == -1) break;
          
          totalsent += n; 
          bytesleft -= n; 
     }

     *len = totalsent;

     return (n == -1) ? -1 : 0;
}

int main(void) {
     struct addrinfo hints, *servinfo;
     int rv;

     memset(&hints, 0, sizeof(hints));
     hints.ai_family = AF_UNSPEC;
     hints.ai_socktype = SOCK_STREAM;
     hints.ai_flags = AI_PASSIVE; // Use machine IP

     // get address info for this host machine
     if((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
          fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
          return 1;
     }

     // loop through results and bind to the first one we can
     int sockfd;
     struct addrinfo* p;
     for(p = servinfo; p != NULL; p = p->ai_next) {
          sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
          if (sockfd == -1) {
               perror("server: socket");
               continue;
          }

          // configure socket info
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

     // black magic
     struct sigaction sa;
     sa.sa_handler = sigchld_handler; // reap all dead processes
     sigemptyset(&sa.sa_mask);
     sa.sa_flags = SA_RESTART;
     if (sigaction(SIGCHLD, &sa, NULL) == -1) {
          perror("sigaction");
          exit(1);
    }

    printf("server: waiting for connections...\n");

     // 
     int new_fd;
     struct sockaddr_storage client_addr;
     socklen_t sin_size = sizeof(client_addr);
     char s [INET6_ADDRSTRLEN];
     while(1) {
          new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
          if(new_fd == -1) {
               perror("accept");
               continue;
          }

          inet_ntop(client_addr.ss_family, 
               get_in_addr((struct sockaddr *)&client_addr), 
               s, sizeof(s));
          printf("server: got connection from %s\n", s);

          if(!fork()) {
               close(sockfd);

               // TODO: do something about buffer overflow
               char buffer[BUFF_SIZE] = {0};
               recv(new_fd, buffer, BUFF_SIZE, 0);
               printf("Request: %s\n", buffer);

               // TODO: Implement handling html file serving
               const char* response = 
                    "HTTP/0.9 200 OK\r\n"
                    "Content-Type: text/plain\r\n\r\n"
                    "Hello, World!";
               size_t reslen = strlen(response);     
               if(sendall(new_fd, response, &reslen) == -1) {
                    perror("sendall");
                    printf("Only %zu bytes of data were sent before an error!\n", reslen);
               }
               

               // char* str = "Hello, world!, again\n";
               // size_t len = strlen(str);
               // if (sendall(new_fd, str, &len) == -1) {
               //      perror("sendall");
               //      printf("Only %zu bytes of data were sent before an error!\n", len);
               // }

               close(new_fd);
               exit(0);
          }

          close(new_fd);
     }
     return 0;
}

