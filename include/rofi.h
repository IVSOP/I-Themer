#ifndef ROFI
#define ROFI

#include "parsing.h"
int mainRofiLoop(char *filename);
int readData(char *filename);
void inputHandler(Data *data, char *input);

#endif
