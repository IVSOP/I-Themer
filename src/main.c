#include <stdlib.h>
#include <stdio.h>
#include "rofi.h"

// for now filepath does nothing, it is hardcoded
int main (int argc, char **argv) {
	// arg 1: database location
	if (argc == 0) {
		printf("Not enough args\n");
		return 1;
	}

	return mainRofiLoop(argv[1]);
}
