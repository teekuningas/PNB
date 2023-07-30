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

static LocalGameInfo localGameInfo;
static GlobalGameInfo globalGameInfo;

int initMutableWorld(StateInfo* stateInfo)
{
	int result;

	stateInfo->localGameInfo = &localGameInfo;
	stateInfo->globalGameInfo = &globalGameInfo;

	result = initPlayer(stateInfo);
	if(result != 0) {
		printf("Could not init player. Exiting.");
		return -1;
	}


	result = initBall();
	if(result != 0) {
		printf("Could not init ball. Exiting.");
		return -1;
	}

	initActionImplementation(stateInfo);
	initActionInvocations(stateInfo);
	initGameAnalysis(stateInfo);

	return 0;
}

void updateMutableWorld(StateInfo* stateInfo)
{
	if(stateInfo->localGameInfo->gAI.pause == 0) {
		gameAnalysis(stateInfo);
		actionInvocations(stateInfo);
		actionImplementation(stateInfo);
		gameManipulation(stateInfo);
	}
}
void drawMutableWorld(StateInfo* stateInfo, double alpha)
{
	// players and ball are the building blocks of all the action on the screen.
	if(stateInfo->localGameInfo->gAI.pause == 0) {
		drawPlayer(stateInfo, localGameInfo.playerInfo, alpha);
		drawBall(&(localGameInfo.ballInfo), alpha);
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
	result = cleanPlayer(stateInfo);
	if(result != 0) {
		printf("Could not clean player properly.\n");
		return -1;
	}
	return 0;
}
