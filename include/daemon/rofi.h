#ifndef ROFI
#define ROFI

#include "parsing.h"
int mainRofiLoop(char *filename, char *dir);
int readData(char *filename);
void inputHandler(Data *data, char *input, char *dir);

#endif
