# Fermity
c native web server with secure connection with SSL

## How it works
when program run it calls main() function
### Main " int main() "
Main start server by using arguments as hostname and port.
Made some thread to serve some client request.
This is doing loop till client accepted by socket then call thread to serving_client.
### Serving Client "serving_client"
Doing loop till client socket is accepted