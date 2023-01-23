#include "parsing.h"
#include "rofi.h"

#define SEP1 putchar('\0')
#define SEP2 putchar('\x1f')

#define TABLE_PATH "I-Themer/data"

struct DataObj {
	void *info;
	TYPE type;
};

struct Data {
	GHashTable *main_table;
	GPtrArray *color_icons;
	int *active;
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

void freeTableStruct(void *data);
void dumpDataObjArray(DataObjArray *, long int depth);
void dumpTableEntry(gpointer key, gpointer value, gpointer user_data);
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
	printSpace(depth);
	printf("active = [");
	int i;
	for (i = 0; i < (const int)data->color_icons->len; i++) {
		printf("%d, ", data->active[i]);
	}
	printf("%d]\n", data->active[i]);
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

inline int getActivePerTheme(Data *data, int theme) {
	return (data->active)[theme];
}

void *getValue(DataObj *data) {
	return data->info;
}

void generateThemeOptions(Data *data, int selected_theme) {
	GHashTableIter iter;
	char *key = NULL;
	DataObjArray *current = NULL;
	DataObj *arr;
	int theme;

	int active[g_hash_table_size(data->main_table) - 1], i;
	int mode;
	char *home = getenv("HOME");

	printf("All");
	SEP1;
	printf("info");
	SEP2;
	printf("theme%d/All(3)", selected_theme);
	SEP2;
	printf("icon");
	SEP2;
	printf("%s/%s\n", home, getColor(data, selected_theme));

	g_hash_table_iter_init (&iter, data->main_table);
	for (i = 0; g_hash_table_iter_next (&iter, (void **)&key, (void **)&current); i++) {
		arr = current->arr;
		mode = ((char *)(&arr[2])->info)[5] / 59;
		printf("%s", key);
		if (mode == 1) { // sub
			printf(" --> %d/%d", current->dependency_table->active[selected_theme], getTableSize(current->dependency_table));
		}
		SEP1;
		printf("info");
		SEP2;
		printf("theme%d/%s(%d)", selected_theme, key, mode);
		SEP2;
		printf("icon");
		SEP2;
		// no matter subtheme, colors is of the main theme
		if (arr[1].type == INT) {	
			theme = (int)((long int)(arr[1].info));
		} else { // INT_VERSION
			theme = ((Theme *)arr[1].info)->big;
		}
		printf("%s/%s\n", home, getColor(data, theme));

		active[i] = theme == selected_theme ? 1 : 0;
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

// receives info without "query0/" from the start
void query0(Data *data, char *info) {
	int i;
	for (i = 0; info[i] != '\0' && info[i] != '/'; i++);
	char tmp = info[i];
	tmp = info[i];
	info[i] = '\0';
	DataObjArray *dataobjarray = (DataObjArray *)g_hash_table_lookup(data->main_table, info);
	if (tmp == '\0') { // print data
		// can only print strings and lists (of strings)

		// case 0: is a string, and type is int -> print string
		// case 1: is a string, and type is not int -> NOT POSSIBLE
		// case 2: is a list, and type is int -> print entire list
		// case 3: is a list, and type is not int -> print an element of the list
		DataObj *arr = dataobjarray->arr,
		*themeobj = &arr[1];
		if (themeobj->type == INT_VERSION) {
			// assumed to be list, and print a single element
			Theme *theme = (Theme *)themeobj->info;
			DataObjArray *list = (&arr[theme->big + 3])->info;
			printf("%s\n", (char *)((&(list->arr[theme->small - 1]))->info));
			// assumed to be list, and have to print all its elements
		} else {
			DataObj *current = &arr[(long int)themeobj->info + 3];
			if (current->type == LIST) { // assumed list of strings
				DataObjArray *list = (DataObjArray *)current->info;
				for (i = 0; i < (const int)list->len; i++) {
					current = &(list->arr[i]);
					printf("%s ", (char *)current->info);
				}
			} else { // assumed to be string
				printf("%s\n", (char *)current->info);
			}
		}

	} else { // have to go into subadata
		query0(dataobjarray->dependency_table, info + i + 1);
	}
}

// format received: query<number>/<arg1>/<arg2>/...
// 0: lookup <name>/<subname> (subname only if it has sub tables)
// 1: change to (not implemented) <theme>/<name>/<subname>
void queryHandler(Data *data, char *info) {
	char *endptr;
	int query = (int)strtol(info + 5, &endptr, 10);
	if (query != 0) {
		printf("Only query 0 has been completed\n");
		exit(1);
	}
	// no error checking, responsibility of user?
	query0(data, endptr + 1); // skip numbers and '/'
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
// applies and goes back to previous menu
// applying is slightly different in case of lists, it is done by varHandler
// in case of sub, the data being passed is already the subtable
void applyHandler(Data *data, char *info, int offset) {
	int i;
	for (i = offset; info[i] != '('; i++);
	info[i] = '\0';
	DataObjArray *dataobjarray = (DataObjArray *)g_hash_table_lookup(data->main_table, info + offset);
	// info[i] = '(';
	// info is theme<x>/...
	int theme = atoi(info + 5);
	changeTheme(dataobjarray->arr, theme, 0);

	// back to previous menu
	int j;
	// checks if nothing relevant happened
	for (j = 0; info[j] != '/'; j++);
	info[offset - 1] = '\0';
	// can either be var or sub, never apply
	// or it can be theme
	if (j + 1 == offset) { // previous menu is just the menu of a theme
		printThemeOptions(data, theme);
	} else {
		for (i = offset - 2; info[i] != '/'; i--);
		if (info[offset - 3] == '1') { // sub
			// crashing because it needs to go back to a data * that no longer exists. need to apply in sub.
			// subHandler(data, info, i + 1);
			// for now, this will do nothing, since the apply of the new theme is correct but the display of previous data isn't
		} else { // var
			varHandler(data, info, i + 1);
		}
	}
}

// input format: .../<option>(2)/..., offset is first char after /
void varHandler(Data *data, char *info, int offset) {
	int i;
	for (i = offset; info[i] != '\0' && info[i] != '/'; i++);
	if (info[i] == '\0') { // ends here, nothing needs to be changed and options need to be displayed
		displayVar(data, info, offset);
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
			displayVar(data, info, offset);
		} else { // .../<option>(2)/<option>(<m>) need to keep displaying options
			printf("Advanced recursion incomplete (%s)\n", __func__);
			exit(1);
		}
	}
}

// input format: .../<option>(1)/...
void subHandler(Data *data, char *info, int offset) {
	int i;
	for (i = offset; info[i] != '\0' && info[i] != '/'; i++);
	if (info[i] == '\0') { // ends here, nothing needs to be changed and options need to be displayed
		displaySub(data, info, offset);
	} else { // .../<option>(1)/<option>(<m>) need to call apropriate function just like inputHandler would, but cant call it
		int j;
		for (j = offset; info[j] != '('; j++);
		info[j] = '\0';
		info[i] = '\0';
		DataObjArray *dataobjarray = (DataObjArray *)g_hash_table_lookup(data->main_table, info + offset);
		info[j] = '(';
		info[i] = '/';
		// inputHandler(dataobjarray->dependency_table, info + i + 1);
		for (j = i + 1; info[j] != '('; j++);
		// see error in apply handler for explanation as to why this doesnt work
		// handlerFunc *handlers[] = {applyHandler, subHandler, varHandler};
		// handlers[(int)(info[j + 1] - 48)](dataobjarray->dependency_table, info, i + 1);
		switch ((int)(info[j + 1] - 48))
		{
		case 0: // apply
			applyHandler(dataobjarray->dependency_table, info, i + 1); // I assume some magic happens here and a \0 is perfectly placed to allow to pass info + i + 1 next
			// magic is correct but offset is not
			// ineficient but idc
			for (i -= 1; info[i] != '/'; i--);
			displaySub(data, info, i + 1);
			break;
		case 1: // sub
			subHandler(dataobjarray->dependency_table, info, i + 1);
			break;
		case 2: // var
			varHandler(dataobjarray->dependency_table, info, i + 1);
			break;
		case 3:
			allHandler(dataobjarray->dependency_table, info, i + 1);
			break;
		}
	}
}

// input format: .../<option>(2)/...
// output format: <original info>/<option number>
void displayVar(Data *data, char *str, int offset) {
	int i;
	for (i = offset; str[i] != '('; i++);
	str[i] = '\0';
	DataObjArray *dataobjarray = (DataObjArray *)g_hash_table_lookup(data->main_table, str + offset);
	str[i] = '(';
	int theme = atoi(str + 5);
	
	// NOTE: For now, it is assumed that show_var is used with lists, whose items should be applied
	// no error checking is performed
	Theme *original_theme = (Theme *)((&(dataobjarray->arr[1]))->info);
	DataObjArray *list = (DataObjArray *)((&dataobjarray->arr[theme + 3])->info);
	DataObj *arr = list->arr, *current;
	int len = list->len;

	// I assume that if type is show_var then the theme must be version and not int
	// I also assume all elements in list are strings
	char *home = getenv("HOME");

	// kind of a bad solution, but background images are show as what the info itself says
	if (strncmp((char *)(&dataobjarray->arr[0])->info, "background", 10) == 0) {
		for (i = 0; i < len; i++) {
			current = &arr[i];
			printDataObj(current);
			SEP1;
			printf("icon");
			SEP2;
			printf("%s/%s", home, (char *)current->info);
			SEP2;
			printf("info");
			SEP2;
			printf("%s/%d\n", str, i + 1);
		}
	} else { // UNTESTED
		for (i = 0; i < len; i++) {
			current = &arr[i];
			printDataObj(current);
			SEP1;
			printf("icon");
			SEP2;
			printf("%s/%s", home, getColor(data, original_theme->big));
			SEP2;
			printf("info");
			SEP2;
			printf("%s/%d\n", str, i + 1);
		}
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

void displaySub(Data *data, char *str, int offset) {
	int i;
	for (i = offset; str[i] != '('; i++);
	str[i] = '\0';
	DataObjArray *dataobjarray = (DataObjArray *)g_hash_table_lookup(data->main_table, str + offset);
	str[i] = '(';
	// int theme = atoi(str + 5);

	// need to show all options of the subtable
	Data *dep = dataobjarray->dependency_table;
	GHashTableIter iter;
	char *key = NULL;
	DataObjArray *current = NULL;
	DataObj *arr;

	// output format: .../<option>(1)/<option>(<m>)
	int theme, active[g_hash_table_size(dep->main_table)], original_theme = atoi(str + 5);
	char mode, *home = getenv("HOME"), *infostr;
	g_hash_table_iter_init (&iter, dep->main_table);

	printf("All");
	SEP1;
	printf("info");
	SEP2;
	printf("%s/All(3)", str);
	SEP2;
	printf("icon");
	SEP2;
	printf("%s/%s\n", home, getColor(data, original_theme));

	for (i = 0; g_hash_table_iter_next (&iter, (void **)&key, (void **)&current); i++)
	{
		arr = current->arr;
		mode = ((char *)((&arr[2])->info))[5];
		infostr = (char *)((&arr[0])->info);
		// assume it can only be int or int_version
		theme =	(&arr[1])->type == INT ? (int)((long int)((&arr[1])->info)) : ((Theme *)((&arr[1])->info))->big;
		active[i] = theme == original_theme ? 1 : 0;
		printf("%s", infostr);
		SEP1;
		printf("info");
		SEP2;
		printf("%s/%s(%d)", str, infostr, mode);
		SEP2;
		printf("icon");
		SEP2;
		printf("%s/%s\n", home, getColor(data, theme));
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
	
	str[offset - 1] = '\0';
	printf("\nBack");
	SEP1;
	printf("info");
	SEP2;
	printf("%s\n", str);
}

GPtrArray *parseColors(char *name) {
	GPtrArray *arr = g_ptr_array_new_full(3, free);
	char str[BUFFER_SIZE];
	snprintf(str, BUFFER_SIZE, "%s/%s", TABLE_PATH, name); // not .tb??
	FILE *fp = fopen(str, "r");
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

// it is assumed data is already the dependency data
// maybe use this more oftern and avoid an extra lookup??????
void displaySubWithoutDep(Data *data, char *str, int offset) {
	GHashTableIter iter;
	char *key = NULL;
	DataObjArray *current = NULL;
	DataObj *arr;

	// output format: .../<option>(1)/<option>(<m>)
	int theme, active[g_hash_table_size(data->main_table)], original_theme = atoi(str + 5), i;
	char mode, *home = getenv("HOME"), *infostr;
	g_hash_table_iter_init (&iter, data->main_table);

	printf("All");
	SEP1;
	printf("info");
	SEP2;
	printf("%s/All(3)", str);
	SEP2;
	printf("icon");
	SEP2;
	printf("%s/%s\n", home, getColor(data, original_theme));

	for (i = 0; g_hash_table_iter_next (&iter, (void **)&key, (void **)&current); i++)
	{
		arr = current->arr;
		mode = ((char *)((&arr[2])->info))[5];
		infostr = (char *)((&arr[0])->info);
		// assume it can only be int or int_version
		theme =	(&arr[1])->type == INT ? (int)((long int)((&arr[1])->info)) : ((Theme *)((&arr[1])->info))->big;
		active[i] = theme == original_theme ? 1 : 0;
		printf("%s", infostr);
		SEP1;
		printf("info");
		SEP2;
		printf("%s/%s(%d)", str, infostr, mode);
		SEP2;
		printf("icon");
		SEP2;
		printf("%s/%s\n", home, getColor(data, theme));
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
	
	str[offset - 1] = '\0';
	printf("\nBack");
	SEP1;
	printf("info");
	SEP2;
	printf("%s\n", str);
}
