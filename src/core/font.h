#ifndef FONT_H
#define FONT_H

#include "globals.h"

int initFont();
void printText(const char* str, unsigned int len, float x, float y, float size);
void drawFontBackground();
int cleanFont();

#endif /* FONT_H */
