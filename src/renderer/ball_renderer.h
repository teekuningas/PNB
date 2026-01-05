#ifndef BALL_RENDERER_H
#define BALL_RENDERER_H

#include "../include/globals.h" // For BallInfo structure
#include "../core/resource_manager.h"

// Function to initialize ball rendering resources (textures, models)
int initBallRenderer(ResourceManager* rm);

// Function to draw the ball
void drawBallRenderer(const BallInfo* ballInfo, double alpha, ResourceManager* rm);

// Function to clean up ball rendering resources
int cleanBallRenderer(void);

#endif // BALL_RENDERER_H
