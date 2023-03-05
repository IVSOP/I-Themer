#ifndef HANDLERS_H
#define HANDLERS_H

#include "parsing.h"

//handlers
typedef void handlerFunc(Data *, char *, int, OUT_STRING *res);
void queryHandler(Data *data, char *query, OUT_STRING *res);
void menuHandler(Data *data, char *original_info, OUT_STRING *res);
void applyHandler(Data *data, char *info, int offset, OUT_STRING *res);
void varHandler(Data *data, char *info, int offset, OUT_STRING *res);
void subHandler(Data *data, char *info, int offset, OUT_STRING *res);
void allHandler(Data *data, char *info, int offset, OUT_STRING *res);
void applyAll(Data *data, int theme, OUT_STRING *res);

void changeThemeApply(DataObj *arr, void **old_theme, int theme, int *active);
void changeThemeVar(DataObj *arr, Theme *old_theme, int big, int small, int *active);

#endif