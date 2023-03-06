#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>

#define PORT 8080
  
int main(int argc, char const* argv[])
{
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! because rofi only uses stdout
	dup2(STDOUT_FILENO, STDERR_FILENO);  // redirects stderr to stdout below this line

    int status, valread, client_fd;
    struct sockaddr_in serv_addr;
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }
  
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
  
    // Convert IPv4 and IPv6 addresses from text to binary
    // form
	 // this address is localhost
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }
  
    if ((status = connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) {
        perror("Connection Failed");
        return -1;
    }

	char *rofi_info = getenv("ROFI_INFO"),
    buffer[1024]; // assumed to not end in \0!!!!!
	buffer[0] = 'm';
	buffer[1] = '/';

	if (rofi_info != NULL) {
		char * res = stpncpy(buffer + 2, rofi_info, 1024 - 2 - 1);
    	send(client_fd, buffer, res - buffer, 0);
    	// printf("Message sent: %s\n", buffer);
	} else {
		send(client_fd, buffer, 2, 0);
	}
	// ????? -1 + 1
    valread = read(client_fd, buffer, 1024 - 1);
	buffer[valread + 1] = '\0';

    // printf("Message received:\n");
	// fflush(stdout);
	if (write(STDOUT_FILENO, buffer, valread) == -1) {
		perror("Error writing data");
	}
  
    // closing the connected socket
    close(client_fd);
    return 0;
}
