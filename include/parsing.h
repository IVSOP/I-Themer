#ifndef PARSING
#define PARSING
#include <stdlib.h>
#include <stdio.h>

#define BUFFER_SIZE 64
#define LINE_STR_SIZE 256
#define DATA_BUFF_SIZE 64

typedef enum {
	INT = 0,
	STRING = 1,
	INT_VERSION = 2, // will get stored as string
	EMPTY = 3,
	LIST = 4,
	OFFSET = 5,
} TYPE;

typedef struct DataObj DataObj;
typedef struct Data Data;
typedef struct DataObjArray DataObjArray;

typedef void freeFunc(void *);

int readString(FILE *fp, char *buffer);
int readStringDelim(FILE *fp, char delim, char *buffer);
Data *parseMainTable(FILE *);
DataObjArray *parseLine(FILE *fp);
void freeTableData(void *);
void dumpDataObjArray(DataObjArray *, int depth);

#endif
