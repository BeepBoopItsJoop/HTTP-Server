#ifndef REQUEST_T
#define REQUEST_T

#include <stddef.h>
#include <string.h>
#define MAX_HEADERS 100

typedef struct {
     // Header-Name: Header-Value
     char* key;
     char* value;
} HttpHeader;

// TODO: make this a dynamic array
typedef struct {
     HttpHeader data[MAX_HEADERS];
     size_t count;
} HttpHeaders;

typedef struct {
     // Request line
     const char* method;
     const char* path;
     const char* version;

     // Headers
     HttpHeaders headers;

     // Request Body (optional)
     char* body;
     size_t body_length;
} HttpRequest;

typedef struct {
     // Status line
     int status;
     const char* reason;
     const char* version;

     // Headers
     HttpHeaders headers;

     const char* body;
     FILE* file;
     size_t body_length;
} HttpResponse;

HttpResponse create_http_res(int status, const char* reason, const char* version, const char* body, FILE* file, size_t body_length) {
     HttpResponse res;
     res.status = status;
     res.reason = reason;
     res.version = version;
     res.body = body;
     res.file = file;
     res.body_length = body_length;

     memset(&res.headers, 0, sizeof(res.headers));

     return res;
}

#endif  // REQUEST_T
