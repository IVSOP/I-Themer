#include "queries.h"

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
		DataObj *arr = dataobjarray->list->arr,
		*themeobj = getThemeObj(dataobjarray);
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
