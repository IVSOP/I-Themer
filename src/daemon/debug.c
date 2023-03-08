#include "debug.h"

void printSpace(int depth) {
	for (; depth; depth--) putchar(' ');
}

void dumpList(List *list, long int depth) {
	DataObj *tmp, *arr = list->arr;
	TYPE type;
	int len = list->len, i;
	for (i = 0; i < len; i++) {
		tmp = &arr[i];
		type = tmp->type;
		printSpace(depth);
		if (type == STRING) {
			printf("string: %s\n", (char *)tmp->info);
		} else if (type == LIST) {
			printf("list:\n");
			dumpList((List *)tmp->info, depth + 4);
		} else if (type == EMPTY) {
			printf("empty\n");
		} else {
			fprintf(stderr, "Invalid type: %d\n", type);
			exit(1);
		}
	}
}

void dumpDataObjArray(DataObjArray * data, long int depth) {
	printSpace(depth);
	printf("[--------------[%ld]\n", depth);
	printSpace(depth);
	printf("Name: %s\n", data->name);
	printSpace(depth);
	printf("Mode: %d\n", data->mode);
	printSpace(depth);
	printf("Theme: ");
	if (data->mode == VAR) {
		Theme *theme = (Theme *)data->theme;
		printf("%d.%d\n", theme->big, theme->small);
	} else {
		printf("%ld\n", (long int)data->theme);
	}

	// sub have no data, but their subtables do
	if (data->mode == SUB) {
		if (data->dependency_table != NULL) {
			printSpace(depth);
			printf("Found dependency table\n");
			dumpTable((Data *)data->dependency_table, depth + 4);
		}
	} else {
		dumpList(data->list, depth);
	}

	printSpace(depth);
	printf("[--------------[%ld]\n", depth);
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

// incomplete
// void printValue(DataObj *data) {
// 	TYPE type = data->type;
// 	if (type == INT) {
// 		printf("%ld", (long int)data->info);
// 	} else if (type == STRING) {
// 		printf("%s", (char *)data->info);
// 	}
// }