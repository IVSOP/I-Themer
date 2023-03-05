#include "rofi.h"
#include "display.h"
#include <unistd.h>
#include <string.h>
#include "handlers.h"
#include "debug.h"

#define TABLE_PATH "I-Themer/data/table.tb"
#define INFO_SIZE 512

// SEE HOW POWERMENU SCRIPT HANDLES ICONS

// void inputHandler(Data *data, char *input, char *dir) {
// 	char * original_info = getenv("ROFI_INFO");
// 	if (input != NULL && strncmp("query", input, 5) == 0) {
// 		char *input2 = alloca(sizeof(char) * INFO_SIZE);
// 		strcpy(input2, input); // same explanation as bellow

// 		queryHandler(data, input2);
// 	} else {
// 		if (original_info == NULL) { // ""
// 			printMainOptions(data);
// 		} else {
// 			// when varHandler calls displayVar, what happens if string overflows????????
// 			// info used big alloca just to be safe
// 			char *info = alloca(sizeof(char) * INFO_SIZE);
// 			strcpy(info, original_info);

// 			int i;
// 			for (i = 0; info[i] != '\0' && info[i] != '/'; i++);
// 			if (info[i] == '\0') { // "theme<x>"
// 				printThemeOptions(data, atoi(info + 5));
// 			} else { // "theme<x>/option(<m>)/..." m can be 0(apply), 1(show_sub) or 2(show_var) 0=0  1=115 2=118 so just /59
// 				int j;
// 				for (j = i + 1; info[j] != '('; j++);
// 				handlerFunc *handlers[] = {applyHandler, subHandler, varHandler, allHandler};
// 				handlers[info[j + 1] - '0'](data, info, i + 1);
// 			}
// 		}
// 		// is file closed at this time?????????????
// 		saveTableToFile(data, "table", dir);
// 	}
// }

// int mainRofiLoop(char *input, char *dir) {
// 	FILE *fp = fopen(TABLE_PATH, "r");
// 	CHECK_FILE_ERROR(fp);

// 	GPtrArray *colorArr = parseColors("color-icons", dir);

// 	Data *data = parseMainTable(fp, colorArr, dir);

// 	// dumpTable(data, 0);

// 	inputHandler(data, input, dir);

// 	freeTableData(data);
// 	g_ptr_array_free(colorArr, TRUE);
// 	fclose(fp);
// 	return 0;
// }
