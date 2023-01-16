#include "dataInsert.h"

// MISSING LOTS OF ERROR CHECKING

void createDataFromTable(char *);

int insertData(char *filename) {
	// printf("For now, selective insertion is not supported, this is a basic tool to create the data table.\
	//  Please type 0 to make the entire table here (not supported), or 1 to make a useable database from a file with only the table\n");
	// int option, res = scanf("%d", &option);
	// if (res == 0) return 1;
	// if (option == 0) {
	// 	printf("WIP\n");
	// 	return 1;
	// } else {
	// 	createDataFromTable(filename);
	// }
	createDataFromTable(filename);

	return 0;
}

void createDataFromTable(char *filename) {
// 	printf("Make sure table has the following format (FIRST LINE IS ALLWAYS IGNORED):\n\
// 	active  sub_categories   on_click	0	1	2	3\n\
// string;int.int;...;...\n\
// keep in mind all that is not int is considered string. [...] denotes lists.\n");
// 	printf("Enter file path with your table\n");
	// char str[64];
	// int res = scanf("%s", str);//, n_of_param, n_of_themes;
	char str[] = "data/table.tb";
	// if (res == 0) {
	// 	printf("input error");
	// 	return;
	// }
	FILE *fp = fopen(str, "r");
	// error checking???

	while (fgetc(fp) != '\n'); // skip first line

	Data *data = parseMainTable(fp);

	// function 1: runs until first table ends
	// function 2: runs until file ends, trying to find the subtables
	// data structure: g_hash_tables for everything, structs are {string, int, int, {void *, TYPE},...}



	fclose(fp);
}
