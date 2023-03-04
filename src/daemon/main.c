#include <stdlib.h>
#include <stdio.h>
#include "rofi.h"
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <signal.h>
#include <syslog.h>

#define PORT 8080

// Signal handler function for SIGTERM

// void sigterm_handler(int signum) {
//     syslog(LOG_INFO, "Received SIGTERM signal. Terminating...");
//     // Perform any necessary cleanup here...
//     exit(0);
// }

int main (int argc, char **argv) {
	// parse data
	if (argc < 2) {
		fprintf(stderr, "Insuficient arguments: need dataset path\n");
		exit(EXIT_FAILURE);
	}

	//////////////////////////////////////////////// parsing file

	char path[256];
	snprintf(path, 256 * sizeof(char), "%s/%s", argv[1], "table.tb");
	FILE *fp = fopen(path, "r");
	CHECK_FILE_ERROR(fp);

	GPtrArray *colorArr = parseColors("color-icons", argv[1]);

	Data *data = parseMainTable(fp, colorArr, argv[1]);
	fclose(fp);

	//////////////////////////////////////////////// creating socket

	int server_fd, client_fd, valread;
	struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = { 0 };
    char* hello = "Hello from server";
  
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
  
    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
  
    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((client_fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    valread = read(client_fd, buffer, 1024);
    printf("%s\n", buffer);
    send(client_fd, hello, strlen(hello), 0);
    printf("Hello message sent\n");
  
    // closing the connected socket
    close(client_fd);
    // closing the listening socket
    shutdown(server_fd, SHUT_RDWR);

    // Close syslog connection
    closelog();

	//////////////////////////////////////////////// creating daemon



    return 0;
}