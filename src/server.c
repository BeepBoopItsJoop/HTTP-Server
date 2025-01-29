#include <signal.h>
#include <stdbool.h>
#include <sys/wait.h>

#include "networking.h"

#define PORT "3490"
#define ROOT "./www"
#define BACKLOG 10

void trim_newline(char* str) {
     size_t len = strlen(str);

     // Loop through the string backwards and remove any \n or \r
     while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
          str[len - 1] = '\0';  // Replace the last character with null terminator
          len--;                // Reduce the length to move towards the beginning of the string
     }
}
void parse_request(char* buffer, char** method, char** path) {
     trim_newline(buffer);

     // TODO: parse method function
     *method = strtok(buffer, " ");
     // TODO: parse path function
     *path = strtok(NULL, " ");
}

// GET method function
const char* serve_file(int fd, const char* path) {
     // Prevent "../"
     if (strstr(path, "..")) {
          return "<plaintext>HTTP/0.9 403 Forbidden\r\n\r\nAccess Denied!";
     }

     char filepath[BUFF_SIZE];
     snprintf(filepath, BUFF_SIZE, "%s%s", ROOT, path);

     FILE* file = fopen(filepath, "r");
     if (file == NULL) {
          perror("serve_file");
          return "<plaintext>HTTP/0.9 404 Not Found\r\n\r\nFile Not Found!";
     }

     char buffer[BUFF_SIZE];
     size_t bytes_read;

     // Read and send content in chunks
     size_t bytes_sent;
     while ((bytes_read = fread(buffer, sizeof(char), BUFF_SIZE, file)) > 0) {
          bytes_sent = bytes_read;
          if (sendall(fd, buffer, &bytes_sent) == -1) {
               perror("sendall");
               printf("Only %zu bytes out of %d were sent before an error!\n",
                      bytes_sent, BUFF_SIZE);
          }
     }

     fclose(file);

     return "";
}

const char* perform_request(int fd, const char* method, const char* filepath) {
     printf("DEBUG:\nMethod - %s, Path - %s\n", method, filepath);

     const char* response;
     if (strcmp(method, "GET") != 0) {
          printf("Invalid argument: method %s unsupported\n", method);
          response = "<plaintext>HTTP/0.9 405 Method Not Allowed\r\n\r\n";
          return response;
     }

     // TODO: currently assumes method is GET, add more methods in future
     response = serve_file(fd, filepath);

     return response;
}

bool addrConfig(struct addrinfo** servinfo) {
     struct addrinfo hints;

     memset(&hints, 0, sizeof(hints));
     hints.ai_family = AF_UNSPEC;
     hints.ai_socktype = SOCK_STREAM;
     hints.ai_flags = AI_PASSIVE;  // Use machine IP

     // get address info for this host machine
     int rv;
     if ((rv = getaddrinfo(NULL, PORT, &hints, servinfo)) != 0) {
          fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
          return 1;
     }
     return 0;
}

bool openSockBind(struct addrinfo* servinfo, int* sockfd) {
     // loop through result linked list and bind to the first one we can
     struct addrinfo* p;
     for (p = servinfo; p != NULL; p = p->ai_next) {
          *sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
          if (*sockfd == -1) {
               perror("server: socket");
               continue;
          }

          // configure socket info
          int yes = 1;
          int setres =
              setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
          if (setres == -1) {
               perror("setsockopt");
               return (1);
          }

          int bindres = bind(*sockfd, p->ai_addr, p->ai_addrlen);
          if (bindres == -1) {
               close(*sockfd);
               perror("server: bind");
               continue;
          }

          break;
     }

     if (p == NULL) {
          fprintf(stderr, "server: failed to bind\n");
          return (1);
     }
}

void sigchld_handler(int s) {
     // waitpid() might overwrite errno, so we save and restore it:
     int saved_errno = errno;

     while (waitpid(-1, NULL, WNOHANG) > 0);

     errno = saved_errno;
}

void handleConnection(int fd) {
     char buffer[BUFF_SIZE] = {0};
     recv(fd, buffer, BUFF_SIZE, 0);

     // GET /index.html
     char *method, *path;
     printf("DEBUG:\nRequest: %s\n", buffer);
     parse_request(buffer, &method, &path);

     const char* response = perform_request(fd, method, path);
     size_t reslen = strlen(response);

     if (sendall(fd, response, &reslen) == -1) {
          perror("sendall");
          printf("Only %zu bytes of data were sent before an error!\n", reslen);
     }
}

int main(void) {
     struct addrinfo* servinfo;
     if (addrConfig(&servinfo)) {
          return 1;
     }

     int sockfd;
     if (openSockBind(servinfo, &sockfd)) {
          return 1;
     }

     freeaddrinfo(servinfo);

     int lisres = listen(sockfd, BACKLOG);
     if (lisres == -1) {
          perror("listen");
          exit(1);
     }

     // black magic
     struct sigaction sa;
     sa.sa_handler = sigchld_handler;  // reap all dead processes
     sigemptyset(&sa.sa_mask);
     sa.sa_flags = SA_RESTART;
     if (sigaction(SIGCHLD, &sa, NULL) == -1) {
          perror("sigaction");
          exit(1);
     }

     printf("server: waiting for connections...\n");

     // Main loop
     while (1) {
          struct sockaddr_storage client_addr;
          socklen_t sin_size = sizeof(client_addr);

          int new_fd =
              accept(sockfd, (struct sockaddr*)&client_addr, &sin_size);
          if (new_fd == -1) {
               perror("accept");
               continue;
          }

          char s[INET6_ADDRSTRLEN];
          inet_ntop(client_addr.ss_family,
                    get_in_addr((struct sockaddr*)&client_addr), s, sizeof(s));
          printf("server: got connection from %s\n", s);

          // Handle connection in a seperate process
          if (!fork()) {
               close(sockfd);

               handleConnection(new_fd);

               close(new_fd);
               exit(0);
          }

          close(new_fd);
     }
     return 0;
}
