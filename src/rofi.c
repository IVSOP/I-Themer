#include "rofi.h"
#include <unistd.h>
#include <string.h>

#define SEP1 putchar('\0')
#define SEP2 putchar('\x1f')
#define TABLE_PATH "I-Themer/data/table.tb"
#define INFO_SIZE 512

// SEE HOW POWERMENU SCRIPT HANDLES ICONS

void printMainOptions(Data *data) {
	int i;
	char * home = getenv("HOME");
	int len = getNumberOfColors(data);
	for (i = 0; i < len; i++) {
		printf("Theme %d --> %d", i, getActivePerTheme(i));
		SEP1;
		printf("info");
		SEP2;
		printf("theme%d", i);
		SEP2;
		printf("icon");
		SEP2;
		printf("%s/%s\n", home, getColor(data, i));
	}
}

void printThemeOptions(Data *data, int theme) {
	SEP1;
	printf("prompt");
	SEP2;
	printf("Theme %d\nAll", theme);
	SEP1;
	printf("icon");
	SEP2;
	printf("%s/%s\n", getenv("HOME"), getColor(data, theme));

	generateThemeOptions(data, theme);
}

void inputHandler(Data *data, char *input) {
	char * original_info = getenv("ROFI_INFO");
	if (input != NULL && strncmp("query", input, 5) == 0) {
		queryHandler(data, input + 6);
	}
	if (original_info == NULL) { // ""
		printMainOptions(data);
	} else {
		// when varHandler calls displayVar, what happens if string overflows????????
		// info used big alloca just to be safe
		char *info = alloca(sizeof(char) * INFO_SIZE);
		strcpy(info, original_info);

		int i;
		for (i = 0; info[i] != '\0' && info[i] != '/'; i++);
		if (info[i] == '\0') { // "theme<x>"
			printThemeOptions(data, atoi(info + 5));
		} else { // "theme<x>/option(<m>)/..." m can be 0(apply), 1(show_sub) or 2(show_var) 0=0  1=115 2=118 so just /59
			int j;
			for (j = i + 1; info[j] != '('; j++);
			handlerFunc *handlers[] = {applyHandler, subHandler, varHandler};
			handlers[(int)(info[j + 1] - 48)](data, info, i + 1);
		}
	}
}

int mainRofiLoop(char *input) {
	FILE *fp = fopen(TABLE_PATH, "r");

	GPtrArray *colorArr = parseColors("color-icons");

	Data *data = parseMainTable(fp, colorArr);

	inputHandler(data, input);

	// dumpTable(data, 0);
	saveTableToFile(data, "table"); // this is inneficient, but it is the only way. doing it on applyHandler would break when data is actually a subtable and not a main table
	freeTableData(data);
	g_ptr_array_free(colorArr, TRUE);
	fclose(fp);
	return 0;
}
