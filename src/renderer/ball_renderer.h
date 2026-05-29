#ifndef BALL_RENDERER_H
#define BALL_RENDERER_H

#include "../include/globals.h" // For BallInfo structure
#include "../core/resource_manager.h"

// Function to initialize ball rendering resources (textures, models)
int init_ball_renderer(ResourceManager* rm);

// Function to draw the ball
void draw_ball_renderer(const BallInfo* ballInfo, double alpha, ResourceManager* rm);

// Function to clean up ball rendering resources
int clean_ball_renderer(void);

#endif // BALL_RENDERER_H
