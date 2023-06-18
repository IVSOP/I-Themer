#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>

#define PORT 8080
  
int main(int argc, char const* argv[])
{

	if (argc < 2) {
		fprintf(stderr, "Insuficient arguments: need query information\n");
		exit(EXIT_FAILURE);
	}

    int status, valread, client_fd;
    struct sockaddr_in serv_addr;
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }
  
    serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
  
    // Convert IPv4 and IPv6 addresses from text to binary
	// this address is localhost
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }
  
    while ((status = connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) {
        perror("Connection failed, retrying");
        // return -1;
		// sleep????????
    }

	char buffer[1024];
	buffer[0] = 'q';
	buffer[1] = '/';

	int i, size;
	for (i = 1; i < argc; i++) {
		size = stpncpy(buffer + 2, argv[i], 1024 - 2 - 1) - buffer;
		send(client_fd, buffer, size, 0);

		// -1 to keep space for \n, -2 to reserve "q/" // is \n needed??? wtf
		valread = read(client_fd, buffer + 2, 1024 - 1 - 2);
		buffer[valread + 2] = '\n';
		if (write(STDOUT_FILENO, buffer + 2, valread + 1) == -1) {
			perror("Error writing data");
		}

		// // buffer[valread + 2] = '\0';
		// puts(buffer + 2);
	}
  
    // closing the connected socket
    close(client_fd);
    return 0;
}
