#ifndef DISPLAY_H
#define DISPLAY_H

#include "parsing.h"

//displayers (same type as handlerFunc)
void displayVar(Data *data, char *str, int offset, OUT_STRING *res);
void displaySub(Data *data, char *str, int offset, OUT_STRING *res);
void displaySubWithoutDep(Data *data, char *str, int offset, OUT_STRING *res);

void printDataObj(DataObj *data, OUT_STRING *res);
void generateThemeOptions(Data *data, int selected_theme, OUT_STRING *res);
void printThemeOptions(Data *data, int theme, OUT_STRING *res);
void printMainOptions(Data *data, OUT_STRING *res);

#endif