#ifndef HANDLERS_H
#define HANDLERS_H

#include "parsing.h"

//handlers
typedef void handlerFunc(Data *, char *, int);
void queryHandler(Data *data, char *query);
void applyHandler(Data *data, char *info, int offset);
void varHandler(Data *data, char *info, int offset);
void subHandler(Data *data, char *info, int offset);
void allHandler(Data *data, char *info, int offset);
void applyAll(Data *data, int theme);

#endif