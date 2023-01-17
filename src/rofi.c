#include "rofi.h"
#include <unistd.h>
#include <string.h>

#define SEP1 putchar('\0')
#define SEP2 putchar('\x1f')

// SEE HOW POWERMENU SCRIPT HANDLES ICONS

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

void printThemeOptions(Data *data, char *theme) {
	char *endptr;
	long int res = strtol(theme, &endptr, 10);
	if (*endptr == '.') {
		printf("Theme not an int??\n");
		exit(5);
		res = res; // compiler warn (???)
	}
	int themeInt = atoi(theme);

	DataObjArray * arr = tableLookup(data, "color-icons");
	DataObj *dataobj = getDataObj(arr, themeInt + 1);
	SEP1;
	printf("prompt");
	SEP2;
	printf("Theme %d\nAll", themeInt);
	SEP1;
	printf("icon");
	SEP2;
	printf("%s/%s\n", getenv("HOME"), (char *)getValue(dataobj));

	// each iteration gets a struct with the theme currently displayed, and the original data
	LoopInfo info = {themeInt, data};
	g_hash_table_foreach(getTable(data), generateThemeOptions, (void *)&info);
	exit(5);
	printf("Back");
	SEP1;
	printf("info");
	SEP2;
	printf("main menu\n");
	// printf("Done\n");
}

// next thing to do: make each click do what the on_click section says
// then, clean up table.tb
// the end result should be a directory with files that the scripts will use
// those files should be soft links to other fiels
// those other files are the ones in the table
// this way few data gets copied

// 2: make individual buttons work
// 3: make 'All' button work
// 4: make 3 look pretty, make the line to be changed have underline, icon changes etc

// this should be changed to something faster
void inputHandler(Data *data, char *input) {
	if (strncmp("Theme", input, 5) == 0) { // selected theme from main menu
		printThemeOptions(data, input + 6);
	} else if (strncmp("Done", input, 4) == 0) {
		return;
	} else { // clicked an option like "background", info contains theme it was picked in
		executeChange(data, input);
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