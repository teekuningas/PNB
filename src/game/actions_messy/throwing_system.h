#ifndef THROWING_SYSTEM_H
#define THROWING_SYSTEM_H

#include "globals.h"

#define THROW_MAX 50

void initThrowingSystem(StateInfo* stateInfo);

void prepareThrow(StateInfo* stateInfo, int base);

void genericThrowRelease(StateInfo* stateInfo);
void genericThrowLoad(StateInfo* stateInfo, int base);
void genericMove(StateInfo* stateInfo, int direction);
void genericStopMove(StateInfo* stateInfo, int direction);
void dropBall(StateInfo* stateInfo);
void updateControlledPlayerSpeed(StateInfo* stateInfo);

#endif
