full details in pcc.pdf

this code implements a toy client/server architecture: a printable characters counting
(PCC) server. Clients connect to the server and send it a stream of bytes. The server counts how
many of the bytes are printable and returns that number to the client. The server also maintains
overall statistics on the number of printable characters it has received from all clients. When the
server terminates, it prints these statistics to standard output.

Server (pcc_server):
-

The server accepts TCP connections from clients. A client that connects
sends the server a stream of N bytes (N is determined by the client and is not a global constant).
The server counts the number of printable characters in the stream. Once the stream ends, the server sends the count back to
the client over the same connection. In addition, the server maintains a data structure in which it
counts the number of times each printable character was observed in all the connections. When
the server receives a SIGINT, it prints these counts and exits.

Server specification
-

Command line arguments:
    • argv[1]: server’s port (assume a 16-bit unsigned integer).

 Client (pcc_client):
 -
The client creates a TCP connection to the server and sends it the contents
of a user-supplied file. The client then reads back the count of printable characters from the
server, prints it, and exits.

Client specification
-

Command line arguments:
    • argv[1]: server’s IP address (assume a valid IP address).
    • argv[2]: server’s port (assume a 16-bit unsigned integer).
    • argv[3]: path of the file to send.

