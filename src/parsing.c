#include "parsing.h"

#define SEP1 putchar('\0')
#define SEP2 putchar('\x1f')

struct DataObj {
	void *info;
	TYPE type;
};

struct Data {
	GHashTable *main_table;
	GPtrArray *dependency_array;
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
void executeOnclick(Data * data, DataObjArray *dataobjarray, Theme *new_theme, Theme *original_theme);
void saveTableToFile(Data *data);

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
	// whis is probably bad for sub-tables but idc
	data->dependency_array = NULL;
	
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

void parseDependecyTables(Data *data, FILE *fp) {
	char *str = alloca(sizeof(char) * BUFFER_SIZE); // compiler didnt like the fact that it was array and not pointer
	DataObjArray * lookup_res;
	// this is done here so that subtables dont make one
	// 10 was pulled out of my ass
	data->dependency_array = g_ptr_array_new_full(10, NULL);
	while ((str = getDependencyString(str, fp)) != NULL) {
		lookup_res = g_hash_table_lookup(((const Data *) data)->main_table, str);
		if (lookup_res == NULL) {
			fprintf(stderr, "Sub-table found but no entry in main table corresponds to it. Found:'%s'\n", str);
			exit(1);
		} else {
			lookup_res->dependency_table = parseMainTable(fp);
			// add to the array that lists all the dependencies
			g_ptr_array_add(((const Data *) data)->dependency_array, (char *)(lookup_res->arr[0].info));
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
	if (data != NULL) {
		g_hash_table_destroy(data->main_table);
		if (data->dependency_array != NULL) g_ptr_array_free(data->dependency_array, TRUE);
		free(data);
	}
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
// from here needs to go to a function that queries for filename!!!!!!!
void executeChange(Data *data, char * input) {

	// printf("size: %d\n", (data->dependency_array)->len);
	// printf("'%s'\n", (char *)g_ptr_array_index(data->dependency_array, 0));
	// printf("'%s'\n", (char *)g_ptr_array_index(data->dependency_array, 1));

	DataObjArray *dataobjarray = tableLookup(data, input);
	if (dataobjarray == NULL) {
		printf("Received %s, not found in main table\nInfo is%s\n", input, getenv("ROFI_INFO"));
		exit(1);
	}
	DataObj *themeobj = &(dataobjarray->arr)[1];

	char *endptr;
	long int res = strtol(getenv("ROFI_INFO"), &endptr, 10);

	Theme new_theme = {(int)res, 0};
	if (*endptr == '.') {
		new_theme.small = atoi(endptr + 1);
	}

	Theme original_theme;
	
	if (themeobj->type == INT) {
		original_theme.big = (int)((long int)themeobj->info);
		original_theme.small = 0;
	} else if (themeobj->type == INT_VERSION) {
		original_theme = *((Theme *)themeobj->info);
	} else {
		printf("Error in '%s' (%s)\n", __func__, input);
	}
	executeOnclick(data, dataobjarray, &new_theme, &original_theme);
}

void freeDataObj(DataObj *data) {
	freeFunc *freeDispatchTable[] = {freeNULL, free, free, freeNULL, freeTableStruct};
	freeDispatchTable[data->type](data->info);
}

// switch??? jump table???
// WILL NOT put \n in the end
void printDataObj(DataObj *data) {
	TYPE type = data->type;
	if (type == INT) {
		printf("%ld", (long int)data->info);
	} else if (type == STRING) {
		printf("%s", (char *)data->info);
	} else if (type == EMPTY) {
		printf("Empty");
	} else if (type == LIST) {
		printf("Printing lists not implemented\n");
		exit(1);
	} else {
		Theme *theme = (Theme *)data->info;
		printf("%d.%d", theme->big, theme->small);
	}
}

// input to get here:
// <commmand> (theme is passed through info parsed in other function)
void executeOnclick(Data * data, DataObjArray *dataobjarray, Theme *new_theme, Theme *original_theme) {
	// needs to perform action depending on what on_click section says, and if there are things to be changed then output entire table
	char * command = (char *)dataobjarray->arr[2].info;
	// 3 possibilities: show_var, show_sub, apply
	// this is slightly faster than strcmp
	DataObj *themeObj = &dataobjarray->arr[1];
	DataObj *current;
	char cmd = command[5];
	char *home = getenv("HOME");
	if (cmd == '\0') {
		// even if themes are the same you can just apply them
		freeDataObj(themeObj);
		if (new_theme->small == 0) {
			themeObj->type = INT;
			themeObj->info = (void *)((long int)new_theme->big);
		} else {
			themeObj->type = INT_VERSION;
			Theme *new = malloc(sizeof(Theme));
			themeObj->info = memcpy(new, new_theme, sizeof(Theme));
		}
		saveTableToFile(data);
	} else if (cmd == 'v') {
		DataObjArray *list = (DataObjArray *)dataobjarray->arr[new_theme->big + 3].info;
		DataObj *arr = list->arr;
		int i, len = list->len;
		
		// i dont like this being hardcoded, but it was the simplest way
		// background icons are not the color theme but the picture itself
		if (strncmp("background", (char *)(dataobjarray->arr[0].info), 10) == 0) {
			// 1 if true, 0 if false
			for (i = 0; i < len; i++) {
				current = &arr[i];
				printDataObj(current);
				SEP1;
				printf("icon");
				SEP2;
				printf("%s/%s", home, (char *)current->info);
				SEP2;
				printf("info"); // format: x.y-background
				SEP2;
				printf("%d.%d-%s\n", new_theme->big, i + 1, "background"); // background hardcoded idc
			}
			// I tried using nonselectable, but didnt understand how it worked
			// will leave it to the user to not repeat selections, but program will do them anyway probably
			if (original_theme->big == new_theme->big) {
				SEP1;
				printf("active");
				SEP2;
				printf("%d\n", original_theme->small - 1);
			}
		} else {
			printf("show_var for things other that background not complete\n"); exit(5);
		}
	} else {
		DataObj *colorArr = ((DataObjArray *)g_hash_table_lookup(data->main_table, "color-icons"))->arr;
		Data *dep = dataobjarray->dependency_table;
		GHashTableIter iter;
		char *key = NULL;
		DataObjArray *current = NULL;
		DataObj *arr;
		Theme theme;

		g_hash_table_iter_init (&iter, dep->main_table);
		while (g_hash_table_iter_next (&iter, (void **)&key, (void **)&current))
		{
			arr = current->arr;
			if ((&arr[1])->type == INT) {
				theme.big = (int)((long int)((&arr[1])->info));
				theme.small = 0;
			} else {
				theme = *((Theme *)(&arr[1])->info);
			}
			printf("%s", key);
			SEP1;
			printf("info"); //format: x.y-command, just like in background
			SEP2;
			printf("%d.%d-%s", theme.big, theme.small, command);
			SEP2;
			printf("icon");
			SEP2;
			printf("%s/%s\n", home, (char *)(&colorArr[theme.big + 1])->info);
		}
		// missing showing active lines
	}
	printf("Back");
	SEP1;
	printf("info");
	SEP2;
	printf("Theme %d\n", original_theme->big);
}

void saveTableToFile(Data *data) {
	return;
	// FILE *fp = fopen("data/table_save.tb", "w");
	// int res = fputs("This table was autogenerated as output\n", fp);
}
