#include "rofi.h"
#include <unistd.h>
#include <string.h>

#define SEP1 putchar('\0')
#define SEP2 putchar('\x1f')

// SEE HOW POWERMENU SCRIPT HANDLES ICONS

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

	generateThemeOptions(data, themeInt);
	printf("Back\n");
}

// this should be changed to something faster
void inputHandler(Data *data, char *input) {
	char * info;
	if (strncmp("Theme", input, 5) == 0) { // selected theme from main menu
		printThemeOptions(data, input + 6);
	} else if (strncmp("Done", input, 4) == 0) {
		return;
	} else if (strncmp("Back", input, 4) == 0) {
		info = getenv("ROFI_INFO");
		if (info == NULL) printMainOptions(data); // back to main menu
		else { // back to previous option (for now, goes back to theme selection)
			printThemeOptions(data, info + 6); // Theme x
		}
	} else if (strncmp("Theme", (info = getenv("ROFI_INFO")), 5) == 0) { // applied an option, going back to theme selection
		printThemeOptions(data, info + 6);
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