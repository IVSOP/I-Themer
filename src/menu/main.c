#include <stdlib.h>
#include <stdio.h>

int main (int argc, char **argv) {
	if (argc == 0) return mainRofiLoop(NULL);
	return mainRofiLoop(argv[1]);
}