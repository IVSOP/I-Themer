#include <stdlib.h>
#include <stdio.h>
#include "tables.h"

int main (int argc, char **argv) {
	// arg 1: database location
	if (argc == 0) {
		printf("Not enough args\n");
		return 1;
	}

	return mainRofiLoop(argv[1]);
}
