# HTTP 0.9
Project for understanding socket networking and the protocol by building an HTTP server and client from scratch
## Server
Per 0.9, supports the GET method to receive an ASCII stream of bytes to receive and html page
# Usage
* Compile using Cmake
* Run the server to receive connections and requests on `localhost`
* Connect to the server using `netcan`, `curl` or the client
* Run the client with a port and a filepath 
## Example
* Run `server` inside of `build/`
* Run `netcat localhost 3490` in terminal followed by `GET /index.html`
* Or run `./build/client localhost 3490 /index.html` and open `/build/receivedsite.html`

# HTTP 1.0
## TODO:

* [x] Request structure
	* [x] Write
* [x] Parsing
	* [x] using new structs
	* [x] version
	* [x] path without filetype
	* [x] filetype - stores in `Content-Type`
* [x] Refactor GET
* [x] Decide method function based on Request.method
* [x] SendRes function
     * [x] Version, Status, Reason
     * [x] `\r\n`
     * [x] Add `Content-length` header 
          * [x] Calculate body length
     * [x] Loop through headers
          * [x] Add `\r\n` at the end of each one
     * [x] Add a blank line
     * [x] Read and send file or body
* [x] Status codes
	* [x] Get reason phrase dynamically from enum
- [x] Research 
	- [x] Caching
* [ ] Handle filetype dynamically
     * [ ] Set content type header based on extension
* [ ] New methods
	* [] POST
* [ ] Refactor HttpHeaders to be dynamically allocated
* [ ] Implement chunked transfer encoding
