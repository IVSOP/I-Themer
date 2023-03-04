#include <stdlib.h>
#include <stdio.h>
#include "rofi.h"
#include <unistd.h>

int main (int argc, char **argv) {
	// arg 1: database location

	if (chdir(getenv("HOME")) == -1) {
		printf("Error in cd home\n");
		exit(1);
	}

	if (argc == 0) return mainRofiLoop(NULL);
	return mainRofiLoop(argv[1]);
}
