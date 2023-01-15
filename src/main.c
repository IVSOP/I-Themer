#include <stdlib.h>
#include <stdio.h>
#include "dataRead.h"
#include "dataInsert.h"

#define INSERT '0'
#define READ '1'
#define ROFI '2'

int main (int argc, char **argv) {
	// arg 1: database location
	// agr 2: mode (insert/read/rofi-read)
	if (argc == 0) {
		printf("Not enough args\n");
		return 1;
	}
	char mode = *(argv[2]);
	if (mode == READ) {
		return parser(argv[1]);
	} else if (mode == INSERT) {
		return insertData(argv[1]);
	} else {
		return 6;
	}
	
    
    return 0;
}
