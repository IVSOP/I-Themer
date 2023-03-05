#include "display.h"

// it is assumed data is already the dependency data
// maybe use this more often and avoid an extra lookup??????
void displaySubWithoutDep(Data *data, char *str, int offset, OUT_STRING *res) {
	GHashTableIter iter;
	char *key = NULL;
	DataObjArray *current = NULL;

	// output format: .../<option>(1)/<option>(<m>)
	int theme, active[g_hash_table_size(data->main_table)], original_theme = atoi(str + 5), i;
	char mode, *home = getenv("HOME"), *infostr;
	g_hash_table_iter_init (&iter, data->main_table);

	outStringBuilder(res, "All");
	outAddChar(res, SEP1);
	outStringBuilder(res, "info");
	outAddChar(res, SEP2);
	res->len += sprintf(res->str + res->len, "%s/All(3)", str);
	outAddChar(res, SEP2);
	outStringBuilder(res, "icon");
	outAddChar(res, SEP2);
	res->len += sprintf(res->str + res->len, "%s/%s\n", home, getColor(data, original_theme));

	for (i = 0; g_hash_table_iter_next (&iter, (void **)&key, (void **)&current); i++)
	{
		mode = current->mode;
		infostr = current->name;
		// assume it can only be int or int_version
		// check mode instead of type???
		theme =	(mode == VAR) ? (((Theme *)(current->theme))->big) : (int)((long int)(current->theme));
		active[i] = theme == original_theme ? 1 : 0;
		res->len += sprintf(res->str + res->len, "%s", infostr);
		outAddChar(res, SEP1);
		outStringBuilder(res, "info");
		outAddChar(res, SEP2);
		res->len += sprintf(res->str + res->len, "%s/%s(%d)", str, infostr, mode);
		outAddChar(res, SEP2);
		outStringBuilder(res, "icon");
		outAddChar(res, SEP2);
		res->len += sprintf(res->str + res->len, "%s/%s\n", home, getColor(data, theme));
	}

	outAddChar(res, SEP2);
	outStringBuilder(res, "active");
	outAddChar(res, SEP1);
	int j = i;
	for (i = 0; i < j; i++) {
		if (active[i] == 1) {
			res->len += sprintf(res->str + res->len, "%d,", i + 1);
		}
	}
	
	str[offset - 1] = '\0';
	outStringBuilder(res, "\nBack");
	outAddChar(res, SEP1);
	outStringBuilder(res, "info");
	outAddChar(res, SEP2);
	res->len += sprintf(res->str + res->len, "%s\n", str);
}

// input format: .../<option>(2)/...
// output format: <original info>/<option number>
// displays list contained in list->arr[theme]
void displayVar(Data *data, char *str, int offset, OUT_STRING *res) {
	int i;
	for (i = offset; str[i] != '('; i++);
	str[i] = '\0';
	DataObjArray *dataobjarray = (DataObjArray *)g_hash_table_lookup(data->main_table, str + offset);
	str[i] = '(';
	int theme = atoi(str + 5);
	
	// NOTE: For now, it is assumed that show_var is used with lists, whose items should be applied
	// no error checking is performed besides empty list
	// assumed it is of type Theme *
	Theme *original_theme = (Theme *)(dataobjarray->theme);
	DataObj *themeObj = &(dataobjarray->list->arr[theme]);
	if (themeObj->type == EMPTY) {
		outStringBuilder(res, "List is empty");
		outAddChar(res, SEP1);
		outStringBuilder(res, "info");
		outAddChar(res, SEP2);
		str[offset - 1] = '\0';
		res->len += sprintf(res->str + res->len, "%s\n", str); //same as pressing "Back"
		return;
	}
	//else

	List *list = (List *)(themeObj->info);
	// check for empty can either be type == EMPTY or info == NULL

	DataObj *arr = list->arr, *current;
	int len = list->len;

	// I assume that if type is show_var then the theme must be version and not int
	// I also assume all elements in list are strings
	char *home = getenv("HOME");

	// kind of a bad solution, but background images are show as what the info itself says
	if (strncmp(dataobjarray->name, "background", 10) == 0) {
		for (i = 0; i < len; i++) {
			current = &arr[i];
			printDataObj(current, res);
			outAddChar(res, SEP1);
			outStringBuilder(res, "icon");
			outAddChar(res, SEP2);
			res->len += sprintf(res->str + res->len, "%s/%s", home, (char *)current->info);
			outAddChar(res, SEP2);
			outStringBuilder(res, "info");
			outAddChar(res, SEP2);
			res->len += sprintf(res->str + res->len, "%s/%d\n", str, i + 1);
		}
	} else { // UNTESTED
		for (i = 0; i < len; i++) {
			current = &arr[i];
			printDataObj(current, res);
			outAddChar(res, SEP1);
			outStringBuilder(res, "icon");
			outAddChar(res, SEP2);
			res->len += sprintf(res->str + res->len, "%s/%s", home, getColor(data, original_theme->big));
			outAddChar(res, SEP2);
			outStringBuilder(res, "info");
			outAddChar(res, SEP2);
			res->len += sprintf(res->str + res->len, "%s/%d\n", str, i + 1);
		}
	}
	if (theme == original_theme->big) {
		outAddChar(res, SEP1);
		outStringBuilder(res, "active");
		outAddChar(res, SEP2);
		res->len += sprintf(res->str + res->len, "%d\n", original_theme->small - 1);
	}

	str[offset - 1] = '\0';
	outStringBuilder(res, "Back");
	outAddChar(res, SEP1);
	outStringBuilder(res, "info");
	outAddChar(res, SEP2);
	res->len += sprintf(res->str + res->len, "%s\n", str);
}

void displaySub(Data *data, char *str, int offset, OUT_STRING *res) {
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

	// output format: .../<option>(1)/<option>(<m>)
	int theme, active[g_hash_table_size(dep->main_table)], original_theme = atoi(str + 5);
	char mode, *home = getenv("HOME"), *infostr;
	g_hash_table_iter_init (&iter, dep->main_table);

	outStringBuilder(res, "All");
	outAddChar(res, SEP1);
	outStringBuilder(res, "info");
	outAddChar(res, SEP2);
	res->len += sprintf(res->str + res->len, "%s/All(3)", str);
	outAddChar(res, SEP2);
	outStringBuilder(res, "icon");
	outAddChar(res, SEP2);
	res->len += sprintf(res->str + res->len, "%s/%s\n", home, getColor(data, original_theme));

	for (i = 0; g_hash_table_iter_next (&iter, (void **)&key, (void **)&current); i++)
	{
		mode = current->mode;
		infostr = current->name;
		theme =	(mode == VAR) ? (((Theme *)(current->theme))->big) : (int)((long int)(current->theme));
		active[i] = theme == original_theme ? 1 : 0;
		res->len += sprintf(res->str + res->len, "%s", infostr);
		outAddChar(res, SEP1);
		outStringBuilder(res, "info");
		outAddChar(res, SEP2);
		res->len += sprintf(res->str + res->len, "%s/%s(%d)", str, infostr, mode);
		outAddChar(res, SEP2);
		outStringBuilder(res, "icon");
		outAddChar(res, SEP2);
		res->len += sprintf(res->str + res->len, "%s/%s\n", home, getColor(data, theme));
	}

	if (i == 0) { // maybe check before so the option "All" doesn't show up?
		str[offset - 1] = '\0';
		outStringBuilder(res, "No results");
		outAddChar(res, SEP1);
		outStringBuilder(res, "info");
		outAddChar(res, SEP2);
		res->len += sprintf(res->str + res->len, "%s\n", str);
		return;
	}

	outAddChar(res, SEP1);
	outStringBuilder(res, "active");
	outAddChar(res, SEP2);
	int j = i;
	for (i = 0; i < j; i++) {
		if (active[i] == 1) {
			res->len += sprintf(res->str + res->len, "%d,", i + 1);
		}
	}
	
	str[offset - 1] = '\0';
	outStringBuilder(res, "\nBack");
	outAddChar(res, SEP1);
	outStringBuilder(res, "info");
	outAddChar(res, SEP2);
	res->len += sprintf(res->str + res->len, "%s\n", str);
}

void generateThemeOptions(Data *data, int selected_theme, OUT_STRING *res) {
	GHashTableIter iter;
	char *key = NULL;
	DataObjArray *current = NULL;
	// DataObj *arr;
	int theme;

	int active[g_hash_table_size(data->main_table) - 1], i;
	int mode;
	char *home = getenv("HOME");


	outStringBuilder(res, "All");
	outAddChar(res, SEP1);
	outStringBuilder(res, "info");
	outAddChar(res, SEP2);
	res->len += sprintf(res->str + res->len, "theme%d/All(3)", selected_theme);
	outAddChar(res, SEP2);
	outStringBuilder(res, "icon");
	outAddChar(res, SEP2);
	res->len += sprintf(res->str + res->len, "%s/%s\n", home, getColor(data, selected_theme));

	g_hash_table_iter_init (&iter, data->main_table);
	for (i = 0; g_hash_table_iter_next (&iter, (void **)&key, (void **)&current); i++) {
		// if (i == 2) {
			// arr = current->list->arr;
			mode = current->mode;
			if (mode == VAR) {	
				theme = ((Theme *)current->theme)->big;
			} else {
				theme = (int)((long int)(current->theme));
			}
			res->len += sprintf(res->str + res->len, "%s", key);
			if (mode == SUB) { // sub
				res->len += sprintf(res->str + res->len, " --> %d/%d", current->dependency_table->active[selected_theme], getTableSize(current->dependency_table));
			}
			outAddChar(res, SEP1);
			outStringBuilder(res, "info");
			outAddChar(res, SEP2);
			res->len += sprintf(res->str + res->len, "theme%d/%s(%d)", selected_theme, key, mode);
			outAddChar(res, SEP2);
			outStringBuilder(res, "icon");
			outAddChar(res, SEP2);

			res->len += sprintf(res->str + res->len, "%s/%s\n", home, getColor(data, theme));

			active[i] = (theme == selected_theme) ? 1 : 0;
	}

	if (i == 0) {
		// No info, will go straight to main menu
		outStringBuilder(res, "No options available\n");
		return;
	}

	outAddChar(res, SEP1);
	outStringBuilder(res, "active");
	outAddChar(res, SEP2);
	int j = i;
	for (i = 0; i < j; i++) {
		if (active[i] == 1) {
			res->len += sprintf(res->str + res->len, "%d,", i + 1);
		}
	}
	outStringBuilder(res,"\nBack\n");
	// info is not defined so it will be null and take you back to main menu

	outAddChar(res, '\n');

}

// switch??? jump table???
// WILL NOT put \n in the end
void printDataObj(DataObj *data, OUT_STRING *res) {
	TYPE type = data->type;
	if (type == INT) {
		res->len += sprintf(res->str + res->len, "%ld", (long int)data->info);
	} else if (type == STRING) {
		res->len += sprintf(res->str + res->len, "%s", (char *)data->info);
	} else if (type == EMPTY) {
		outStringBuilder(res, "Empty");
	} else if (type == LIST) {
		outStringBuilder(res, "Printing lists not implemented\n");
		exit(1);
	} else {
		Theme *theme = (Theme *)data->info;
		res->len += sprintf(res->str + res->len, "%d.%d", theme->big, theme->small);
	}
}

void printMainOptions(Data *data, OUT_STRING *res) {
	int i;
	char * home = getenv("HOME");
	int len = getNumberOfColors(data),
	total = getTableSize(data);
	for (i = 0; i < len; i++) {
		// turn this into a function?? how, it has variable number of args
		res->len += sprintf(res->str + res->len, "Theme %d --> %d/%d", i, getActivePerTheme(data, i), total);
		outAddChar(res, SEP1);
		outStringBuilder(res, "info");
		outAddChar(res, SEP2);
		res->len += sprintf(res->str + res->len, "theme%d", i);
		outAddChar(res, SEP2);
		outStringBuilder(res, "icon");
		outAddChar(res, SEP2);
		res->len += sprintf(res->str + res->len,"%s/%s\n", home, getColor(data, i));
	}
	outAddChar(res, SEP1);
	outStringBuilder(res, "active");
	outAddChar(res, SEP2);
	res->len += sprintf(res->str + res->len,"%d\n", getMostUsed(data));
}

void printThemeOptions(Data *data, int theme, OUT_STRING *res) {
	outAddChar(res, SEP1);
	outStringBuilder(res, "prompt");
	outAddChar(res, SEP2);
	res->len += sprintf(res->str + res->len, "Theme %d\n", theme);

	generateThemeOptions(data, theme, res);
}
