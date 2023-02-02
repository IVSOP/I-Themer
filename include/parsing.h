#ifndef PARSING
#define PARSING
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>

#define BUFFER_SIZE 64
#define LINE_STR_SIZE 256
#define DATA_BUFF_SIZE 64

typedef enum {
	INT = 0,
	STRING = 1,
	INT_VERSION = 2, // will get stored as string
	EMPTY = 3,
	LIST = 4, // stored as List *, not as Data *
} TYPE;

#define SEP1 putchar('\0')
#define SEP2 putchar('\x1f')

typedef struct DataObj DataObj;
typedef struct Data Data;
typedef struct DataObjArray DataObjArray;
typedef struct Theme Theme;
typedef struct List List;

struct DataObj {
	void *info;
	TYPE type;
};

struct Data {
	GHashTable *main_table;
	GPtrArray *color_icons;
	int *active;
};

struct List {
	DataObj *arr;
	int len;
};

typedef enum {
	APPLY = 0,
	SUB = 1,
	VAR = 2,
	ALL = 3,
} Mode;

//contains data from an entire line
struct DataObjArray {
	char *name;
	Mode mode;
	void *theme;
	List *list;
	DataObj *arr;
	int len; // number of fiels so I know how many to free
	Data * dependency_table;
};

struct Theme {
	int big, small;
};

typedef void freeFunc(void *);

void saveTableToFile(Data *data, char *name);

#define CHECK_FILE_ERROR(fp) {\
	if (fp == NULL) {\
		fprintf(stderr, "Error opening file, exiting\n");\
		exit(1);\
	}\
}

//parsing
Data *parseMainTable(FILE *fp, GPtrArray *colorArr);
char *readString(char *str, int *len);
DataObjArray *parseLine(FILE *fp);
GPtrArray *parseColors(char *name);

//getters
GHashTable *getTable(Data *data);
void *getValue(DataObj *data);
DataObj *getDataObj(DataObjArray *data, int i);
int getLen(DataObjArray *data);
DataObjArray *tableLookup(Data *data, char *str);
char *getColor(Data *data, int theme);
int getNumberOfColors(Data *data);
int getActivePerTheme(Data *data, int theme);
int getThemeBig(DataObj *themeobj);
int getMostUsed(Data *data);
int getTableSize(Data *data);

//freeing
void freeTableData(Data *data);

#endif
