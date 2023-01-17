#include "parsing.h"

#define SEP1 putchar('\0')
#define SEP2 putchar('\x1f')

struct DataObj {
	void *info;
	TYPE type;
};

struct Data {
	GHashTable *main_table;
};

//contains data from an entire line
struct DataObjArray {
	DataObj *arr;
	int len; // number of fiels so I know how many to free
	Data * dependency_table;
};

struct Theme {
	int big, small;
};

char *custom_strdup(char *src, int len);
void freeTableStruct(void *data);
void dumpDataObjArray(DataObjArray *, long int depth);
void dumpTableEntry(gpointer key, gpointer value, gpointer user_data);


char *readString(char *str, int *len) {
	int i;
	for (i = 0; str[i] != ';'; i++);
	str[i] = '\0';
	i++;
	(*len) += i;
	char * res = malloc(sizeof(char) * i);
	return memcpy(res, str, i);
}

int readInt(char *str, int *len) {
	char *endptr;
	long int res = strtol(str, &endptr, 10);
	(*len) += (int)(endptr - str);
	return (int)res;
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
DataObjArray *parseLineString(char *str, ssize_t strlen) {
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
					Theme *theme = malloc(sizeof(Theme));
					theme->big = (int)res;
					theme->small = readInt(str + i, &offset);
					tmp->info = theme;
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
	final->dependency_table = NULL;
	return final;
}

// will malloc everything for you, with a pointer to an array of DataObj that
// contains info on an entire line
// will return NULL if it reads nothing (like if it is on an empty line)
// this is not efficient at all but makes it way easier to deal with a recursive type
// getline can be replaced by a version that uses a buffer on the stack or something idk
DataObjArray *parseLine(FILE *fp) {
	char *linestr = NULL;
	size_t buffsiz = 0;
	ssize_t len_chars = getline(&linestr, &buffsiz, fp);

	if (len_chars == -1 || len_chars == 1) { // EOF and empty line, respectively (is EOF check needed??)
		free(linestr);
		return NULL;
	}

	DataObjArray *res = parseLineString(linestr, len_chars);

	free(linestr);
	return res;
	buffsiz = buffsiz;
}

// returns NULL on EOF
Data *parseMainTable(FILE *fp) {
	Data *data = malloc(sizeof(Data));
	// key destroy func is NULL since they will be freed when the remaining data is freed (they are shared)
	GHashTable * table = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, freeTableStruct);
	DataObjArray *lineData = parseLine(fp);
	DataObj *tmp;
	int max_len = 0;
	while (lineData != NULL) {
		tmp = &(lineData->arr)[0];
		if (tmp->type == LIST || tmp->type == INT ) {
			fprintf(stderr, "For now the hash table is designed to use strings in the hashing function, please start all lines with a string\nError thrown in parsing.c:parseMainTable\n");
			exit(1);
		}
		if (max_len < lineData->len) max_len = lineData->len;
		g_hash_table_insert(table, tmp->info, lineData);
		lineData = parseLine(fp);
	}

	if (max_len == 0) { // EOF
		free(data);
		g_hash_table_destroy(table);
		return NULL;
	}
	data->main_table = table;
	// data->n_of_themes = max_len-2;
	return data;
}

// copies first string until ':'
// returns NULL on EOF, else returns the input string
char *getDependencyString(char *str, FILE *fp) {
	int chr, i;
	
	for (i = 0; (chr = fgetc(fp)) != ':' && chr != EOF; i++) {
		str[i] = chr;
	}
	str[i] = '\0';
	
	if (chr == EOF) return NULL;
	chr = fgetc(fp); // skip over '\n' after the ':'
	return str;
}

void parseDependecyTables(const Data *data, FILE *fp) {
	char *str = alloca(sizeof(char) * BUFFER_SIZE); // compiler didnt like the fact that it was array and not pointer
	DataObjArray * lookup_res;

	while ((str = getDependencyString(str, fp)) != NULL) {
		lookup_res = g_hash_table_lookup(data->main_table, str);
		if (lookup_res == NULL) {
			fprintf(stderr, "Sub-table found but no entry in main table corresponds to it. Found:'%s'\n", str);
			exit(1);
		} else {
			lookup_res->dependency_table = parseMainTable(fp);
			// printf("dependency table for %s:\n", str);
			// dumpTable(lookup_res->dependency_table);
			// printf("%s dependency table ended\n", str);
		}
	}	
}

void freeNULL (void *data) {
	return;
}

// frees DataObjArray, NOT Data
void freeTableStruct(void *data) {
	// if (data == NULL) return;
	DataObjArray *dataArr = (DataObjArray *)data;
	DataObj *arr = dataArr->arr, *tmp; // what a mess
	int i;
	freeFunc *freeDispatchTable[] = {freeNULL, free, free, freeNULL, freeTableStruct};
	for (i = 0; i < (const int)dataArr->len; i++) {
		tmp = &arr[i];
		freeDispatchTable[tmp->type](tmp->info);
	}
	if (dataArr->dependency_table != NULL) freeTableData((Data *)dataArr->dependency_table);
	free(arr);
	free(dataArr);
}

//frees Data *
void freeTableData(Data *data) {
	if (data != NULL) g_hash_table_destroy(data->main_table);
	free(data);
}

void printSpace(int depth) {
	for (; depth; depth--) putchar(' ');
}

void dumpDataObjArray(DataObjArray * data, long int depth) {
	DataObj *arr = data->arr, *tmp;
	int i;
	TYPE type;
	printSpace(depth);
	printf("[--------------[%ld]\n", depth);
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
			dumpDataObjArray((DataObjArray *)tmp->info, depth + 4);
		} else if (type == EMPTY) {
			printf("empty\n");
		} else if (type == INT_VERSION) {
			printf("version: %d.%d\n", ((Theme *)tmp->info)->big, ((Theme *)tmp->info)->small);
		}
	}
	if (data->dependency_table != NULL) {
		printSpace(depth);
		printf("Found dependency table\n");
		dumpTable((Data *)data->dependency_table, depth + 4);
	}
	printSpace(depth);
	printf("[--------------[%ld]-[%d]\n", depth, i);
}

void dumpTableEntry(gpointer key, gpointer value, gpointer user_data) {
	dumpDataObjArray((DataObjArray *)value, (long int)user_data);
}

void dumpTable(Data *data, long int depth) {
	g_hash_table_foreach(data->main_table, dumpTableEntry, (void *)depth);
}

DataObjArray *tableLookup(Data *data, char *str) {
	return g_hash_table_lookup(data->main_table, str);
}

// incomplete
void printValue(DataObj *data) {
	TYPE type = data->type;
	if (type == INT) {
		printf("%ld", (long int)data->info);
	} else if (type == STRING) {
		printf("%s", (char *)data->info);
	}
}

int getLen(DataObjArray *data) {
	return data->len;
}

DataObj *getDataObj(DataObjArray *data, int i) {
	if (i >= data->len) return NULL;
	return &(data->arr)[i];
}

// TODO
int getActivePerTheme(int theme) {
	return -1;
}

void *getValue(DataObj *data) {
	return data->info;
}

// theme being browsed is passed as info
// what to do if slect theme that was already selected?
void generateThemeOptions(gpointer key, gpointer value, gpointer user_data) {
	if (strcmp("color-icons", (char *)key) == 0) return;

	LoopInfo *info = (LoopInfo *)user_data;
	DataObj *obj = ((DataObjArray *)value)->arr;
	DataObj *colorArr = (tableLookup(info->data, "color-icons"))->arr;

	printf("%s", (char *)(obj[0].info));
	SEP1;
	printf("info");
	SEP2;
	printf("%d", info->selected_theme);
	SEP2;
	printf("icon");
	SEP2;
	// no matter subtheme, colors is of the main theme
	long int theme;
	if (obj[1].type == INT) {	
		theme = (long int)(obj[1].info);
	} else { // INT_VERSION
		theme = ((Theme *)obj[1].info)->big;
	}
	printf("%s/%s\n", getenv("HOME"), (char *)(colorArr[theme + 1].info));
}

GHashTable *getTable(Data *data) {
	return data->main_table;
}

// checks if there needs to be a change
// if so, applies it immediately (for simplicity)
void executeChange(Data *data, char * input) {
	DataObjArray *dataobjarray = tableLookup(data, input);
	DataObj *themeobj = &(dataobjarray->arr)[1];
	if (themeobj->type == INT) {
		long int original_theme = (long int)themeobj->info;
		printf("Executing change for %s, from theme %d to theme %s", input, (int)original_theme, getenv("ROFI_INFO"));
	} else if (themeobj->type == INT_VERSION) {
		Theme * original_theme = (Theme *)themeobj->info;
		printf("Executing change for %s, from theme %d.%d to theme %s", input, original_theme->big, original_theme->small, getenv("ROFI_INFO"));
	} else {
		printf("Error in '%s' (%s)\n", __func__, input);
	}
}
