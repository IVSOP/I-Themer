#include "handlers.h"
#include "display.h"
#include "queries.h"

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
