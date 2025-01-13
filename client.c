// find first available socket
// connect
// receive
// do other stuff
// close

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFF_SIZE 1024
#define MAXDATASIZE 100  // max number of bytes we can get at once

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
     if (sa->sa_family == AF_INET) {
          return &(((struct sockaddr_in *)sa)->sin_addr);
     }
     
     return &(((struct sockaddr_in6 *)sa)->sin6_addr);
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

int main(int argc, char *argv[]) {

     char s[INET6_ADDRSTRLEN];

     if (argc != 4) {
          fprintf(stderr, "usage: client hostname port filepath\n");
          exit(1);
     }
     const char* HOSTNAME = argv[1];
     const char* PORT = argv[2];
     const char* FILEPATH = argv[3];

     // get address info
     struct addrinfo hints;
     memset(&hints, 0, sizeof hints);
     hints.ai_family = AF_UNSPEC;
     hints.ai_socktype = SOCK_STREAM;

     struct addrinfo* servinfo; 
     int rv;
     if ((rv = getaddrinfo(HOSTNAME, PORT, &hints, &servinfo)) != 0) {
          fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
          return 1;
     }

     int sockfd;
     struct addrinfo* p;
     // loop through all the results and connect to the first we can
     for (p = servinfo; p != NULL; p = p->ai_next) {
          sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
          if (sockfd == -1) {
               perror("client: socket");
               continue;
          }

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

     inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s,
               sizeof s);
     printf("client: connecting to %s\n", s);

     freeaddrinfo(servinfo);  // all done with this structure

     char reqbuf[BUFF_SIZE] = "GET /test.html";
     sprintf(reqbuf, "GET %s", FILEPATH);
     size_t reqlen = BUFF_SIZE;

     if(sendall(sockfd, reqbuf, &reqlen) == -1) {
          perror("sendall");
          printf("Only %zu bytes of data were sent before an error!\n", reqlen);
     }

     FILE* file = fopen("receivedsite.html", "w");
     if(file == NULL) {
          perror("fopen");
          exit(1);
     }

     char buf[MAXDATASIZE];
     int bytes_recv;
     int bytes_written;
     while(1) {
          bytes_recv = recv(sockfd, buf, MAXDATASIZE - 1, 0);
          if(bytes_recv == 0) {
               break;
          }
          if(bytes_recv == -1) {
               perror("recv");
               exit(1);
          }

          // TODO: overenginner or whateer idk atp
          bytes_written = fwrite(buf, sizeof(char), bytes_recv, file);
          if(bytes_written != bytes_recv) {
               perror("fwrite");
               break;
          }
     }
     fclose(file);

     close(sockfd);

     return 0;
}
