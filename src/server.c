#include <signal.h>
#include <stdbool.h>
#include <sys/wait.h>

#include "networking.h"
#include "request.h"

#define PORT "3490"
#define ROOT "../www"
#define BACKLOG 10

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

// TODO: remove this
void trim_newline(char* str) {
     size_t len = strlen(str);

     // Loop through the string backwards and remove any \n or \r
     while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
          str[len - 1] = '\0';  // Replace the last character with null terminator
          len--;                // Reduce the length to move towards the beginning of the string
     }
}

const char* get_reason_phrase(int code) {
     switch (code) {
          case 200:
               return "OK";
          case 403:
               return "Forbidden";
          case 404:
               return "Not found";
          case 405:
               return "Method not allowed";
          default:
               return "Unknown status code";
     }
}

void parse_request(char* buffer, HttpRequest* req) {
     // trim_newline(buffer);

     req->method = strtok(buffer, " ");
     req->path = strtok(NULL, " ");
     req->version = strtok(NULL, "\r\n");

     // Parse headers
     // TODO: do better parsing
     for (req->headers.count = 0;
          req->headers.count < MAX_HEADERS;
          req->headers.count++) {
          char* key = strtok(NULL, ":");
          char* value = strtok(NULL, "\r");
          if (key == NULL || value == NULL) break;

          key++[0] = ' ';
          value++[0] = ' ';

          req->headers.data[req->headers.count].key = key;
          req->headers.data[req->headers.count].value = value;
     }

     // TODO: Parse optional body
}

HttpResponse http_get(HttpRequest* req) {
     char filepath[BUFF_SIZE];
     snprintf(filepath, BUFF_SIZE, "%s%s%s", ROOT, (strcmp(req->path, "/") == 0 ? "/index" : req->path), ".html");
     // TODO: add file ext based on header

     FILE* file = fopen(filepath, "r");
     if (file == NULL) {
          perror("fopen");
          HttpResponse res = create_http_res(404, get_reason_phrase(404),
                                             HTTP_VERSION, "File not found!", NULL, strlen("File not found!"));
          res.headers.data[res.headers.count++] = (HttpHeader){"Content-Type", "text/plain"};

          return res;
     }

     HttpResponse res = create_http_res(200, get_reason_phrase(200), HTTP_VERSION, "", file, 0);
     // TODO: headers for content type!
     res.headers.data[res.headers.count++] = (HttpHeader){"Content-Type", "text/html"};

     return res;
}

HttpResponse perform_request(HttpRequest* req) {
     printf("DEBUG - Method: \"%s\", Path: \"%s\", Version: \"%s\"\n", req->method, req->path, req->version);

     // Prevent "../"
     if (strstr(req->path, "..")) {
          HttpResponse res = create_http_res(403, get_reason_phrase(403), HTTP_VERSION, "Permission denied!", NULL, strlen("Permission denied!"));

          res.headers.data[res.headers.count++] = (HttpHeader){"Content-Type", "text/plain"};

          return res;
     }

     if (!strcmp(req->method, "GET")) {
          return http_get(req);
          // TODO: implement POST
          // } else if (!strcmp(req->method, "POST")) {
          // return http_post();
     } else {
          fprintf(stderr, "Invalid argument: method %s unsupported\n", req->method);
          HttpResponse res = create_http_res(405, get_reason_phrase(405), HTTP_VERSION, "", NULL, 0);

          return res;
     }
}

void SendHttpResponse(int fd, HttpResponse* res) {
     char header_buffer[BUFF_SIZE];
     size_t header_size = 0;

     // Print status line
     header_size += snprintf(header_buffer + header_size, BUFF_SIZE - header_size, "%s %d %s\r\n", res->version, res->status, res->reason);

     // Print headers
     for (size_t i = 0; i < res->headers.count; i++) {
          header_size += snprintf(header_buffer + header_size, BUFF_SIZE - header_size, "%s: %s\r\n", res->headers.data[i].key, res->headers.data[i].value);
     }

     // TODO: maybe find a way to not do this lol
     char file_buffer[FILE_BUFF_SIZE];
     size_t body_size = 0;
     if (res->file) {
          // Read file contents and calculate the length
          body_size = fread(file_buffer, sizeof(char), FILE_BUFF_SIZE, res->file);
     } else {
          body_size = res->body_length;
     }

     // Add content length header
     header_size += snprintf(header_buffer + header_size, BUFF_SIZE - header_size, "%s: %zu\r\n", "Content-length", body_size);
     // Add blank line before the body
     header_size += snprintf(header_buffer + header_size, BUFF_SIZE - header_size, "\r\n");

     if (sendall(fd, header_buffer, &header_size) == -1) {
          perror("sendall");
          printf("Only %zu bytes of data were sent before an error in header!\n", header_size);
          if (res->file) fclose(res->file);
          return;
     }
     if (sendall(fd, (res->file ? file_buffer : res->body), &body_size) == -1) {
          perror("sendall");
          printf("Only %zu bytes of data were sent before an error in body!\n", body_size);
          if (res->file) fclose(res->file);
          return;
     }

     if (res->file) fclose(res->file);
}

void handleConnection(int fd) {
     char buffer[BUFF_SIZE] = {0};
     recv(fd, buffer, BUFF_SIZE, 0);
     printf("DEBUG - Received request\n - Start- \n%s\n - End -\n", buffer);

     // GET /index.html HTTP/1.0
     HttpRequest request;
     parse_request(buffer, &request);

     HttpResponse response = perform_request(&request);

     // TODO: should this return an error?
     SendHttpResponse(fd, &response);
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

     // Reap zombie child processes after fork()ing
     struct sigaction sa;
     sa.sa_handler = sigchld_handler;
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
          int id = fork();
          if (!id) {
               close(sockfd);

               handleConnection(new_fd);

               close(new_fd);
               exit(0);
          }

          close(new_fd);
     }
     return 0;
}
