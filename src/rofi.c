#include "rofi.h"
#include <unistd.h>

// MISSING LOTS OF ERROR CHECKING

void printMainOptions(Data *data) {
	DataObjArray * arr = tableLookup(data, "color-icons");
	int i;
	DataObj *dataobj;
	int status;
	for (i = 1; (dataobj = getDataObj(arr, i)) != NULL; i++) {
		printf("Theme %d --> %d", i - 1, getActivePerTheme(i-1));
		putchar('\0');
		printf("icon");
		putchar('\x1f');
		printf("%s/%s\n", getenv("HOME"), (char *)getValue(dataobj));
	}
	status = status;
}

int mainRofiLoop(char *filename) {
	// FILE *fp = fopen(filename, "r");
	FILE *fp = fopen("data/table.tb", "r");
	while (fgetc(fp) != '\n'); // skip first line
	// Data *data = getData(fp);
	Data *data = parseMainTable(fp);
	parseDependecyTables(data, fp); // prob inefficient but it works so idc

	printMainOptions(data);

	// dumpTable(data, 0);
	freeTableData(data);
	fclose(fp);
	return 0;
}