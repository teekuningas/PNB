#ifndef FONT_INTERNAL_H
#define FONT_INTERNAL_H

#define FONT_SCALE 0.01f
#define FONT_OFFSET_SCALE 0.012f

extern StateInfo stateInfo;

static void printCharacter(char character);

static GLuint fontTexture;
static GLuint emptyTexture;

static MeshObject* planeMesh;
static GLuint planeDisplayList;

#endif /* FONT_INTERNAL_H */
