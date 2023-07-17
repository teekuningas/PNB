#include "globals.h"
#include "render.h"

#include "font.h"
#include "font_internal.h"

/*
	provides means to write on the screen and also provides the background that main menu uses and statistics in game screen use. had to put them in
	same place because you cannot load same texture in multiple places.

	the basic idea is just that we render planes one at a time for each character that have texture of a font image, and texture coordinates are the things
	that change. lights and z buffer should be off and we should be drawing last. also here i use this in perspective camera mode so we have to
	have specific kind of camera matrix.

*/

int initFont()
{
	if(tryLoadingTextureGL(&fontTexture, "data/textures/font.tga", "font") != 0) return -1;
	if(tryLoadingTextureGL(&emptyTexture, "data/textures/empty_background.tga", "empty background") != 0) return -1;
	planeMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/models/plane.obj", "Plane", planeMesh, &planeDisplayList) != 0) return -1;
	return 0;
}

void drawFontBackground()
{
	glBindTexture(GL_TEXTURE_2D, emptyTexture);
	glPushMatrix();
	glScalef(2.0f, 1.0f, 1.0f); // 2.0 to make black borders from sides away
	glCallList(planeDisplayList);
	glPopMatrix();
}
void printText(char* str, unsigned int len, float x, float y, float size)
{
	int i;
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	for(i = 0; i < (int)len; i++)
	{
		glPushMatrix();
		glTranslatef(x + i*size*FONT_OFFSET_SCALE, 1.0f, y); // these numeric parameters come from
		glScalef(size*FONT_SCALE, size*FONT_SCALE, size*FONT_SCALE); // texture
		printCharacter(str[i]);
		glPopMatrix();
	}
}

void renderCharacter(float top, float bottom, float left, float right)
{
	// simple immediate mode drawing.
	glBegin(GL_QUADS);
		glTexCoord2f(left, bottom);
		glVertex3f( -0.6f, 0.0f, 1.0f);		// bottom left
		glTexCoord2f(right, bottom);
		glVertex3f(0.6f, 0.0f, 1.0f);	// bottom right
		glTexCoord2f(right, top);
		glVertex3f(0.6f,0.0f, -1.0f); // top right
		glTexCoord2f(left, top);
		glVertex3f( -0.6f,0.0f, -1.0f);	// top left
	glEnd();
}
static void printCharacter(char character)
{
	// set the texture coordinates for drawing.
	switch(character) {
		case 'A':
		case 'a':
			renderCharacter(0.94f, 0.845f, 0.0f, 0.1f);
			break;
		case 'B':
		case 'b':
			renderCharacter(0.94f, 0.845f, 0.1f, 0.19f);
			break;
		case 'C':
		case 'c':
			renderCharacter(0.94f, 0.845f, 0.195f, 0.28f);
			break;
		case 'D':
		case 'd':
			renderCharacter(0.94f, 0.845f, 0.28f, 0.37f);
			break;
		case 'E':
		case 'e':
			renderCharacter(0.94f, 0.845f, 0.37f, 0.46f);
			break;
		case 'F':
		case 'f':
			renderCharacter(0.94f, 0.845f, 0.465f, 0.555f);
			break;
		case 'G':
		case 'g':
			renderCharacter(0.94f, 0.845f, 0.555f, 0.645f);
			break;
		case 'H':
		case 'h':
			renderCharacter(0.94f, 0.845f, 0.645f, 0.735f);
			break;
		case 'I':
		case 'i':
			renderCharacter(0.94f, 0.845f, 0.735f, 0.815f);
			break;
		case 'J':
		case 'j':
			renderCharacter(0.94f, 0.83f, 0.79f, 0.87f);
			break;
		case 'K':
		case 'k':
			renderCharacter(0.94f, 0.845f, 0.87f, 0.96f);
			break;
		case 'L':
		case 'l':
			renderCharacter(0.805f, 0.71f, 0.0f, 0.1f);
			break;
		case 'M':
		case 'm':
			renderCharacter(0.805f, 0.71f, 0.095f, 0.195f);
			break;
		case 'N':
		case 'n':
			renderCharacter(0.805f, 0.71f, 0.205f, 0.295f);
			break;
		case 'O':
		case 'o':
			renderCharacter(0.805f, 0.71f, 0.305f, 0.395f);
			break;
		case 'P':
		case 'p':
			renderCharacter(0.805f, 0.71f, 0.395f, 0.485f);
			break;
		case 'Q':
		case 'q':
			renderCharacter(0.805f, 0.695f, 0.49f, 0.575f);
			break;
		case 'R':
		case 'r':
			renderCharacter(0.805f, 0.71f, 0.585f, 0.675f);
			break;
		case 'S':
		case 's':
			renderCharacter(0.805f, 0.71f, 0.675f, 0.765f);
			break;
		case 'T':
		case 't':
			renderCharacter(0.805f, 0.71f, 0.765f, 0.855f);
			break;
		case 'U':
		case 'u':
			renderCharacter(0.805f, 0.71f, 0.855f, 0.945f);
			break;
		case 'V':
		case 'v':
			renderCharacter(0.675f, 0.575f, 0.0f, 0.1f);
			break;
		case 'W':
		case 'w':
			renderCharacter(0.675f, 0.575f, 0.1f, 0.21f);
			break;
		case 'X':
		case 'x':
			renderCharacter(0.675f, 0.575f, 0.22f, 0.31f);
			break;
		case 'Y':
		case 'y':
			renderCharacter(0.675f, 0.575f, 0.31f, 0.4f);
			break;
		case 'Z':
		case 'z':
			renderCharacter(0.675f, 0.575f, 0.4f, 0.49f);
			break;
		case '0':
			renderCharacter(0.405f, 0.31f, 0.64f, 0.73f);
			break;
		case '1':
			renderCharacter(0.405f, 0.31f, 0.735f, 0.815f);
			break;
		case '2':
			renderCharacter(0.405f, 0.31f, 0.82f, 0.91f);
			break;
		case '3':
			renderCharacter(0.405f, 0.31f, 0.905f, 1.0f);
			break;
		case '4':
			renderCharacter(0.27f, 0.18f, 0.01f, 0.1f);
			break;
		case '5':
			renderCharacter(0.27f, 0.18f, 0.09f, 0.18f);
			break;
		case '6':
			renderCharacter(0.27f, 0.18f, 0.175f, 0.26f);
			break;
		case '7':
			renderCharacter(0.27f, 0.18f, 0.26f, 0.35f);
			break;
		case '8':
			renderCharacter(0.27f, 0.18f, 0.35f, 0.44f);
			break;
		case '9':
			renderCharacter(0.27f, 0.18f, 0.435f, 0.525f);
			break;
		case '-':
			renderCharacter(0.27f, 0.18f, 0.51f, 0.6f);
			break;
		case '.':
			renderCharacter(0.27f, 0.195f, 0.595f, 0.675f);
			break;
		case ',':
			renderCharacter(0.27f, 0.195f, 0.67f, 0.75f);
			break;

		default:
			break;
	}
}


int cleanFont()
{
	cleanMesh(planeMesh);
	return 0;
}
