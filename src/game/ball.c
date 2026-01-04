#include "globals.h"
#include "ball.h"
#ifndef NO_RENDER
#include "../renderer/ball_renderer.h"
#endif

#define BALL_SCALE BALL_SIZE
#define SHADOW_CONSTANT 0.2f


// initializes ball as an entity in the empty space. ball has to be located to the field in a different place
int initBall()
{
#ifndef NO_RENDER
	if (initBallRenderer() != 0) return -1;
#endif
	return 0;
}
void drawBall(const BallInfo* ballInfo, double alpha)
{
	// well ball renderer handles the ball drawing
#ifndef NO_RENDER
	drawBallRenderer(ballInfo, alpha);
#endif
}
int cleanBall()
{
#ifndef NO_RENDER
	return cleanBallRenderer();
#else
	return 0;
#endif
}

