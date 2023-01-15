#include "parsing.h"
#include <glib.h>

struct DataObj {
	void *info;
	TYPE type;
};

//contains data from an entire line
struct DataObjArray {
	DataObj *arr;
	int len; // number of fiels so I know how many to free
};

struct Data {
	GHashTable *main_table;
	int n_of_themes;
	// n_of_param; not needed since you can just get the size of the hash table
};

void *parseList(FILE *fp);

// NONE OF THESE CURRENTLY CHECK FOR EOF
// will return 1 on newline
int readString(FILE *fp, char *buffer) {
	int i, chr;
    for (i = 0; (chr = fgetc(fp)) != ';' && chr != '\n'; i++) {
		buffer[i] = chr;
	}
	buffer[i] = '\0';
	if (chr == '\n') return 1;
	if (chr == ';') return 3;
	return 0;
}

// same as above but will return 2 if it finds delim
int readStringDelim(FILE *fp, char delim, char *buffer) {
	int i, chr;
    for (i = 0; (chr = fgetc(fp)) != ';' && chr != '\n' && chr != delim; i++) {
		buffer[i] = chr;
	}
	buffer[i] = '\0';
	if (chr == '\n') return 1;
	if ((char)chr == delim) return 2;
	if (chr == ';') return 3;
	return 0;
}

// this is pretty much a copy of parseSegment, got lazy
// are the outputs of this correct??
int parseSegmentList(FILE *fp, DataObj *data) {
	char str[BUFFER_SIZE], *endptr;
	int strres = readStringDelim(fp, ']', str);
	if (*str == '[') {
		data->type = LIST;
		data->info = parseList(fp);
	} else if (*str == '\0') {
		data->type = EMPTY;
		data->info = NULL;		
	} else {
		long res = strtol(str, &endptr, 10);
		if (endptr == str) { // read string or other
			data->type = STRING;
			data->info = strndup(str, BUFFER_SIZE);
		} else {
			if (*endptr == '.') {
				data->type = INT_VERSION;
				data->info = strndup(str, BUFFER_SIZE);
			} else {
				data->type = INT;
				data->info = (void *) res;
			}
		}
	}
	return strres;
}

void *parseList(FILE *fp) {
	DataObj arr[DATA_BUFF_SIZE];
	int res,  len = 0;
	do {
		res = parseSegmentList(fp, &arr[len]);
		len++;
	} while (res != 2);

	DataObjArray *final = malloc(sizeof(DataObjArray));
	final->len = len;
	final->arr = malloc(len*sizeof(DataObj));
	final->arr = memcpy(final->arr, arr, len*sizeof(DataObj));
	fgetc(fp); // SKIP OVER ';' THAT COMES AFTER ']'
	return final;
}

// returns 2 if line is empty
// 1 if reached end of line
int parseSegment(FILE *fp, DataObj *data) {
	char str[BUFFER_SIZE], *endptr;
	int strres = readStringDelim(fp, '[', str);
	// this is a mess but avoids unecessary calls to strtol

	// if (strres != 2) printf("string: '%s' res: %d *str: %d\n", str, strres, (int)*str); else printf("list\n");
	if (strres == 2) { // read '['
		data->type = LIST;
		data->info = parseList(fp);
	} else if (*str == '\0') {
		if (strres == 3) { // empty field
			data->type = EMPTY;
			data->info = NULL;
		} else if (strres == 1) { // entire line is empty
			printf("Returning as empty line\n");
			return 2;
		}
	} else {
		long int res = strtol(str, &endptr, 10);
		if (endptr == str) { // read string or other
			data->type = STRING;
			data->info = strndup(str, BUFFER_SIZE);
		} else {
			if (*endptr == '.') {
				data->type = INT_VERSION;
				data->info = strndup(str, BUFFER_SIZE);
			} else {
				data->type = INT;
				data->info = (void *) res;
			}
		}
	}
	if (strres == 1) { // normal end of line
		// printf("returning as normal end of line\n");
		return 1;
	}
	// printf("Returning normally\n");
	return 0;
}


// will malloc everything for you, with a pointer to an array of DataObj that
// contain info on an entire line
// will return NULL if it reads nothing (like if it is on an empty line)
DataObjArray *parseLine(FILE *fp) {
	DataObj arr[DATA_BUFF_SIZE];
	int res, len = -1;
	do {
		len++;
		res = parseSegment(fp, &arr[len]);
	} while (res == 0);
	if (res == 1) {
		len++;
		res = parseSegment(fp, &arr[len]);
	} else if (res == 2) { // empty line
		return NULL;
	}
	DataObjArray *final = malloc(sizeof(DataObjArray));
	final->len = len;
	final->arr = malloc(len*sizeof(DataObj));
	final->arr = memcpy(final->arr, arr, len*sizeof(DataObj));
	return final;
}

Data *parseMainTable(FILE *fp) {
	Data *data = malloc(sizeof(Data));
	GHashTable * table = g_hash_table_new_full(g_str_hash, g_str_equal, free, freeTableData);
	DataObjArray *lineData = parseLine(fp);
	while (lineData != NULL) {
		dumpDataObjArray(lineData, 0);
		g_hash_table_insert(table, (lineData->arr)[0].info, lineData);
		lineData = parseLine(fp);
	}
	return data;
}

void freeNULL (void *data) {
	return;
}

void freeString (void *data) {
	free(data);
}

void freeList(void *data) {
	printf("incomplete\n");
}

void freeTableData(void *data) {
	DataObjArray *dataArr = (DataObjArray *)data;
	DataObj *arr = dataArr->arr; // what a mess
	int i;
	freeFunc *freeDispatchTable[] = {freeNULL, freeString, freeString, freeNULL, freeList, freeNULL};
	for (i = 0; i < (const int)dataArr->len; i++) {
		freeDispatchTable[arr[i].type](arr[i].info);
	}
}

void printSpace(int depth) {
	for (; depth; depth--) putchar(' ');
}

void dumpDataObjArray(DataObjArray * data, int depth) {
	DataObj *arr = data->arr, *tmp;
	int i;
	TYPE type;
	printSpace(depth);
	printf("[--------------[%d]\n", depth);
	for (i = 0; i < (const int)data->len; i++) {
		tmp = &arr[i];
		type = tmp->type;
		printSpace(depth);
		printf("%d-", i);
		if (type == INT) {
			printf("int: %ld\n", (long int)tmp->info);
		} else if (type == STRING) {
			printf("string: %s\n", (char *)tmp->info);
		} else if (type == LIST) {
			printf("list:\n");
			dumpDataObjArray((DataObjArray *)tmp->info, depth+1);
		} else if (type == EMPTY) {
			printf("empty\n");
		}
	}
	printSpace(depth);
	printf("[--------------[%d]-[%d]\n", depth, i);
}

// FIXED current issue: example last fields of i3 and dunst not showing up (normal end of line)
// also, several lines are being treated as one, for example background and dunst
// end of list mixed with end of line is fucking everything up
