#include "globals.h"
#include "ball.h"
#ifndef NO_RENDER
#include "../renderer/ball_renderer.h"
#endif

#define BALL_SCALE BALL_SIZE
#define SHADOW_CONSTANT 0.2f

// initializes ball as an entity in the empty space. ball has to be located to the field in a different place
int init_ball(ResourceManager* rm)
{
#ifndef NO_RENDER
    if (init_ball_renderer(rm) != 0) return -1;
#endif
    return 0;
}
void draw_ball(const BallInfo* ballInfo, double alpha, ResourceManager* rm)
{
    // well ball renderer handles the ball drawing
#ifndef NO_RENDER
    draw_ball_renderer(ballInfo, alpha, rm);
#endif
}
int clean_ball()
{
#ifndef NO_RENDER
    return clean_ball_renderer();
#else
    return 0;
#endif
}
