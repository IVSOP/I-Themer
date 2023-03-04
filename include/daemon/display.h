#ifndef DISPLAY_H
#define DISPLAY_H

#include "parsing.h"

//displayers (same type as handlerFunc)
void displayVar(Data *data, char *str, int offset);
void displaySub(Data *data, char *str, int offset);
void displaySubWithoutDep(Data *data, char *str, int offset);

void printDataObj(DataObj *data);
void generateThemeOptions(Data *data, int selected_theme);
void printThemeOptions(Data *data, int theme);
void printMainOptions(Data *data);

#endif