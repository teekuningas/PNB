/*
    Draws text on the screen. The idea is that we render one plane per character, all
    textured from a single font image, and it is the texture coordinates that change per
    character. Lights and the z buffer should be off and we should be drawing last.

    This file used to own a copy of the background texture and the plane mesh as well,
    because the same texture could not be loaded twice — a constraint the ResourceManager
    has since removed. The background the menus and the game screen draw comes from there
    now (menu_helpers.c, game_screen.c), so those copies are gone.
*/

#include "globals.h"
#include "render.h"

#include "font.h"

static void printCharacter2D(char character);
static void renderCharacter2D(float top, float bottom, float left, float right);

static GLuint fontTexture;

int init_font()
{
    if (try_loading_texture_gl(&fontTexture, "data/textures/font.tga", "font") != 0) return -1;
    return 0;
}

void print_text_2d(const char* str, unsigned int len, float x, float y, float size)
{
    int i;
    glBindTexture(GL_TEXTURE_2D, fontTexture);

    // size is desired font height in pixels.
    // The original character quad has a height of 2.0 and width of 1.2.
    float char_height_pixels = size;
    float char_width_pixels = size * 0.6f;
    float char_spacing_pixels = size * 0.7f; // A bit wider than the char for spacing

    for (i = 0; i < (int)len; i++) {
        glPushMatrix();
        // We translate to the *center* of where the character should be.
        glTranslatef(x + i * char_spacing_pixels + (char_width_pixels / 2.0f), y + (char_height_pixels / 2.0f), 0.0f);
        // And then scale the unit quad (1.2 x 2.0) to the desired pixel size.
        glScalef(char_width_pixels / 1.2f, char_height_pixels / 2.0f, 1.0f);

        // This calls the new 2D-specific printCharacter function.
        printCharacter2D(str[i]);

        glPopMatrix();
    }
}

// Nothing to release since the plane mesh moved to the ResourceManager: the font
// texture is owned by GL and torn down with the context. Kept as the shutdown hook
// main.c calls, so a future font resource has a place to be freed.
int clean_font()
{
    return 0;
}

static void renderCharacter2D(float top, float bottom, float left, float right)
{
    // 2D immediate mode drawing for ortho projection.
    // Vertex order is Clockwise (TL -> BL -> BR -> TR) to match glFrontFace(GL_CW)
    glBegin(GL_QUADS);
    // Top-Left
    glTexCoord2f(left, top);
    glVertex2f(-0.6f, -1.0f);
    // Bottom-Left
    glTexCoord2f(left, bottom);
    glVertex2f(-0.6f, 1.0f);
    // Bottom-Right
    glTexCoord2f(right, bottom);
    glVertex2f(0.6f, 1.0f);
    // Top-Right
    glTexCoord2f(right, top);
    glVertex2f(0.6f, -1.0f);
    glEnd();
}

static void printCharacter2D(char character)
{
    // This is a copy of the original printCharacter, but it calls the new
    // renderCharacter2D function. This keeps the new rendering path
    // completely separate from the legacy one.
    // set the texture coordinates for drawing.
    switch (character) {
    case 'A':
    case 'a':
        renderCharacter2D(0.94f, 0.845f, 0.0f, 0.1f);
        break;
    case 'B':
    case 'b':
        renderCharacter2D(0.94f, 0.845f, 0.1f, 0.19f);
        break;
    case 'C':
    case 'c':
        renderCharacter2D(0.94f, 0.845f, 0.195f, 0.28f);
        break;
    case 'D':
    case 'd':
        renderCharacter2D(0.94f, 0.845f, 0.28f, 0.37f);
        break;
    case 'E':
    case 'e':
        renderCharacter2D(0.94f, 0.845f, 0.37f, 0.46f);
        break;
    case 'F':
    case 'f':
        renderCharacter2D(0.94f, 0.845f, 0.465f, 0.555f);
        break;
    case 'G':
    case 'g':
        renderCharacter2D(0.94f, 0.845f, 0.555f, 0.645f);
        break;
    case 'H':
    case 'h':
        renderCharacter2D(0.94f, 0.845f, 0.645f, 0.735f);
        break;
    case 'I':
    case 'i':
        renderCharacter2D(0.94f, 0.845f, 0.735f, 0.815f);
        break;
    case 'J':
    case 'j':
        renderCharacter2D(0.94f, 0.83f, 0.79f, 0.87f);
        break;
    case 'K':
    case 'k':
        renderCharacter2D(0.94f, 0.845f, 0.87f, 0.96f);
        break;
    case 'L':
    case 'l':
        renderCharacter2D(0.805f, 0.71f, 0.0f, 0.1f);
        break;
    case 'M':
    case 'm':
        renderCharacter2D(0.805f, 0.71f, 0.095f, 0.195f);
        break;
    case 'N':
    case 'n':
        renderCharacter2D(0.805f, 0.71f, 0.205f, 0.295f);
        break;
    case 'O':
    case 'o':
        renderCharacter2D(0.805f, 0.71f, 0.305f, 0.395f);
        break;
    case 'P':
    case 'p':
        renderCharacter2D(0.805f, 0.71f, 0.395f, 0.485f);
        break;
    case 'Q':
    case 'q':
        renderCharacter2D(0.805f, 0.695f, 0.49f, 0.575f);
        break;
    case 'R':
    case 'r':
        renderCharacter2D(0.805f, 0.71f, 0.585f, 0.675f);
        break;
    case 'S':
    case 's':
        renderCharacter2D(0.805f, 0.71f, 0.675f, 0.765f);
        break;
    case 'T':
    case 't':
        renderCharacter2D(0.805f, 0.71f, 0.765f, 0.855f);
        break;
    case 'U':
    case 'u':
        renderCharacter2D(0.805f, 0.71f, 0.855f, 0.945f);
        break;
    case 'V':
    case 'v':
        renderCharacter2D(0.675f, 0.575f, 0.0f, 0.1f);
        break;
    case 'W':
    case 'w':
        renderCharacter2D(0.675f, 0.575f, 0.1f, 0.21f);
        break;
    case 'X':
    case 'x':
        renderCharacter2D(0.675f, 0.575f, 0.22f, 0.31f);
        break;
    case 'Y':
    case 'y':
        renderCharacter2D(0.675f, 0.575f, 0.31f, 0.4f);
        break;
    case 'Z':
    case 'z':
        renderCharacter2D(0.675f, 0.575f, 0.4f, 0.49f);
        break;
    case '0':
        renderCharacter2D(0.405f, 0.31f, 0.64f, 0.73f);
        break;
    case '1':
        renderCharacter2D(0.405f, 0.31f, 0.735f, 0.815f);
        break;
    case '2':
        renderCharacter2D(0.405f, 0.31f, 0.82f, 0.91f);
        break;
    case '3':
        renderCharacter2D(0.405f, 0.31f, 0.905f, 1.0f);
        break;
    case '4':
        renderCharacter2D(0.27f, 0.18f, 0.01f, 0.1f);
        break;
    case '5':
        renderCharacter2D(0.27f, 0.18f, 0.09f, 0.18f);
        break;
    case '6':
        renderCharacter2D(0.27f, 0.18f, 0.175f, 0.26f);
        break;
    case '7':
        renderCharacter2D(0.27f, 0.18f, 0.26f, 0.35f);
        break;
    case '8':
        renderCharacter2D(0.27f, 0.18f, 0.35f, 0.44f);
        break;
    case '9':
        renderCharacter2D(0.27f, 0.18f, 0.435f, 0.525f);
        break;
    case '-':
        renderCharacter2D(0.27f, 0.18f, 0.51f, 0.6f);
        break;
    case '.':
        renderCharacter2D(0.27f, 0.195f, 0.595f, 0.675f);
        break;
    case ',':
        renderCharacter2D(0.27f, 0.195f, 0.67f, 0.75f);
        break;

    default:
        break;
    }
}

float get_text_width_2d(const char* str, unsigned int len, float size)
{
    // Corresponds to the logic in print_text_2d
    float char_spacing_pixels = size * 0.7f;
    return len * char_spacing_pixels;
}
