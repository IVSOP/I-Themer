#include "rofi.h"
#include <unistd.h>
#include <string.h>

#define SEP1 putchar('\0')
#define SEP2 putchar('\x1f')
#define TABLE_PATH "I-Themer/data/table.tb"

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
		printf("theme%d", i - 1);
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

	generateThemeOptions(data, theme);
}

void inputHandler(Data *data, char *input) {
	char * info = getenv("ROFI_INFO");
	if (info == NULL) { // ""
		printMainOptions(data);
	} else {
		int i;
		for (i = 0; info[i] != '\0' && info[i] != '/'; i++);
		if (info[i] == '\0') { // "theme<x>"
			printThemeOptions(data, atoi(info + 5));
		} else { // "theme<x>/option(<m>)" m can be 0(apply), 1(show_sub) or 2(show_var) 0=0  1=115 2=118 so just /59
			int j;
			for (j = i + 1; info[j] != '('; j++);
			handlerFunc *handlers[] = {applyHandler, subHandler, varHandler};
			handlers[(int)(info[j + 1] - 48)](data, info, i + 1);
			// default:
			// 	printf("Error\n");
			// 	break;
		}
	}
	
	// if (strncmp("Theme", input, 5) == 0) { // selected theme from main menu
	// 	printThemeOptions(data, input + 6);
	// } else if (strncmp("Done", input, 4) == 0) {
	// 	return;
	// } else if (strncmp("Back", input, 4) == 0) {
	// 	info = getenv("ROFI_INFO");
	// 	if (info == NULL) printMainOptions(data); // back to main menu
	// 	else { // back to previous option (for now, goes back to theme selection)
	// 		printThemeOptions(data, info + 6); // Theme x
	// 	}
	// } else if (strncmp("query", input, 5) == 0) { //format: query-number-arg1-arg2;...
	// 	queryHandler(data, input + 6);
	// // WILL CRASH IF INFO IS NULL
	// } else if (strncmp("Theme", (info = getenv("ROFI_INFO")), 5) == 0) { // applied an option, going back to theme selection
	// 	printThemeOptions(data, info + 6);
	// } else { // clicked an option like "background", info contains theme it was picked in
	// 	executeChange(data, input);
	// }
}

int mainRofiLoop(char *input) {
	// FILE *fp = fopen(filename, "r");
	FILE *fp = fopen(TABLE_PATH, "r");
	while (fgetc(fp) != '\n'); // skip first line
	// Data *data = getData(fp);
	Data *data = parseMainTable(fp);
	parseDependecyTables(data, fp); // prob inefficient but it works so idc

	inputHandler(data, input);

	// dumpTable(data, 0);
	freeTableData(data);
	fclose(fp);
	return 0;
}