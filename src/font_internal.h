#ifndef FONT_INTERNAL_H
#define FONT_INTERNAL_H

#if defined(__wii__)

#include "empty_background_tpl.h"
#define empty_background 0

#include "font_tpl.h"
#define font 0

#else
#endif

#define FONT_SCALE 0.01f
#define FONT_OFFSET_SCALE 0.012f

extern StateInfo stateInfo;

static void printCharacter(char character);

#if defined(__wii__)
extern Mtx view;
static Mtx model, modelview;

static GXTexObj fontTexture;
static TPLFile fontTPL;
static GXTexObj emptyTexture;
static TPLFile emptyTPL;

static MeshObject* planeMesh;
static void *planeDisplayList;
static u32 planeListSize;

#else

static GLuint fontTexture;
static GLuint emptyTexture;

static MeshObject* planeMesh;
static GLuint planeDisplayList;

#endif

#endif /* FONT_INTERNAL_H */