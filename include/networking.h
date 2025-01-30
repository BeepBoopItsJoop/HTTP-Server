#ifndef NETWORKING_H
#define NETWORKING_H

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFF_SIZE 1024 * 4
#define FILE_BUFF_SIZE 1024 * 8
#define HTTP_VERSION "HTTP/1.0"

// get sockaddr, IPv4 or IPv6:
void* get_in_addr(struct sockaddr* sa) {
     if (sa->sa_family == AF_INET) {
          return &(((struct sockaddr_in*)sa)->sin_addr);
     }

     return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int sendall(int s, const char* buf, size_t* len) {
     int totalsent = 0;
     int bytesleft = *len;
     int n;

     while (totalsent < *len) {
          n = send(s, buf + totalsent, bytesleft, 0);
          if (n == -1) break;

          totalsent += n;
          bytesleft -= n;
     }

     *len = totalsent;

     return (n == -1) ? -1 : 0;
}

#endif
