#ifndef BALL_H
#define BALL_H

#include "globals.h"
#include "resource_manager.h"

int init_ball(ResourceManager* rm);
void draw_ball(const BallInfo* ballInfo, double alpha, ResourceManager* rm);
int clean_ball();

#endif /* BALL_H */
