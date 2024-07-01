# UDP transport Router:
The transport program connects to a specified server and, following the outlined protocol, sends UDP packets to retrieve a file of a specified size from the server. The program accepts four arguments: IP address, port, file name, and size (in bytes). The goal is to download the first size bytes of the file from a special UDP server listening on port at the address IP address and save these bytes to the file file name.

# Implementation:
The program is implemented in C++, utilizing raw sockets to send and receive UDP packets.

# Run the program with the required arguments:
sudo ./transport <IP_address> <port> <file_name> <size>
