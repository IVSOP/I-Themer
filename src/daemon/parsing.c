#include "parsing.h"
#include "rofi.h"
#include "display.h"
#include "debug.h"
#include "handlers.h"
#include "queries.h"

#define TABLE_PATH "I-Themer/data"

typedef void outputFunc(void*data, FILE *fp);

void freeTableStruct(void *data);
void outLongInt(void*data, FILE *fp);
void outString(void*data, FILE *fp);
void outVersion(void*data, FILE *fp);
void outEmpty(void*data, FILE *fp);
void outList(List *list, FILE *fp);

// concatenates string 'str' into the output string
inline void outStringBuilder(OUT_STRING *res, char *str) {
	res->len += stpncpy(res->str + res->len, str, STR_RESULT_SIZE - 1) - res->str + res->len;
}

inline void outAddChar(OUT_STRING *res, char chr) {
	res->str[res->len++] = chr;
}

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

void *parseTheme(char *str, int *len) {
	char *endptr;
	long int big = strtol(str, &endptr, 10);
	if (str == endptr) {
		fprintf(stderr, "Theme is empty\n");
		exit(1);
	}
	if (*endptr == '.') {
		char *endptr2;
		int small = strtol(endptr + 1, &endptr2, 10);
		if (*endptr2 != ';') {
			fprintf(stderr, "Invalid theme\n");
			exit(1);
		}
		Theme *theme = malloc(sizeof(Theme));
		theme->big = (int)big;
		theme->small = small;
		(*len) += (endptr2 - str) + 1;
		return theme;
	} else if (*endptr != ';') {
		fprintf(stderr, "Invalid theme\n");
		exit(1);
	} else {
		*len += (endptr - str) + 1;
		return (void *)big;
	}
}

// parses remainder of line, without name, theme and mode
// (can be used to parse a list)
// NULL when empty ??
List *parseParameters(char *str, ssize_t strlen) {
	str[strlen - 1] = ';';
	// wtf is going on with ';;;' in the print below? adding chars in the middle of a string somehow????
	// printf("parsing parameters '%.*s'\noriginal: %s\n", (int)strlen - 1, str, str);
	DataObj arr[DATA_BUFF_SIZE], *tmp;
	char chr;
	int j;

	// if (strlen == 0) {
		// fprintf(stderr, "Empty list, can't parse\n");
		// return NULL;
	// }

	int len, i;
	for (i = len = 0; i < (int)strlen - 1; i++) { // - 1 ????????????????
		// printf("[%d] %d\n", len, i);
		tmp = &arr[len];
		chr = str[i];
		if (chr == '[') {
			for (j = i + 1; str[j] != ']'; j++);
			tmp->type = LIST;
			// printf("parsing list\n");// %.*s\n", j - i - 1, str + i + 1);
			tmp->info = parseParameters(str + i + 1, j - i);
			// printf("parsing list ended\n");
			i = j + 1;
		} else if (chr == ';') {
			tmp->type = EMPTY;
			tmp->info = NULL;
		} else { // string, numbers are also treated as strings since theme has already been parsed
			tmp->type = STRING;
			tmp->info = readString(str + i, &i);
			i--;
		}
		// printf("[%d] has type %d\n", len, tmp->type);
		len++;
	}

	List *list = malloc(sizeof(List));
	list->len = len;
	list->arr = malloc(sizeof(DataObj) * len);
	list->arr = memcpy(list->arr, arr, len * sizeof(DataObj));
	return list;
}

Mode parseMode(char *str, int *len) {
	// improve this???
	if (strncmp(str, "apply", 5) == 0) {
		(*len) += 6;
		return APPLY;
	} else if (strncmp(str, "show_var", 8) == 0) {
		(*len) += 9;
		return VAR;
	} else if (strncmp(str, "show_sub", 8) == 0) {
		(*len) += 9;
		return SUB;
	} else {
		fprintf(stderr, "Invalid mode\n");
		exit(1);
	}
}

// takes in string for (what is thinks) is the entire line and turns it into DataObjArray *, mallocing as needed
// the string includes the \n in [strlen - 1]
DataObjArray *parseLineString(char *str, ssize_t strlen) {
	int i = 0;

	// printf("parsing line %s\n", str);

	DataObjArray *final = malloc(sizeof(DataObjArray));
	final->name = readString(str, &i);
	// printf("name: %s\n", final->name);

	final->theme = parseTheme(str + i, &i);

	final->mode = parseMode(str +i, &i);
	// printf("mode: %d\n", final->mode);
	if (final->mode != SUB) {
		final->list = parseParameters(str + i, strlen - i);
	} // else list is undefined

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

inline int getThemeBig(DataObj *themeobj) {
	return (themeobj->type == INT ? (int)((long int)themeobj->info) : *(int *)themeobj->info);
}

// returns NULL on EOF
Data *parseMainTable(FILE *fp, GPtrArray *colorArr, char *dir) {
	Data *data = malloc(sizeof(Data));
	int *active = calloc((colorArr->len + 1), sizeof(int));
	data->active = active;

	// key destroy func is NULL since they will be freed when the remaining data is freed (they are shared)
	GHashTable * table = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, freeTableStruct);
	DataObjArray *lineData = parseLine(fp);
	int current_theme, biggest;
	while (lineData != NULL) {
		if (lineData->mode == VAR) {
			current_theme = ((Theme *)lineData->theme)->big;
			active[current_theme] += 1;
		} else {
			current_theme = (long int)lineData->theme;
			if (lineData->mode == SUB) { // mode is sub, needs its dependencies resolved
				current_theme = (long int)lineData->theme;
				char str[BUFFER_SIZE];
				snprintf(str, BUFFER_SIZE, "%s/%s.tb", dir, lineData->name);
				FILE *fp2 = fopen(str, "r");
				CHECK_FILE_ERROR(fp2)
				lineData->dependency_table = parseMainTable(fp2, colorArr, dir);
				fclose(fp2);
				// need to update current selected theme, based on the most used theme
				// is sub, so can be sure it is an INT and not INT_VERSION
				if ((biggest = lineData->dependency_table->active[(const int)colorArr->len]) == current_theme) {
					// if the most used is the same as current, no need to update it
					active[current_theme] += 1;
				} else {
					// else have to change it and active[other theme] += 1
					lineData->theme = (void *)((long int)biggest);
					active[biggest] += 1;
				}
				
			} else {
				active[current_theme] += 1;
			}
		}
			
			// printf("adding theme %d to %s\n", getThemeBig(&lineData->arr[1]), ((char *)(&lineData->arr[0])->info));

		// if (tmp->type != STRING) {
		// 	fprintf(stderr, "For now the hash table is designed to use strings in the hashing function, please start all lines with a string\nError thrown in %s\n", __func__);
		// 	exit(1);
		// }

		// WHY IS THIS INSIDE THE WHILE LOOP?????????????????????????
		
		// 0 will be checked anyway, just start at 1
		for (current_theme = 1, biggest = 0; current_theme < (const int)colorArr->len; current_theme++) {
			if (active[current_theme] > active[biggest]) biggest = current_theme;
		}
		active[current_theme] = biggest;

		g_hash_table_insert(table, lineData->name, lineData);
		lineData = parseLine(fp);
	}

	data->main_table = table;
	data->color_icons = colorArr;
	return data;
}

void freeNULL (void *data) {
	return;
}

// will free list itself and the array
void freeList(List *list) {
	int i, len = list->len;
	DataObj *arr = list->arr,
	*tmp;
	for (i = 0; i < len; i++) {
		tmp = &arr[i];
		if (tmp->type == STRING) { // is either string or list or empty
			free(tmp->info);
		} else if (tmp->type == LIST) {
			freeList((List *)tmp->info);
		} // else do nothing, it is empty
	}
	free(arr);
	free(list);
}

// frees DataObjArray, NOT Data
void freeTableStruct(void *data) {
	// if (data == NULL) return;
	DataObjArray *dataArr = (DataObjArray *)data;
	free(dataArr->name);

	if (dataArr->mode != SUB) {
		if (dataArr->mode == VAR) free(dataArr->theme);
		freeList(dataArr->list);
	}

	if (dataArr->dependency_table != NULL) freeTableData((Data *)dataArr->dependency_table);
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

inline DataObjArray *tableLookup(Data *data, char *str) {
	return g_hash_table_lookup(data->main_table, str);
}

inline int getLen(DataObjArray *data) {
	return data->list->len;
}

inline DataObj *getDataObj(DataObjArray *data, int i) {
	if (i >= data->list->len) return NULL;
	return &(data->list->arr)[i];
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



// outputs List *
void outList(List *list, FILE *fp) {
	DataObj *arr = list->arr, *tmp;
	int i, len = list->len;
	TYPE type;
	// fputc('[', fp);
	for (i = 0; i < len - 1; i++) {
		tmp = &arr[i];
		type = tmp->type;
		if (type == STRING) {
			fputs((char *)tmp->info, fp);
		// } else if (type == EMPTY) {
		// 	does nothing
		} else if (type == LIST) {
			fputc('[', fp);
			outList((List *)tmp->info, fp);
			fputc(']', fp);
		} // else not possible
		fputc(';', fp);
	}
	tmp = &arr[i];
	type = tmp->type;
	if (type == STRING) {
		fputs((char *)tmp->info, fp);
	// } else if (type == EMPTY) {
	// 	does nothing
	} else if (type == LIST) {
		fputc('[', fp);
		outList((List *)tmp->info, fp);
		fputc(']', fp);
	} // else not possible
}

// outputs entire DataObjArray *
// kind of a mess but works
void outLine(DataObjArray *dataobjarray, FILE *fp, char *dir) {
	outString(dataobjarray->name, fp);
	putc(';', fp);
	if (dataobjarray->mode == VAR) {
		outVersion(dataobjarray->theme, fp);
		putc(';', fp);
		outString("show_var", fp);
		putc(';', fp);
		outList(dataobjarray->list, fp);
	} else if (dataobjarray->mode == APPLY) {
		outLongInt(dataobjarray->theme, fp);
		putc(';', fp);
		outString("apply", fp);
		putc(';', fp);
		outList(dataobjarray->list, fp);
	} else {
		outLongInt(dataobjarray->theme, fp);
		putc(';', fp);
		outString("show_sub", fp);
		// not putc(';', fp), it should be empty after this
		// sub also has no list (it is not necessarily null, shoudn't check it)
	}

	fputc('\n', fp);
	if (dataobjarray->dependency_table != NULL) {
		saveTableToFile(dataobjarray->dependency_table, dataobjarray->name, dir);
	}
}

void saveTableToFile(Data *data, char *name, char *dir) {
	char str[BUFFER_SIZE];
	snprintf(str, BUFFER_SIZE, "%s/%s.tb", dir, name);
	FILE *fp = fopen(str, "w");
	CHECK_FILE_ERROR(fp);

	GHashTableIter iter;
	char *key = NULL;
	DataObjArray *current = NULL;


	g_hash_table_iter_init (&iter, data->main_table);
	while (g_hash_table_iter_next (&iter, (void **)&key, (void **)&current))
	{
		outLine(current, fp, dir);
	}

	fclose(fp);
}

void outLongInt(void *data, FILE *fp) {
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

GPtrArray *parseColors(char *name, char *dir) {
	GPtrArray *arr = g_ptr_array_new_full(3, free);
	char str[BUFFER_SIZE];
	snprintf(str, BUFFER_SIZE, "%s/%s", dir, "color-icons");
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
