#ifndef GAME_SCREEN_H
#define GAME_SCREEN_H

#include "globals.h"

int initGameScreen(StateInfo* stateInfo);
void updateGameScreen(StateInfo* stateInfo);
void drawGameScreen(StateInfo* stateInfo, double alpha);
int cleanGameScreen(StateInfo* stateInfo);

#endif /* GAME_SCREEN_H */
