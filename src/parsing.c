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

char *readString(char *str, int *len) {
	int i = *len;
	for (i = 0; str[i] != ';'; i++);
	str[i] = '\0';
	i++;
	(*len) += i;
	char * res = malloc(sizeof(char) * i);
	return memcpy(res, str, i);
}

// int readStringDelim(FILE *fp, char delim, char *buffer) {
// 	int i, chr;
//     for (i = 0; (chr = fgetc(fp)) != ';' && chr != '\n' && chr != delim; i++) {
// 		buffer[i] = chr;
// 	}
// 	buffer[i] = '\0';
// 	if (chr == '\n') return 1;
// 	if ((char)chr == delim) return 2;
// 	if (chr == ';') return 3;
// 	return 0;
// }

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

// takes in string for (what is thinks) is the entire line and turns it into DataObjArray *, mallocing as needed
// the string includes the \n in [strlen - 1]
DataObjArray *parseLineString(char *str, size_t strlen) {
	DataObj arr[DATA_BUFF_SIZE], *tmp;
	int len = 0, // number of structs in the array
	i;
	char chr;
	str[strlen-1] = ';';

	for (i = 0; i < (int)strlen; i++) {
		tmp = &arr[len];
		// printf("current string: %s (%d)", str + i, i);
		chr = str[i];
		if (chr == '[') {
			int nested = 1, j;
			for (j = i + 1; j < (int)strlen && nested > 0; j++) {
				if (str[j] == '[') nested++;
				else if (str[j] == ']') nested--;
			}
			// at his stage, str[i until j] has the string that needs to be parsed into a list
			// str[i] is at '[' and str[j] is at ';' after ']'
			tmp->type = LIST;
			tmp->info = parseLineString(str + i + 1, j - i - 1);
			i = j;
		} else if (chr == ';') {
			tmp->type = EMPTY;
			tmp->info = NULL;
		} else {
			char *endptr;
			long int res = strtol(str + i, &endptr, 10);
			if (endptr != str + i) { // did read int
				if (*endptr == '.') {
					int offset = 0;
					tmp->type = INT_VERSION;
					tmp->info = readString(str + i, &offset);
					i += offset;
				} else {
					tmp->type = INT;
					tmp->info = (void *) res;
					i++;
				}
			} else {
				int offset = 0;
				tmp->type = STRING;
				tmp->info = readString(str + i, &offset);
				i += offset - 1;
			}
		}
		len++;
	}

	DataObjArray *final = malloc(sizeof(DataObjArray));
	final->len = len;
	final->arr = malloc(len*sizeof(DataObj));
	final->arr = memcpy(final->arr, arr, len*sizeof(DataObj));
	return final;
}

// will malloc everything for you, with a pointer to an array of DataObj that
// contains info on an entire line
// will return NULL if it reads nothing (like if it is on an empty line)
// this is not efficient at all but makes it way easier to deal with a recursive type
// getline can be replaced by a version that uses a buffer on the stack or something idk
DataObjArray *parseLine(FILE *fp) {
	char *linestr = NULL;
	size_t buffsiz = 0, len_chars = getline(&linestr, &buffsiz, fp);

	// empty line
	if (len_chars == 1) {
		free(linestr);
		return NULL;
	}

	DataObjArray *res = parseLineString(linestr, len_chars);

	free(linestr);
	return res;
	buffsiz = buffsiz;
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
