#ifndef BALL_INTERNAL_H
#define BALL_INTERNAL_H

#include "globals.h"

#if defined(__wii__)

#include "pallo_tpl.h"
#define pallo 0

#else
#endif

#define BALL_SCALE BALL_SIZE
#define SHADOW_CONSTANT 0.2f

#if defined(__wii__)
extern Mtx view;
static Mtx model, modelview, mvi;

static TPLFile ballTPL;
static GXTexObj ballTexture;

static MeshObject* ballMesh;
static void *ballDisplayList;
static u32 ballListSize;

static MeshObject* shadowMesh;
static void *shadowDisplayList;
static u32 shadowListSize;
u8 *shadowColors;
#else

static GLuint ballTexture;

static MeshObject* ballMesh;
static GLuint ballDisplayList;

static MeshObject* shadowMesh;
static GLuint shadowDisplayList;

#endif

// stateinfo
extern StateInfo stateInfo;

#endif /* BALL_INTERNAL_H */