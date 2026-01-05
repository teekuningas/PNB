#ifndef BALL_H
#define BALL_H

#include "globals.h"
#include "resource_manager.h"

int initBall(ResourceManager* rm);
void drawBall(const BallInfo* ballInfo, double alpha, ResourceManager* rm);
int cleanBall();

#endif /* BALL_H */
