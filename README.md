# Nicholas LaMonica - CSCI 4273 Network Systems PA3  

This is a simple multi-threaded web proxy coded in c.  

## How to run:  
1. make
2. ./webproxy {port num} {cache ttl}
3. enable http requests to run through this proxy in the settings of your computer.


## Files:
### webproxy.c 
This file contains all of the logic for initializing the server and running infinitely.
### threads.c
This file has three functions:  
1. client is a function that is run by the threads that serve as the client proxy interface
2. server is a function that is run by the threads that serve as the server proxy interface
3. send_request is a function called by server threads, this function builds a connection and sends a request to a server

### helpers.c
A few helper methods pertaining to the cache and the blacklist. 

### blacklist.txt
This is the file that contains all of the blacklisted urls, each url must have it's own line in the file. You need to be very precise about which urls you want to block
If you want to blacklist 'example.net' you're entry into the file must be 'http://example.net'. Likewise, if you want to blacklist 'www.example.net', the entry must be 'http://www/example.net'



## Design choices
### Multi-threading (producer-consumer problem)
I accomplished concurrency in this proxy by multithreading with condition variables and mutexes. It was similar to the producer-consumer problem with a shared buffer. Where the 'producers' are actually the threads that interface with the client and the buffer, while the 'consumers' are threads that interface with the buffer and the external server.

The client interface threads are responsible for taking incoming requests. It will then check if the request has been blacklisted, checking for cached requests. If the request is cached, send a response with what is in the cache. if the request is not cached, put the request into a shared buffer for the server threads to handle.  

The server interface threads are responsible for taking requests that have been placed into the shared buffer by the client threads. It will then build a connection and and send it's request to the server. Finally it will cache the server's request.

### The cache and blacklist
For the cache I decided to have a fixed TTL determined as a command line argument. I decided to not cache requests where the urls contained a '?' in it, since that usually signifies a dynamic page.    
The blacklist is simply a character array that is read into at the beginning of the program.