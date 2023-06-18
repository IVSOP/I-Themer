#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <signal.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include "parsing.h"
#include "handlers.h"
#include "debug.h"
#include <arpa/inet.h>

#define PORT 8080

// these variables unfortunately have to be global to be passed to sigterm_handler
static Data *data;
static GPtrArray *colorArr;
static int server_fd, client_fd;
static OUT_STRING *message;
static char *dir;

// Signal handler function for SIGTERM
// SIGTERM cleans up and puts data into file
// SIGKILL will just kill it without writing data (I assume)
void sigterm_handler(int signum) {
    syslog(LOG_INFO, "Received SIGTERM signal. Terminating...");

	// saveTableToFile(data, "table", dir); // no time for anything else in case of shutdown apparently
	// freeTableData(data);
	// g_ptr_array_free(colorArr, TRUE);

	// // closing the connected socket
    // close(client_fd);
    // // closing the listening socket
    // // shutdown(server_fd, SHUT_RDWR);
	// close(server_fd);

	// free(message);

    // // Close syslog connection

    // closelog();
    exit(0);
}

// will have side-effects with this file's global variables
void parseData(char *dir) {
	char path[256];
	snprintf(path, 256 * sizeof(char), "%s/%s", dir, "table.tb");
	FILE *fp = fopen(path, "r");
	CHECK_FILE_ERROR(fp);

	colorArr = parseColors("color-icons", dir);

	data = parseMainTable(fp, colorArr, dir);
	fclose(fp);
}

// message is "<mode>/<remaining string>"
// q: query
// m: menu
// because it is string it is assumed to be null terminated
void messageHandler(char *buffer, OUT_STRING *res) {
	switch (buffer[0])
	{
	case 'q': // query
		queryHandler(data, buffer + 2, res);
		break;
	case 'm': // menu
		menuHandler(data, buffer + 2, res);
		break;

	case 'k': // kill
		sigterm_handler(SIGTERM);
		break;
	
	case 'w': // save to file (idk it probably uses $HOME to figure out the path)
		saveTableToFile(data, "table", dir);
		res->len = 5;
		strcpy(res->str, "Saved");
		// send(client_fd, "Saved", 5, 0);
		break;

	case 'd': // print debug of entire data, will not work in daemon mode as stdout is closed 
		// will improve in the future to dump into a file
		dumpTable(data, 0);
		close(client_fd);
		break;
	
	default:
		break;
		// exit(EXIT_FAILURE);

	}
}

int main (int argc, char **argv) {
	// parse data
	if (argc < 2) {
		fprintf(stderr, "Insuficient arguments: need dataset path\n");
		exit(EXIT_FAILURE);
	}

	//////////////////////////////////////////////// creating socket

	struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
  
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
	inet_pton(AF_INET, "127.14.14.14", &address.sin_addr);
    //address.sin_addr.s_addr =  INADDR_ANY;
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

	//////////////////////////////////////////////// parsing theme file

	dir = argv[1];
	parseData(dir);

	//////////////////////////////////////////////// syslog and trapping

	// Open a syslog connection
    openlog("ithemer-daemon", LOG_PID, LOG_LOCAL1); // LOG_DAEMON

	struct sigaction sa;
    // Set signal handler for SIGTERM using sigaction
	sa.sa_handler = sigterm_handler;
    sigemptyset(&sa.sa_mask); // means that signals will not be blocked??????????????
    sa.sa_flags = 0;
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("Could not set SIGTERM handler");
        exit(EXIT_FAILURE);
    }

	//////////////////////////////////////////////// creating daemon

	pid_t pid, sid;

    // Fork the process
    pid = fork();

    if (pid < 0) {
	// error
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        // Parent process
        exit(EXIT_SUCCESS);
    }

    // Set file mode creation mask to 0
    umask(0);

    // Create a new session
    sid = setsid();
    if (sid < 0) {
        exit(EXIT_FAILURE);
    }

    // Change the working directory to root
    if (chdir("/") < 0) {
        exit(EXIT_FAILURE);
    }

    // Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

	//////////////////////////////////////////////// listen for requests on the socket

	int valread;//, valsend;
	char buffer[STR_RESULT_SIZE];
	message = malloc(sizeof(OUT_STRING));
    while (1) {
		if ((client_fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
			perror("accept");
        	exit(EXIT_FAILURE);
		}

		while ((valread = read(client_fd, buffer, STR_RESULT_SIZE - 1)) > 0) {
			// reset since it is reused
			message->len = 0;

			buffer[valread] = '\0';
			// printf("Received %s, calling handler\n", buffer);
			messageHandler(buffer, message);
			// printf("Final message is:\n");
			// fflush(stdout);
			// write(STDOUT_FILENO, message->str, message->len);
			send(client_fd, message->str, message->len, 0);
			// sigterm_handler(SIGTERM);
		}
		// here????
		close(client_fd);
    }

    return 0;
}
