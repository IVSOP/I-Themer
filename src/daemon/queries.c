#include "queries.h"

// format received: <arg1>/<arg2>/..
void query0(Data *data, char *info, OUT_STRING *res) {
	int i;
	for (i = 0; info[i] != '\0' && info[i] != '/'; i++); 
	char tmp = info[i];
	tmp = info[i];
	info[i] = '\0';
	DataObjArray *dataobjarray = (DataObjArray *)g_hash_table_lookup(data->main_table, info);
	if (tmp == '\0') { // print data
		// can only print strings and lists (of strings)
		// no error checking done???

		res->len = 0; // ??????????????????

		if (dataobjarray->mode == VAR) { // guaranteed to be string inside of list and has theme of type INT_VERSION
			Theme *theme = (Theme *)dataobjarray->theme;
			List *list = (&(dataobjarray->list->arr[theme->big]))->info;
			// printf("%s\n", (char *)((&(list->arr[theme->small - 1]))->info));
			res->len = stpncpy(res->str, (char *)((&(list->arr[theme->small - 1]))->info), STR_RESULT_SIZE - 1) - res->str;
			res->str[STR_RESULT_SIZE - 1] = '\0';
		} else { // can be string or list, and has theme of type INTt
			long int theme = (long int) dataobjarray->theme;
			DataObj *themeObj = &(dataobjarray->list->arr[theme]);
			if (themeObj->type == STRING) {
				// printf("%s\n", (char *)themeObj->info);
				res->len = stpncpy(res->str, (char *)themeObj->info, STR_RESULT_SIZE - 1) - res->str;
				res->str[STR_RESULT_SIZE - 1] = '\0';
			} else { // list, no checking for empty
				const List *list = (List *)themeObj->info;
				for (i = 0; i < list->len; i++) {
					themeObj = &(list->arr[i]);
					// printf("%s\n", (char *)themeObj->info);
					res->len = stpncpy(res->str, (char *)themeObj->info, STR_RESULT_SIZE - 1) - res->str;
					res->str[STR_RESULT_SIZE - 1] = '\0';
				}
			}
		}
	} else { // have to go into subadata
		query0(dataobjarray->dependency_table, info + i + 1, res);
	}
}
