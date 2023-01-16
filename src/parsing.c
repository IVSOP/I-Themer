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
char *custom_strdup(char *src, int len);

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

inline char get_last_char(char *str) {
	while (*str != '\0') str++;
	return *(str--);
}

// this is pretty much a copy of parseSegment, got lazy
// returns: 0 normally
// 1 if end of list
// 2 if end of list coincides with end of line
int parseSegmentList(FILE *fp, DataObj *data) {
	exit(10);
}

// NOTE: assumes it is impossible to reach EOF or empty line
// this will probably come back to haunt me later
void *parseList(FILE *fp) {
	exit(10);
}

char *custom_strdup(char *src, int len) {
	char *dest = malloc(sizeof(char) * len);
	return memcpy(dest, src, len);
}

// fills a DataObj * with the respective data
// does not take int oconsideration completely empty line
void parseSegment(char *str, DataObj *data, int len) {
	if (len == 0) {
		data->type = EMPTY;
		data->info = NULL;
	} else {
		char *endptr;
		long int res = strtol(str, &endptr, 10);

		if (endptr != str) { // did read int
			if (*endptr == '.') {
				data->type = INT_VERSION;
				data->info = custom_strdup(str, len);
			} else {
				data->type = INT;
				data->info = (void *) res;
			}
		} else {
			if (*endptr == '[') exit(15); // should never happen
			else {
				data->type = STRING;
				data->info = custom_strdup(str, len);
			}
		}
	}
}


// will malloc everything for you, with a pointer to an array of DataObj that
// contains info on an entire line
// will return NULL if it reads nothing (like if it is on an empty line)
DataObjArray *parseLine(FILE *fp) {
	DataObj arr[DATA_BUFF_SIZE];
	char linestr[LINE_STR_SIZE];
	int i, chr;
	// this loop will check exactly which string to be sent to the segment parser,
	// and will also check if it should be sent do the list parser
	// this way its simple and allows to check for nested lists
	int len = 0, nested;
	for (i = 0; i < LINE_STR_SIZE && (chr = fgetc(fp)) != '\n'; i++) {
		if (chr == '[') { // list
			for (nested = 1; i < LINE_STR_SIZE && nested > 0; i++) { // for now this remains untested
				chr = fgetc(fp);
				linestr[i] = chr;
				if (chr == '[') nested++;
				else if (chr == ']') nested--;
			}
			chr = fgetc(fp); // char after the last ]
			if (chr == '\n') {
				chr = '\0';
				break;
			}
		} else if (chr == ';') { // normal end of segment
			linestr[i] = '\0';
			parseSegment(linestr, &arr[len], i);
			len++;
			i = -1;
		} else {
			linestr[i] = (char)chr;
		}
	}
	if (i == LINE_STR_SIZE) {
		printf("Line is too large, either increase LINE_STR_SIZE or make it dynamic");
		exit(1);
	} else if (i == 0) { // empty line
		return NULL;
	}

	// normal end of line
	// turn everything into do while??
	if (chr == '\n') {
		linestr[i] = '\0';
		parseSegment(linestr, &arr[len], i);
		len++;
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
