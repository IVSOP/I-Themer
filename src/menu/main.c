#include <stdlib.h>
#include <stdio.h>
#include "rofi.h"

int main (int argc, char **argv) {
	if (argc <= 1) return mainRofiLoop(NULL);
	return mainRofiLoop(argv[1]);
}