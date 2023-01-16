#include "tables.h"

// MISSING LOTS OF ERROR CHECKING

int mainRofiLoop(char *filename) {
	FILE *fp = fopen(filename, "r");
	while (fgetc(fp) != '\n'); // skip first line
	// Data *data = getData(fp);
	Data *data = parseMainTable(fp);
	parseDependecyTables(data, fp); // prob inefficient but it works so idc
	
	// dumpTable(data, 0);
	freeTableData(data);
	fclose(fp);
	return 0;
}