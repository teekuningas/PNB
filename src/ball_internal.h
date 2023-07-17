#ifndef BALL_INTERNAL_H
#define BALL_INTERNAL_H

#include "globals.h"

#define BALL_SCALE BALL_SIZE
#define SHADOW_CONSTANT 0.2f

static GLuint ballTexture;

static MeshObject* ballMesh;
static GLuint ballDisplayList;

static MeshObject* shadowMesh;
static GLuint shadowDisplayList;

// stateinfo
extern StateInfo stateInfo;

#endif /* BALL_INTERNAL_H */
