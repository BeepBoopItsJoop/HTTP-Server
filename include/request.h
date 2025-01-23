struct Header {
	// Header-Name: Header-Value
	char* name;
	char* value;
};

struct Request {
// Reqeust line 
	char* method;
	char* path;
	char* verson;
// Headers
	Header* headers; // TODO: how many headers?
// Request Body (optional)
	char* body;
};

struct Response {
	// Status line
	char* version;
	int status;
	char* reason;
	
	// Header headers[];
	
	char* body;
};
