#include <stddef.h>
#define MAX_HEADERS 100

typedef struct {
     // Header-Name: Header-Value
     char* key;
     char* value;
} HttpHeader;

// TODO: make this a dynamic array
typedef struct {
     HttpHeader headers[MAX_HEADERS];
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
} Request;

typedef struct {
     // Status line
     const char* version;
     int status;
     const char* reason;

     HttpHeaders headers;

     char* body;
} Response;
