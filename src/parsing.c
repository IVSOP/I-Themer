#include "parsing.h"
#include "rofi.h"

#define SEP1 putchar('\0')
#define SEP2 putchar('\x1f')

#define TABLE_PATH "I-Themer/data/table.tb"

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

typedef void outputFunc(void*data, FILE *fp);

char *custom_strdup(char *src, int len);
void freeTableStruct(void *data);
void dumpDataObjArray(DataObjArray *, long int depth);
void dumpTableEntry(gpointer key, gpointer value, gpointer user_data);
void executeOnclick(Data * data, DataObjArray *dataobjarray);
void displaySub(Data *data, DataObjArray *dataobjarray, char * command);
void displayVar(Data *data, DataObjArray *dataobjarray);
void executeVar(Data *data, Theme *new_theme, DataObj *themeobj);
void executeSub(Data *data, char *input);
void executeApply(Data *data, DataObjArray *dataobjarray);
void parseThemes(DataObjArray *dataobjarray, Theme *new_theme, Theme *original_theme);
void outInt(void*data, FILE *fp);
void outString(void*data, FILE *fp);
void outVersion(void*data, FILE *fp);
void outEmpty(void*data, FILE *fp);
void outList(void*data, FILE *fp);
void outTable(Data *data, FILE *fp);

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

void generateThemeOptions(Data *data, int selected_theme) {
	DataObj *colorArr = ((DataObjArray *)g_hash_table_lookup(data->main_table, "color-icons"))->arr;
	GHashTableIter iter;
	char *key = NULL;
	DataObjArray *current = NULL;
	DataObj *arr;
	int theme;

	int active[g_hash_table_size(data->main_table) - 1], i;
	int mode;

	g_hash_table_iter_init (&iter, data->main_table);
	for (i = 0; g_hash_table_iter_next (&iter, (void **)&key, (void **)&current); i++) {
		if (strcmp("color-icons", (char *)key) != 0) {
			arr = current->arr;
			mode = ((char *)(&arr[2])->info)[5] / 59;
			printf("%s", key);
			SEP1;
			printf("info");
			SEP2;
			printf("theme%d/%s(%d.0)", selected_theme, key, mode);
			SEP2;
			printf("icon");
			SEP2;
			// no matter subtheme, colors is of the main theme
			if (arr[1].type == INT) {	
				theme = (int)((long int)(arr[1].info));
			} else { // INT_VERSION
				theme = ((Theme *)arr[1].info)->big;
			}
			printf("%s/%s\n", getenv("HOME"), (char *)(colorArr[theme + 1].info));

			active[i] = theme == selected_theme ? 1 : 0;
		} else i--;
	}
	SEP1;
	printf("active");
	SEP2;
	int j = i;
	for (i = 0; i < j; i++) {
		if (active[i] == 1) {
			printf("%d,", i + 1);
		}
	}
	printf("\nBack\n");
	// info is not defined so it will be null and take you back to main menu
}

GHashTable *getTable(Data *data) {
	return data->main_table;
}

void executeChange(Data *data, char * input) {
	DataObjArray *dataobjarray = tableLookup(data, input);
	if (dataobjarray != NULL) { // option got clicked (for first time)
		executeOnclick(data, dataobjarray);
	} else { // option got clicked inside of a display_var or display_sub
		char *endptr, *info = getenv("ROFI_INFO");
		Theme theme;
		theme.big = (int)strtol(info, &endptr, 10);
		if (*endptr != '.') {
			printf("Error in %s, expected x.y-option\n", __func__);
			exit(1);
		}
		theme.small = (int)strtol(endptr + 1, &info, 10);
		if (*info != '-') {
			printf("Error in %s, expected x.y-option\n", __func__);
			exit(1);
		}
		DataObjArray *dataobjarray = (DataObjArray *)g_hash_table_lookup(data->main_table, info + 1);
		DataObj *arr = dataobjarray->arr, *obj = &arr[2], *objtheme = &arr[1];
		Theme original_theme;
		if (objtheme->type == INT) {
			original_theme.big = (int)((long int) objtheme->info);
			original_theme.small = 0;
		} else { // no error checking
			original_theme = *(Theme *)objtheme->info;
		}
		if (original_theme.big == theme.big && original_theme.small == theme.small) {
			printf("Themes are the same\n");
			return;
		} else {
			// can either be show_sub or show_var
			char mode = ((char *)(obj->info))[5]; // no error checking
			if (mode == 'v') {
				executeVar(data, &theme, objtheme);
				displayVar(data, dataobjarray);
			} else { // no error checking
				printf("show sub\n");
			}
		}
	}
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

// needs to perform action depending on what on_click section says
void executeOnclick(Data * data, DataObjArray *dataobjarray) {	
	char * command = (char *)dataobjarray->arr[2].info;
	char cmd = command[5]; // can either be apply, show_sub, or show_var
	switch (cmd)
	{
	case '\0':
		executeApply(data, dataobjarray);
		break;
	case 's':
		// sub needs to know what original command was
		displaySub(data, dataobjarray, (char *)dataobjarray->arr[0].info);
		break;
	case 'v':
		displayVar(data, dataobjarray);
		break;

	default:
		printf("Error, invalid on_click\n");
		exit(1);
	}
}

void parseThemes(DataObjArray *dataobjarray, Theme *new_theme, Theme *original_theme) {
	DataObj *themeobj = &(dataobjarray->arr)[1];

	char *endptr;
	long int res = strtol(getenv("ROFI_INFO"), &endptr, 10);

	new_theme->big = (int)res;
	if (*endptr == '.') {
		new_theme->small = atoi(endptr + 1);
	} else {
		new_theme->small = 0;
	}
	
	if (themeobj->type == INT) {
		original_theme->big = (int)((long int)themeobj->info);
		original_theme->small = 0;
	} else if (themeobj->type == INT_VERSION) {
		*original_theme = *((Theme *)themeobj->info);
	} else {
		printf("Error in '%s'\n", __func__);
	}
}

void executeApply(Data *data, DataObjArray *dataobjarray) {
	Theme original_theme, new_theme;
	parseThemes(dataobjarray, &new_theme, &original_theme);
	DataObj *colorArr = ((DataObjArray *)g_hash_table_lookup(data->main_table, "color-icons"))->arr;

	if (original_theme.big != new_theme.big || original_theme.small != new_theme.small) {
		DataObj *themeObj = &dataobjarray->arr[1];
		if (themeObj->type == INT) {
			Theme *theme = malloc(sizeof(Theme));
			themeObj->type = INT_VERSION;
			themeObj->info = memcpy(theme, &new_theme, sizeof(Theme));
		} else if (themeObj->type == INT_VERSION) {
			themeObj->info = memcpy(themeObj->info, &new_theme, sizeof(Theme));
		} else {
			printf("Error in %s\n", __func__);
			exit(1);
		}
	}
	
	SEP1;
	printf("prompt");
	SEP2;
	printf("Theme %d\nAll", new_theme.big);
	SEP1;
	printf("icon");
	SEP2;
	printf("%s/%s\n", getenv("HOME"), (char *)(&colorArr[new_theme.big + 1])->info);
	generateThemeOptions(data, new_theme.big);
	printf("Back\n");

	saveTableToFile(data);
}

// this is an incomplete mess
void displaySub(Data *data, DataObjArray *dataobjarray, char *command) {
	DataObj *colorArr = ((DataObjArray *)g_hash_table_lookup(data->main_table, "color-icons"))->arr;
	Data *dep = dataobjarray->dependency_table;
	GHashTableIter iter;
	char *key = NULL;
	DataObjArray *current = NULL;
	DataObj *arr;
	Theme original_theme; //, new_theme; NOT NEEDED info is already "x.y"

	char *home = getenv("HOME"), *info = getenv("ROFI_INFO");
	int theme = atoi(info);
	g_hash_table_iter_init (&iter, dep->main_table);
	while (g_hash_table_iter_next (&iter, (void **)&key, (void **)&current))
	{
		arr = current->arr;
		if ((&arr[1])->type == INT) {
			original_theme.big = (int)((long int)((&arr[1])->info));
			original_theme.small = 0;
		} else {
			original_theme = *((Theme *)(&arr[1])->info);
		}
		printf("%s", key);
		SEP1;
		printf("info"); //format: x.y-command, just like in background (change <command> to theme <x.y>)
		SEP2;
		printf("%d.0-%s", theme, command);
		SEP2;
		printf("icon");
		SEP2;
		printf("%s/%s\n", home, (char *)(&colorArr[original_theme.big + 1])->info);
	}

	// missing showing active lines
	printf("Back");
	SEP1;
	printf("info");
	SEP2;
	printf("Theme %d\n", theme);
}

void displayVar(Data *data, DataObjArray *dataobjarray) {
	Theme original_theme, new_theme;
	parseThemes(dataobjarray, &new_theme, &original_theme);
	char *home = getenv("HOME");
	DataObjArray *list = (DataObjArray *)dataobjarray->arr[new_theme.big + 3].info;
	DataObj *arr = list->arr, *current;
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
			printf("%d.%d-%s\n", new_theme.big, i + 1, "background"); // background hardcoded idc
		}
		// I tried using nonselectable, but didnt understand how it worked
		// will leave it to the user to not repeat selections, but program will do them anyway (probably)
		if (original_theme.big == new_theme.big) {
			SEP1;
			printf("active");
			SEP2;
			printf("%d\n", original_theme.small - 1);
		}
	} else {
		printf("show_var for things other that background not complete\n"); exit(5);
	}
	printf("Back");
	SEP1;
	printf("info");
	SEP2;
	printf("Theme %d\n", new_theme.big);
}

void executeVar(Data *data, Theme *new_theme, DataObj *themeobj) {
	if (themeobj->type == INT) {
		Theme *new = malloc(sizeof(Theme));
		themeobj->info = memcpy(new, new_theme, sizeof(Theme));
	} else {
		themeobj->info = memcpy(themeobj->info, new_theme, sizeof(Theme));
	}
	saveTableToFile(data);
}

void executeSub(Data *data, char *input) {
	saveTableToFile(data);
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
}

void outTable(Data *data, FILE *fp) {
	GHashTableIter iter;
	char *key = NULL;
	DataObjArray *current = NULL;

	g_hash_table_iter_init (&iter, data->main_table);
	while (g_hash_table_iter_next (&iter, (void **)&key, (void **)&current))
	{
		outLine(current, fp);
	}
}

void saveTableToFile(Data *data) {
	FILE *fp = fopen(TABLE_PATH, "w");
	int res = fputs("This table was autogenerated as output\n", fp);
	if (res == EOF) {
		printf("Output file error in %s\n", __func__);
		exit(1);
	}
	// printing main table
	outTable(data, fp);

	// print all tables in the dependency
	// this is literally the only purpose of this array, maybe change this??
	GPtrArray *arr = data->dependency_array;
	DataObjArray *dataobjarray;
	char *str;
	int i, len = (int)arr->len;
	for (i = 0; i < len; i++) {
		str = (char *)g_ptr_array_index(arr, i);
		fputc('\n', fp);
		fputs(str, fp);
		fputs(":\n", fp);
		dataobjarray = (DataObjArray *)g_hash_table_lookup(data->main_table, str);
		outTable(dataobjarray->dependency_table, fp);
	}
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

// format received: number-arg1-arg2-...
// 0: lookup <name>-<subname> (subname only if it has sub tables)
// 1: change to (not implemented) <theme>-<name>-<subname>
void queryHandler(Data *data, char *query) {
	printf("NOT WORKING\n"); exit(1);
	char *endptr;
	long int command = strtol(query, &endptr, 10);
	endptr++;
	if (command == 0) {
		char *arg1 = endptr, *arg2;
		for (arg2 = arg1; *arg2 != '\0' && *arg2 != '-'; arg2++);
		if (*arg2 == '\0') { // subname not provided
			arg2 = NULL;
		} else {
			arg2[0] = '\0';
			arg2++;
		}
		DataObjArray *dataobjarray = (DataObjArray *)g_hash_table_lookup(data->main_table, arg1);
		if (dataobjarray == NULL) {
			printf("Name not found (arg1)\n");
			exit(1);
		}
		DataObj *arr = dataobjarray->arr, *themeobj = &arr[1];
		Theme theme;
		if (themeobj->type == INT) {
			theme.big = (int)((long int)themeobj->info);
			theme.small = 0;
		} else { // no error checking
			theme = *(Theme *)themeobj->info;
		}
		char mode = ((char *)(&arr[2])->info)[5]; // will be apply(\0), show_var(v) or show_sub(s)
		// switch???
		if (mode == '\0') {	
			if (theme.small != 0) {
				printf("Internal error, theme is x.y but mode is apply (theme should be x.0)\n");
				exit(1);
			}
			printf("%s\n", (char *)(&arr[theme.big + 3])->info);
		} else if (mode == 'v') {
			DataObj *listobj = &arr[theme.big + 3];
			if (listobj->type != LIST) {
				printf("Internal error, theme is x.y but there is no list on [x]\n");
				exit(1);
			}
			if (theme.small == 0) {
				printf("Internal error, theme not in the form x.y\n");
				exit(1);
			}
			DataObj *list = &(((DataObjArray *)listobj->info)->arr)[theme.small - 1];
			printf("%s\n", (char *)(list->info));
		} else if (mode == 's') {
			DataObjArray *target_dep = (DataObjArray *)g_hash_table_lookup(dataobjarray->dependency_table->main_table, arg2);
			if (target_dep == NULL) {
				printf("Name not found (arg2)\n");
				exit(1);
			}
			// for now it is assumed that it is an "apply" thing,
			// in the future change this funtion into several other functions
			// so that you just have to call it for this particular DataObjArray
			// this is really very bad please change
			
			// theme , themeobj and arr are reused
			arr = target_dep->arr;
			themeobj = &arr[1];
			if (themeobj->type == INT) {
				theme.big = (int)((long int)themeobj->info);
				theme.small = 0;
			} else { // no error checking
				theme = *(Theme *)themeobj->info;
			}
			DataObj *current = &arr[theme.big + 3];
			printf("%s\n", (char *)(current->info));
			
		}
	} else {
		printf("Query not implemented\n");
		exit(1);
	}
}

void changeTheme(DataObj *arr, int big, int small) {
	DataObj *themeobj = &arr[1];
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

// input format: .../<option>(0)
// explanation: applying is allways relative to something that has happened before
// nothing relevant happened: aplly directly
// previous entry was sub: need to apply inside the table of the sub
// previous entry was var: need to apply to it, and not to what is inside the var directly
// NOTE: in case of the sub, to avoid large time looking up sub inside sub inside sub...,
// it is assumed that the Data * that is passed is already the dependency data of the last sub
void applyHandler(Data *data, char *info, int offset) {
	// checks if nothing relevant happened
	int i;
	for (i = 0; info[i] != '/'; i++);
	if (i + 1 == offset) { // apply directly
		for (i = offset; info[i] != '('; i++);
		info[i] = '\0';
		DataObjArray *dataobjarray = (DataObjArray *)g_hash_table_lookup(data->main_table, info + offset);
		// info[i] = '(';
		// info is theme<x>/...
		int theme = atoi(info + 5);
		changeTheme(dataobjarray->arr, theme, 0);
		// back to previous menu
		// info[i] is already \0
		// cant just call input handler, too much of a mess
		printThemeOptions(data, theme);
	} else {
		int previous_mode = (int)info[offset - 3] - 49;
		printf("previous mode: %d %s %s\n", previous_mode, info, info + offset);
	}

}

// input format: .../<option>(2)/..., offset is first char after /
void varHandler(Data *data, char *info, int offset) {
	int i;
	for (i = offset; info[i] != '\0' && info[i] != '/'; i++);
	if (info[i] == '\0') { // ends here, nothing needs to be changed and options need to be displayed
		displayVar2(data, info, offset);
	} else { // does not end here
		// call apply handler??????????????????????????????????????'
		char *endptr;
		long int res = strtol(info + i + 1, &endptr, 10);
		if (endptr != info + i + 1) { // .../<option>(2)/<x> need to apply changes
			int j;
			// same assumption as in displayVar
			for (j = offset; info[j] != '('; j++);
			info[j] = '\0';
			DataObjArray *dataobjarray = (DataObjArray *)g_hash_table_lookup(data->main_table, info + offset);
			info[j] = '(';
			// theme<x>...
			int new_theme = atoi(info + 5);
			Theme *theme = (Theme *)((&dataobjarray->arr[1])->info);
			// printf("changing theme from %d.%d to %d.%d\n", theme->big, theme->small, new_theme, (int)res);
			theme->big = new_theme;
			theme->small = (int)res;
			// go back to before click
			info[i] = '\0';
			displayVar2(data, info, offset);
		} else { // .../<option>(2)/<option>(<m>) need to keep displaying options
			printf("Advanced recursion incomplete (%s)\n", __func__);
			exit(1);
			// int j;
			// for (j = i + 1; info[j] != '('; j++);
			// handlerFunc *handlers[] = {applyHandler, subHandler, varHandler};
			// handlers[(int)(info[j + 1] - 48)](data, info, i + 1);
		}
	}
}

// input format: .../<option>(1)/...
void subHandler(Data *data, char *info, int offset) {
	// need to pass secondary table!!!
	printf("sub %s %s\n", info, info + offset);
}

// input format: .../<option>(2)/...
// output format: <original info>/<option number>
void displayVar2(Data *data, char *str, int offset) {
	int i;
	for (i = offset; str[i] != '('; i++);
	str[i] = '\0';
	DataObjArray *dataobjarray = (DataObjArray *)g_hash_table_lookup(data->main_table, str + offset);
	str[i] = '(';
	int theme = atoi(str + offset - 2);
	
	// NOTE: For now, it is assumed that show_var is used with lists, whose items should be applied
	// no error checking is performed
	Theme *original_theme = (Theme *)((&(dataobjarray->arr[1]))->info);
	DataObjArray *list = (DataObjArray *)((&dataobjarray->arr[theme + 3])->info);
	DataObj *arr = list->arr, *current;
	int len = list->len;

	// I assume that if type is show_var then the theme must be version and not int
	// I also assume all elements in list are strings

	char *home = getenv("HOME");
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
		printf("%s/%d\n", str, i + 1);
	}
	if (theme == original_theme->big) {
		SEP1;
		printf("active");
		SEP2;
		printf("%d\n", original_theme->small - 1);
	}

	str[offset - 1] = '\0';
	printf("Back");
	SEP1;
	printf("info");
	SEP2;
	printf("%s\n", str);
}
