#include "handlers.h"
#include "display.h"
#include "queries.h"
#include <string.h>

// format received: <number>/<arg1>/<arg2>/...
// 0: lookup <name>/<subname> (subname only if it has sub tables)
// 1: change to (not implemented) <theme>/<name>/<subname>
void queryHandler(Data *data, char *info, OUT_STRING *res) {
	char *endptr;
	int query = (int)strtol(info, &endptr, 10);
	if (query != 0) {
		printf("Only query 0 has been completed\n");
		exit(1);
	}
	// no error checking, responsibility of user?
	query0(data, endptr + 1, res); // skip numbers and '/'
}

// input format: <theme>/<option>(<mode>)/...
OUT_STRING * menuHandler(Data *data, char *original_info) {
	if (original_info == NULL) { // ""
		printMainOptions(data);
	} else {
		// when varHandler calls displayVar, what happens if string overflows????????
		// info used big alloca just to be safe
		char *info = alloca(sizeof(char) * INFO_SIZE);
		strcpy(info, original_info);

		int i;
		for (i = 0; info[i] != '\0' && info[i] != '/'; i++);
		if (info[i] == '\0') { // "theme<x>"
			printThemeOptions(data, atoi(info + 5));
		} else { // "theme<x>/option(<m>)/..." m can be 0(apply), 1(show_sub) or 2(show_var
			int j;
			for (j = i + 1; info[j] != '('; j++);
			handlerFunc *handlers[] = {applyHandler, subHandler, varHandler, allHandler};
			handlers[info[j + 1] - '0'](data, info, i + 1); // - '0' so that '1' == 1, etc
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
	changeThemeApply(dataobjarray->list->arr, &dataobjarray->theme, theme, data->active);

	// back to previous menu
	int j;
	// checks if nothing relevant happened
	// info received is .../<option>(0)
	// previous could be theme<x>/... or <option><x>/...
	for (j = 0; info[j] != '/'; j++);
	info[offset - 1] = '\0';
	// can either be var or sub, never apply
	// or it can be theme
	if (j + 1 == offset) { // previous menu is just the menu of a theme
		printThemeOptions(data, theme);
	} else { // previous menu is either sub or var (for now sub does nothing, it is assumed whoever calls this displays hte options itself)
		for (i = offset - 2; info[i] != '/'; i--);
		if (info[offset - 3] - '0' == SUB) { // sub
			// crashing because it needs to go back to a data * that no longer exists. need to apply in sub.
			// subHandler(data, info, i + 1);
			// for now, this will do nothing, since the apply of the new theme is correct but the display of previous data isn't
			return;
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
			// why does this not call changeTheme ????
			int new_theme = atoi(info + 5);
			Theme *theme = (Theme *)(dataobjarray->theme);
			if (new_theme != theme->big || res != theme->small) {
				// printf("changing theme from %d.%d to %d.%d\n", theme->big, theme->small, new_theme, (int)res);
				theme->big = new_theme;
				theme->small = (int)res;
				// go back to before click
			}
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
		switch ((info[j + 1] - '0'))
		{
		case APPLY: // apply
			applyHandler(dataobjarray->dependency_table, info, i + 1); // I assume some magic happens here and a \0 is perfectly placed to allow to pass info + i + 1 next
			// magic is correct but offset is not
			// ineficient but idc
			for (i -= 1; info[i] != '/'; i--);
			displaySub(data, info, i + 1);
			break;
		case SUB: // sub
			subHandler(dataobjarray->dependency_table, info, i + 1);
			break;
		case VAR: // var
			varHandler(dataobjarray->dependency_table, info, i + 1);
			break;
		case ALL:
			allHandler(dataobjarray->dependency_table, info, i + 1);
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
	if (offset == i + 1) { // clicked all in the first menu of a theme
		generateThemeOptions(data, theme);
	} else {
		info[offset - 1] = '\0';
		// displaySub(data, info, i + 1); this is bad because data is already the dependency table of something
		// either: copy paste display sub but without using dependency table
		// or: trace the entire path back to the menu it is supposed to be in -> bad because original table was lost, would have to parse again
		displaySubWithoutDep(data, info, i + 1);
	}
}

// change to jump table??
void applyAll(Data *data, int theme) {
	GHashTableIter iter;
	char *key = NULL;
	DataObjArray *current = NULL;
	char mode;

	int *active = data->active, old_theme, new_theme;

	g_hash_table_iter_init (&iter, data->main_table);
	while (g_hash_table_iter_next (&iter, (void **)&key, (void **)&current))
	{

		mode = current->mode;
		switch (mode) {
			case APPLY: // apply
				changeThemeApply(current->list->arr, &current->theme, theme, active);
				break;
			case VAR: // var
				changeThemeVar(current->list->arr, current->theme, theme, 1, active);
				break;
			case SUB: // sub
				// applies to all in subtable, then changes the theme to whatever the new most used theme is
				// also updates data->active[]
				applyAll(current->dependency_table, theme);
				old_theme = (long int)current->theme;
				new_theme = getMostUsed(current->dependency_table);
				// if (old_theme != new_theme) {
				data->active[old_theme] -= 1;
				data->active[new_theme] += 1;
				current->theme = (void *) ((long int)new_theme);
				// }
				break;
		}
	}
	// calculate new max
	int i, max = 0;
	for (i = 1; i < (const int)data->color_icons->len; i++) {
		if (active[max] < active[i]) max = i;
	}
	active[i] = max;
}

void changeThemeApply(DataObj *arr, void **old_theme, int theme, int *active) {
	DataObj *infoObj = &arr[*(long int *)old_theme];
	if (*(long int *)old_theme != theme) {
		if (infoObj->type != EMPTY) {
			active[*(long int *)old_theme] -= 1;
			active[theme] += 1;
			*old_theme = (void *)((long int)theme);
		}
	}
}

// used to receive DataObj, now has to just receive old theme (change from x to y)
void changeThemeVar(DataObj *arr, Theme *old_theme, int big, int small, int *active) {
	// printf("%d %d %d %d\n", old_theme->big, big, old_theme->small, small);
	if (old_theme->big != big || old_theme->small != small) {
		DataObj *infoObj = &arr[big];
		if (infoObj->type != EMPTY) {
			List *list = (List *)infoObj->info;
			infoObj = &(list->arr[small - 1]);
			if (infoObj->type != EMPTY) {
				old_theme->big = big;
				old_theme->small = small;
				active[old_theme->big] -= 1;
				active[big] += 1;
			}
		}
	}
}
