#ifndef BALL_RENDERER_H
#define BALL_RENDERER_H

#include "../include/globals.h" // For BallInfo structure

// Function to initialize ball rendering resources (textures, models)
int initBallRenderer(void);

// Function to draw the ball
void drawBallRenderer(BallInfo* ballInfo, double alpha);

// Function to clean up ball rendering resources
int cleanBallRenderer(void);

#endif // BALL_RENDERER_H
