/*
	this section is going to be the core part of non-platfrom specific code and going to handle
	things happening behind of what is seen on the screen.
*/

#include "globals.h"
#include "ball.h"
#include "player.h"
#include "action_implementation.h"
#include "action_invocations.h"
#include "game_analysis.h"
#include "game_manipulation.h"

#include "mutable_world.h"
#include "common_logic.h"
#include "../renderer/player_renderer.h" // Include player_renderer.h

int initMutableWorld(StateInfo* stateInfo, ResourceManager* rm)
{
	int result;

	// The initPlayer is now handled by initPlayerRenderer, which is called by initPlayer in player.c
	// No direct call to initPlayerRenderer here.
	result = initPlayer(stateInfo, rm);
	if(result != 0) {
		printf("Could not init player. Exiting.");
		return -1;
	}


	result = initBall(rm);
	if(result != 0) {
		printf("Could not init ball. Exiting.");
		return -1;
	}

	initActionImplementation(stateInfo);
	initActionInvocations(stateInfo);
	initGameAnalysis(&(stateInfo->localGameInfo->gameFlowState));

	return 0;
}

void updateMutableWorld(StateInfo* stateInfo, MenuInfo* menuInfo, unsigned int* rng_seed)
{
	if(stateInfo->localGameInfo->gameControl.pause == 0) {
		gameAnalysis(stateInfo, menuInfo, rng_seed);
		actionInvocations(stateInfo);
		actionImplementation(stateInfo, rng_seed);
		gameManipulation(stateInfo);
	}
}
void drawMutableWorld(const StateInfo* stateInfo, double alpha, ResourceManager* rm)
{
	// players and ball are the building blocks of all the action on the screen.
	if(stateInfo->localGameInfo->gameControl.pause == 0) {
#ifndef NO_RENDER
		drawPlayerRenderer(stateInfo, stateInfo->localGameInfo->playerInfo, alpha, rm);
		drawBall(&(stateInfo->localGameInfo->ballInfo), alpha, rm);
#endif
	}
}
int cleanMutableWorld(StateInfo* stateInfo)
{
	int result;
	result = cleanBall();
	if(result != 0) {
		printf("Could not clean ball properly.\n");
		return -1;
	}
#ifndef NO_RENDER
	result = cleanPlayerRenderer();
	if(result != 0) {
		printf("Could not clean player properly.\n");
		return -1;
	}
#endif
	return 0;
}
