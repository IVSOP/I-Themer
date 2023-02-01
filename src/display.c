#include "display.h"

// it is assumed data is already the dependency data
// maybe use this more often and avoid an extra lookup??????
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
	// no error checking is performed besides empty list
	Theme *original_theme = (Theme *)((&(dataobjarray->arr[1]))->info);
	DataObjArray *list = (DataObjArray *)((&dataobjarray->arr[theme + 3])->info);
	// check for empty can either be type == EMPTY or info == NULL
	if (list == NULL) {
		printf("List is empty");
		SEP1;
		printf("info");
		SEP2;
		str[offset - 1] = '\0';
		printf("%s\n", str); //same as pressing "Back"
		return;
	}
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

	if (i == 0) { // maybe check before so the option "All" doesn't show up?
		str[offset - 1] = '\0';
		printf("No results");
		SEP1;
		printf("info");
		SEP2;
		printf("%s\n", str);
		return;
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
		// if (i == 2) {
			arr = current->arr;
			mode = ((char *)(&arr[2])->info)[5] / 59;
			if (arr[1].type == INT) {	
				theme = (int)((long int)(arr[1].info));
			} else { // INT_VERSION
				theme = ((Theme *)arr[1].info)->big;
			}
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

			printf("%s/%s\n", home, getColor(data, theme));

			active[i] = (theme == selected_theme) ? 1 : 0;
	}

	if (i == 0) {
		// No info, will go straight to main menu
		printf("No options available\n");
		return;
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

	putchar('\n');

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

void printMainOptions(Data *data) {
	int i;
	char * home = getenv("HOME");
	int len = getNumberOfColors(data),
	total = getTableSize(data);
	for (i = 0; i < len; i++) {
		printf("Theme %d --> %d/%d", i, getActivePerTheme(data, i), total);
		SEP1;
		printf("info");
		SEP2;
		printf("theme%d", i);
		SEP2;
		printf("icon");
		SEP2;
		printf("%s/%s\n", home, getColor(data, i));
	}
	SEP1;
	printf("active");
	SEP2;
	printf("%d\n", getMostUsed(data));
}

void printThemeOptions(Data *data, int theme) {
	SEP1;
	printf("prompt");
	SEP2;
	printf("Theme %d\n", theme);

	generateThemeOptions(data, theme);
}
