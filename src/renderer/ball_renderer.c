#include "ball_renderer.h"
#include "../core/render.h" // For GL-related functions and MeshObject
#include <GL/glew.h>
#include <GL/gl.h>
#include <stdlib.h> // For malloc

#define BALL_SCALE BALL_SIZE
#define SHADOW_CONSTANT 0.2f

int initBallRenderer(ResourceManager* rm)
{
    return 0;
}

// Function to draw the ball
void drawBallRenderer(const BallInfo* ballInfo, double alpha, ResourceManager* rm)
{
    if (ballInfo->visible == 1) {
        // we draw ball and its shadow. shadow's x offset is just proportional to ball's height.
        glBindTexture(GL_TEXTURE_2D, resource_manager_get_texture(rm, "data/textures/pallo.tga"));
        glPushMatrix();
        glTranslatef(
            (float)(alpha * ballInfo->location.x + (1 - alpha) * ballInfo->lastLocation.x),
            (float)(alpha * ballInfo->location.y + (1 - alpha) * ballInfo->lastLocation.y),
            (float)(alpha * ballInfo->location.z + (1 - alpha) * ballInfo->lastLocation.z)
        );
        glScalef(BALL_SCALE, BALL_SCALE, BALL_SCALE);
        glCallList(resource_manager_get_model(rm, "data/models/pallo.obj"));
        glPopMatrix();
        // and the shadow
        glEnable(GL_BLEND);
        glDisable(GL_LIGHTING);
        glPushMatrix();
        glTranslatef(
            (float)(alpha * ballInfo->location.x + (1 - alpha) * ballInfo->lastLocation.x +
                    -SHADOW_CONSTANT * (alpha * ballInfo->location.y + (1 - alpha) * ballInfo->lastLocation.y)),
            SHADOW_HEIGHT, (float)(alpha * ballInfo->location.z + (1 - alpha) * ballInfo->lastLocation.z)
        );
        glScalef(BALL_SCALE, BALL_SCALE, BALL_SCALE);
        glCallList(resource_manager_get_model(rm, "data/models/shadow.obj"));
        glPopMatrix();
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    }
}

int cleanBallRenderer(void)
{
    return 0;
}
