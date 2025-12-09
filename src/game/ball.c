#include "globals.h"
#include "ball.h"
#include "../renderer/ball_renderer.h" // Include the new renderer header

#define BALL_SCALE BALL_SIZE
#define SHADOW_CONSTANT 0.2f


// initializes ball as an entity in the empty space. ball has to be located to the field in a different place
int initBall()
{
	return initBallRenderer();
}

void drawBall(BallInfo* ballInfo, double alpha)
{
	drawBallRenderer(ballInfo, alpha);
}


// cleaning keeps the house tidy
int cleanBall()
{
	return cleanBallRenderer();
}

