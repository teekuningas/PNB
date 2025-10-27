#ifndef FONT_H
#define FONT_H

int initFont();
void drawFontBackground();
void printText(const char* str, unsigned int len, float x, float y, float size);
void printText2D(const char* str, unsigned int len, float x, float y, float size);
int cleanFont();

#endif /* FONT_H */
