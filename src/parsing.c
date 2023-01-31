#include "parsing.h"
#include "rofi.h"
#include "display.h"
#include "debug.h"
#include "handlers.h"
#include "queries.h"

#define TABLE_PATH "I-Themer/data"

typedef void outputFunc(void*data, FILE *fp);

void freeTableStruct(void *data);
void outInt(void*data, FILE *fp);
void outString(void*data, FILE *fp);
void outVersion(void*data, FILE *fp);
void outEmpty(void*data, FILE *fp);
void outList(void*data, FILE *fp);

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

inline char get_last_char(char *str) {
	while (*str != '\0') str++;
	return *(str--);
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
			tmp->info = parseLineString(str + i + 1, j - i - 1); // wtf???
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
					// could both be stored inside void * since it is 8 bytes, but this is clearer
					Theme *theme = malloc(sizeof(Theme));
					theme->big = (int)res;
					theme->small = readInt(endptr + 1, &offset);
					tmp->info = theme;
					i += offset + 2; // ??????? idk
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

// THIS IS SKETCHY!!!!
inline int getThemeBig(DataObj *themeobj) {
	return (themeobj->type == INT ? (int)((long int)themeobj->info) : *(int *)themeobj->info);
}

// returns NULL on EOF
Data *parseMainTable(FILE *fp, GPtrArray *colorArr) {
	Data *data = malloc(sizeof(Data));
	int *active = calloc((colorArr->len + 1), sizeof(int));
	data->active = active;

	// key destroy func is NULL since they will be freed when the remaining data is freed (they are shared)
	GHashTable * table = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, freeTableStruct);
	DataObjArray *lineData = parseLine(fp);
	DataObj *tmp;
	int current_theme, biggest;
	while (lineData != NULL) {
		char mode = ((char *)(&lineData->arr[2])->info)[5];
		current_theme = getThemeBig(&lineData->arr[1]);
		if (mode == 's') { // mode is sub, needs its dependencies resolved
			char str[BUFFER_SIZE];
			snprintf(str, BUFFER_SIZE, "%s/%s.tb", TABLE_PATH, (char *)(&lineData->arr[0])->info);
			FILE *fp2 = fopen(str, "r");
			CHECK_FILE_ERROR(fp2)
			lineData->dependency_table = parseMainTable(fp2, colorArr);
			fclose(fp2);
			// need to update current selected theme, based on the most used theme
			// is sub, so can be sure it is an INT and not INT_VERSION
			if ((biggest = lineData->dependency_table->active[(const int)colorArr->len]) == current_theme) {
				// if the most used is the same as current, no need to update it
				active[current_theme] += 1;
			} else {
				// else have to change it and active[other theme] += 1
				(&lineData->arr[1])->info = (void *)((long int)biggest);
				active[biggest] += 1;
			}
			
		} else {
			active[current_theme] += 1;
			// printf("adding theme %d to %s\n", getThemeBig(&lineData->arr[1]), ((char *)(&lineData->arr[0])->info));
		}
		tmp = &(lineData->arr)[0];
		// if (tmp->type != STRING) {
		// 	fprintf(stderr, "For now the hash table is designed to use strings in the hashing function, please start all lines with a string\nError thrown in %s\n", __func__);
		// 	exit(1);
		// }
		
		// 0 will be checked anyway, just start at 1
		for (current_theme = 1, biggest = 0; current_theme < (const int)colorArr->len; current_theme++) {
			if (active[current_theme] > active[biggest]) biggest = current_theme;
		}
		active[current_theme] = biggest;

		g_hash_table_insert(table, tmp->info, lineData);
		lineData = parseLine(fp);
	}

	data->main_table = table;
	data->color_icons = colorArr;
	return data;
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
	if (data != NULL) {
		g_hash_table_destroy(data->main_table);
		free(data->active);
		free(data);
	}
}

DataObjArray *tableLookup(Data *data, char *str) {
	return g_hash_table_lookup(data->main_table, str);
}

int getLen(DataObjArray *data) {
	return data->len;
}

DataObj *getDataObj(DataObjArray *data, int i) {
	if (i >= data->len) return NULL;
	return &(data->arr)[i];
}

inline int getActivePerTheme(Data *data, int theme) {
	return (data->active)[theme];
}

void *getValue(DataObj *data) {
	return data->info;
}

GHashTable *getTable(Data *data) {
	return data->main_table;
}

void freeDataObj(DataObj *data) {
	freeFunc *freeDispatchTable[] = {freeNULL, free, free, freeNULL, freeTableStruct};
	freeDispatchTable[data->type](data->info);
}

void outList(void *data, FILE *fp) {
	DataObjArray *dataobjarray = (DataObjArray *)data;
	outputFunc *outDispatchTable[] = {outInt, outString, outVersion, outEmpty, outList};
	const DataObj *arr = dataobjarray->arr;
	int i, len = dataobjarray->len;
	fputc('[', fp);
	for (i = 0; i < len - 1; i++) {
		outDispatchTable[(&arr[i])->type]((&arr[i])->info, fp);
		fputc(';', fp);
	}
	outDispatchTable[(&arr[i])->type]((&arr[i])->info, fp);
	fputc(']', fp);
}

// kind of like outList except it runs first and doesnt print any '['
void outLine(DataObjArray *dataobjarray, FILE *fp) {
	outputFunc *outDispatchTable[] = {outInt, outString, outVersion, outEmpty, outList};
	const DataObj *arr = dataobjarray->arr;
	int i, len = dataobjarray->len;
	for (i = 0; i < len - 1; i++) {
		outDispatchTable[(&arr[i])->type]((&arr[i])->info, fp);
		fputc(';', fp);
	}
	outDispatchTable[(&arr[i])->type]((&arr[i])->info, fp);
	fputc('\n', fp);
	if (dataobjarray->dependency_table != NULL) {
		saveTableToFile(dataobjarray->dependency_table, (char *)(&arr[0])->info);
	}
}

void saveTableToFile(Data *data, char *name) {
	char str[BUFFER_SIZE];
	snprintf(str, BUFFER_SIZE, "%s/%s.tb", TABLE_PATH, name);
	FILE *fp = fopen(str, "w");
	CHECK_FILE_ERROR(fp);

	GHashTableIter iter;
	char *key = NULL;
	DataObjArray *current = NULL;

	g_hash_table_iter_init (&iter, data->main_table);
	while (g_hash_table_iter_next (&iter, (void **)&key, (void **)&current))
	{
		outLine(current, fp);
	}

	fclose(fp);
}

void outInt(void *data, FILE *fp) {
	char str[BUFFER_SIZE / 4];
	snprintf(str, BUFFER_SIZE / 4, "%ld", (long int)(data));
	fputs(str, fp);
}

void outString(void *data, FILE *fp) {
	fputs((char *)data, fp);
}

// print x.0 as x???
void outVersion(void *data, FILE *fp) {
	Theme *theme = (Theme *)data;
	char str[BUFFER_SIZE / 4];
	snprintf(str, BUFFER_SIZE / 4, "%d.%d", theme->big, theme->small);
	fputs(str, fp);
}

void outEmpty(void *data, FILE *fp) {
	return;
}

void changeTheme(DataObj *arr, int big, int small) {
	DataObj *themeobj = &arr[1],
	*infoObj = &arr[big + 3];

	if (infoObj->info == NULL) return; // if it is empty (or info == NULL) then do nothing

	if (themeobj->type == INT) {
		if (small == 0) {
			themeobj->info = (void *)((long int)big);
		} else {
			Theme *new = malloc(sizeof(Theme));
			new->big = big; new->small = small;
			themeobj->info = (void *)new;
			themeobj->type = INT_VERSION;
		}
	} else if (themeobj->type == INT_VERSION) {
		if (small == 0) {
			free(themeobj->info);
			themeobj->info = (void *)((long int)big);
			themeobj->type = INT;
		} else {
			Theme *theme = (Theme *)themeobj->info;
			theme->big = big;
			theme->small = small;
		}
	}
}

GPtrArray *parseColors(char *name) {
	GPtrArray *arr = g_ptr_array_new_full(3, free);
	char str[BUFFER_SIZE];
	snprintf(str, BUFFER_SIZE, "%s/%s", TABLE_PATH, name); // not .tb??
	FILE *fp = fopen(str, "r");
	CHECK_FILE_ERROR(fp);
	char *res = NULL;
	size_t size = 0;
	while (getline(&res, &size, fp) != -1) {
		g_ptr_array_add(arr, res);
		res = NULL;
		size = 0;
	}
	free(res); // wtf?????? why does getline use malloc on eof???
	fclose(fp);
	return arr;
}

// string does NOT come with home/...
inline char *getColor(Data *data, int theme) {
	return g_ptr_array_index(data->color_icons, theme);
}

inline int getNumberOfColors(Data *data) {
	return (int)data->color_icons->len;
}

inline int getMostUsed(Data *data) {
	return (data->active[data->color_icons->len]);
}

inline int getTableSize(Data *data) {
	return g_hash_table_size(data->main_table);
}

// change to jump table??
void applyAll(Data *data, int theme) {
	GHashTableIter iter;
	char *key = NULL;
	DataObjArray *current = NULL;
	char mode;
	DataObj *tmp;

	g_hash_table_iter_init (&iter, data->main_table);
	while (g_hash_table_iter_next (&iter, (void **)&key, (void **)&current))
	{
		tmp = &(current->arr[2]);
		mode = ((char *)tmp->info)[5];
		switch (mode) {
			case '\0': // apply
				changeTheme(current->arr, theme, 0);
				break;
			case 'v': // var
				changeTheme(current->arr, theme, 1);
				break;
			case 's': // sub
				changeTheme(current->arr, theme, 0);
				applyAll(current->dependency_table, theme);
				break;
		}
	}
}

// applies all options in a given table to a given theme, recursively
// in case of array: applies first option
void allHandler(Data *data, char *info, int offset) {
	int theme = atoi(info + 5), i;
	applyAll(data, theme);

	for (i = 0; info[i] != '/'; i++);
	if (offset == i + 1) {
		generateThemeOptions(data, theme);
	} else {
		info[offset - 1] = '\0';
		// displaySub(data, info, i + 1); this is bad because data is already the dependency table of something
		// either: copy paste display sub but without using dependency table
		// or: trace the entire path back to the menu it is supposed to be in -> bad because original table was lost, would have to parse again
		displaySubWithoutDep(data, info, i + 1);
	}
}
