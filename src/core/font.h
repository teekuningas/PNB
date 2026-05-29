#ifndef FONT_H
#define FONT_H

int init_font();
void draw_font_background();
void print_text(const char* str, unsigned int len, float x, float y, float size);
void print_text_2d(const char* str, unsigned int len, float x, float y, float size);
float get_text_width_2d(const char* str, unsigned int len, float size);
int clean_font();

#endif /* FONT_H */
