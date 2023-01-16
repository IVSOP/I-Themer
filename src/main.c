#include <stdlib.h>
#include <stdio.h>
#include "rofi.h"

int main (int argc, char **argv) {
	// arg 1: database location
	if (argc == 0) {
		printf("Not enough args\n");
		return 1;
	}

	if (argc == 0) return mainRofiLoop(NULL);
	return mainRofiLoop(argv[1]);
}
