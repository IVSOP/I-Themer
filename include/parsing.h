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

char *readString(char *str, int *len);
Data *parseMainTable(FILE *);
DataObjArray *parseLine(FILE *fp);
void freeTableData(Data *data);
void dumpTable(Data *data, long int depth);
DataObjArray *tableLookup(Data *data, char *str);
int getLen(DataObjArray *data);
void printValue(DataObj *data);
DataObj *getDataObj(DataObjArray *data, int i);
int getActivePerTheme(int theme);
void *getValue(DataObj *data);
GHashTable *getTable(Data *data);
void generateThemeOptions(Data *data, int theme);

#endif
