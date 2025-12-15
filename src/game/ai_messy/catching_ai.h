#ifndef CATCHING_AI_H
#define CATCHING_AI_H

#include "globals.h"
#include "actions_messy/action_state.h"

#define TIMEOUT_CONSTANT 200

extern int aiMoveCounter;
extern int aiThrowStage;

void initCatchingAI(void);
void updateCatchingAI(StateInfo* stateInfo);
void moveControlledPlayerToLocation(StateInfo* stateInfo, Vector3D* target);
void throwBallToBase(StateInfo* stateInfo, int base);

#endif
