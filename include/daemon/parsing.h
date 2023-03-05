#ifndef PARSING
#define PARSING
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>

#define BUFFER_SIZE 256
#define LINE_STR_SIZE 256
#define DATA_BUFF_SIZE 64
#define INFO_SIZE 1024
#define STR_RESULT_SIZE INFO_SIZE - sizeof(int)

typedef struct {
	int len;
	char str[STR_RESULT_SIZE];
} OUT_STRING;


typedef enum {
	INT = 0,
	STRING = 1,
	INT_VERSION = 2, // will get stored as string
	EMPTY = 3,
	LIST = 4, // stored as List *, not as Data *
} TYPE;

#define SEP1 '\0'
#define SEP2 '\x1f'

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
	Data * dependency_table;
};

struct Theme {
	int big, small;
};

typedef void freeFunc(void *);

void saveTableToFile(Data *data, char *name, char *dir);

#define CHECK_FILE_ERROR(fp) {\
	if (fp == NULL) {\
		char _message[64];\
		snprintf(_message, 64, "Error opening file in %s, exiting\n", __func__);\
		perror(_message);\
		exit(1);\
	}\
}

void outStringBuilder(OUT_STRING *res, char *str);
void outAddChar(OUT_STRING *res, char chr);

//parsing
Data *parseMainTable(FILE *fp, GPtrArray *colorArr, char *dir);
char *readString(char *str, int *len);
DataObjArray *parseLine(FILE *fp);
GPtrArray *parseColors(char *name, char *dir);

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
