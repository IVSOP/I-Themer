#include "rofi.h"
#include <unistd.h>
#include <string.h>

#define SEP1 putchar('\0')
#define SEP2 putchar('\x1f')

//		inputs and info:
// input = NULL: goes to main menu
// input = Theme X: goes to menu of theme X
// when clicked, the next input will just be its text so it cab be looked up

void printMainOptions(Data *data) {
	DataObjArray * arr = tableLookup(data, "color-icons");
	int i;
	DataObj *dataobj;
	char * home = getenv("HOME");
	for (i = 1; (dataobj = getDataObj(arr, i)) != NULL; i++) {
		printf("Theme %d --> %d", i - 1, getActivePerTheme(i - 1));
		SEP1;
		printf("info");
		SEP2;
		printf("theme:%d", i - 1);
		SEP2;
		printf("icon");
		SEP2;
		printf("%s/%s\n", home, (char *)getValue(dataobj));
	}
}

void printThemeOptions(Data *data, int theme) {
	DataObjArray * arr = tableLookup(data, "color-icons");
	DataObj *dataobj = getDataObj(arr, theme + 1);
	SEP1;
	printf("prompt");
	SEP2;
	printf("Theme %d\nAll", theme);
	SEP1;
	printf("icon");
	SEP2;
	printf("%s/%s\n", getenv("HOME"), (char *)getValue(dataobj));
	// need loop to print the [0] of each array, and display its color acording to first part of [1]

	// each iteration gets a struct with the theme currently displayed, and the original data
	LoopInfo info = {theme, data};
	g_hash_table_foreach(getTable(data), generateThemeOptions, (void *)&info);
}

void inputHandler(Data *data, char *input) {
	if (strncmp("Theme", input, 5) == 0) { // selected theme from main menu
		printThemeOptions(data, atoi(input + 6));
	}
}

int mainRofiLoop(char *input) {
	// FILE *fp = fopen(filename, "r");
	FILE *fp = fopen("data/table.tb", "r");
	while (fgetc(fp) != '\n'); // skip first line
	// Data *data = getData(fp);
	Data *data = parseMainTable(fp);
	parseDependecyTables(data, fp); // prob inefficient but it works so idc

	if (input == NULL) {
		printMainOptions(data);
	} else {
		inputHandler(data, input);
	}


	// dumpTable(data, 0);
	freeTableData(data);
	fclose(fp);
	return 0;
}