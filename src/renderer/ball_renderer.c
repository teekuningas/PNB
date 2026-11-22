#include "ball_renderer.h"
#include "field.h"
#include "display.h"
#include "world.h"
#include <GL/glew.h>
#include <GL/gl.h>
#include <stdlib.h> // For malloc

#define BALL_SCALE BALL_SIZE
#define SHADOW_CONSTANT 0.2f

int init_ball_renderer(ResourceManager* rm)
{
    return 0;
}

// Function to draw the ball
void draw_ball_renderer(const BallInfo* ballInfo, double alpha, ResourceManager* rm)
{
    if (ballInfo->visible == 1) {
        // we draw ball and its shadow. shadow's x offset is just proportional to ball's height.
        glBindTexture(GL_TEXTURE_2D, resource_manager_get_texture(rm, "data/textures/pallo.tga"));
        glPushMatrix();
        glTranslatef(
            (float)(alpha * ballInfo->location.x + (1 - alpha) * ballInfo->previous_location.x),
            (float)(alpha * ballInfo->location.y + (1 - alpha) * ballInfo->previous_location.y),
            (float)(alpha * ballInfo->location.z + (1 - alpha) * ballInfo->previous_location.z)
        );
        glScalef(BALL_SCALE, BALL_SCALE, BALL_SCALE);
        glCallList(resource_manager_get_model(rm, "data/models/pallo.obj"));
        glPopMatrix();
        // and the shadow
        glEnable(GL_BLEND);
        glDisable(GL_LIGHTING);
        glPushMatrix();
        glTranslatef(
            (float)(alpha * ballInfo->location.x + (1 - alpha) * ballInfo->previous_location.x +
                    -SHADOW_CONSTANT * (alpha * ballInfo->location.y + (1 - alpha) * ballInfo->previous_location.y)),
            SHADOW_HEIGHT, (float)(alpha * ballInfo->location.z + (1 - alpha) * ballInfo->previous_location.z)
        );
        glScalef(BALL_SCALE, BALL_SCALE, BALL_SCALE);
        glCallList(resource_manager_get_model(rm, "data/models/shadow.obj"));
        glPopMatrix();
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    }
}

int clean_ball_renderer(void)
{
    return 0;
}
