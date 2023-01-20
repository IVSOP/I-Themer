#ifndef ROFI
#define ROFI

#include "parsing.h"
int mainRofiLoop(char *filename);
int readData(char *filename);
void printThemeOptions(Data *data, int theme);
void inputHandler(Data *data, char *input);

#endif
