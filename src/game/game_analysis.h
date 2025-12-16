#ifndef GAME_ANALYSIS_H
#define GAME_ANALYSIS_H

#include "globals.h"
#include "menu_types.h"

void gameAnalysis(StateInfo* stateInfo, MenuInfo* menuInfo, unsigned int* rng_seed);
void initGameAnalysis(StateInfo* stateInfo);

#endif /* GAME_ANALYSIS_H */
