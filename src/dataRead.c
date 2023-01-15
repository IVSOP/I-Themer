#include "dataRead.h"
#include <stdlib.h>
#include <stdio.h>

int parser (char *filename) {
	FILE *fp = fopen(filename, "r");
	if (fp == NULL) {
		printf("Database file opening error\n");
		return 1;
	}
	int n_of_themes, n_of_lines;
	size_t len = fread(&n_of_themes, sizeof(int), 1, fp);
	len = fread(&n_of_lines, sizeof(int), 1, fp);
	if ( ferror(fp) != 0 ) {
		fputs("Error reading file", stderr);
		fclose(fp);
		return 1;
	}
	// I assume small values
	int *offsets = alloca((size_t) (n_of_themes * n_of_lines));
	len = fread(offsets, sizeof(int), n_of_lines, fp);

	printf("Parsed header: %d themes, %d lines, offsets are ", n_of_themes, n_of_lines);
	int i;
	for (i = 0; i < n_of_themes * n_of_lines; i++) {
		printf("%d ", offsets[i]);
	}
	putchar('\n');

	// dont forget to skip the next \n

	fclose(fp);
	return 0;
	if (len == 0) return 1; // idk, wanted to avoid compiler errors without disabling
}