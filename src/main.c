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
	if (argc == 0) {
		printf("Not enough args\n");
		return 1;
	}

	if (argc == 0) return mainRofiLoop(NULL);
	return mainRofiLoop(argv[1]);
}

// when varHandler calls displayVar, what happens if string overflows????????
// original info may need gigantic alloca just to be safe
