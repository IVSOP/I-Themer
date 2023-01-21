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
	LIST = 4, // stored as DataObjArray *, not as Data *
} TYPE;


typedef struct DataObj DataObj;
typedef struct Data Data;
typedef struct DataObjArray DataObjArray;
typedef struct Theme Theme;

typedef void freeFunc(void *);

//handlers
typedef void handlerFunc(Data *, char *, int);
void queryHandler(Data *data, char *query);
void applyHandler(Data *data, char *info, int offset);
void varHandler(Data *data, char *info, int offset);
void subHandler(Data *data, char *info, int offset);

//displayers (same type as handlerFunc)
void displayVar(Data *data, char *str, int offset);
void displaySub(Data *data, char *str, int offset);

void saveTableToFile(Data *data, char *name);


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

//etc
void generateThemeOptions(Data *data, int theme);
void printValue(DataObj *data);

//debug
void dumpTable(Data *data, long int depth);

//freeing
void freeTableData(Data *data);

#endif
