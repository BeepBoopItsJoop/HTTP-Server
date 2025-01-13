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
