#include "globals.h"
#include "ball.h"
#include "player.h"
#include "action_implementation.h"
#include "action_invocations.h"
#include "game_analysis.h"
#include "game_manipulation.h"

#include "mutable_world.h"
#include "mutable_world_internal.h"
#include "common_logic.h"

/*
	this section is going to be the core part of non-platfrom specific code and going to handle
	things happening behind of what is seen on the screen.
*/

int initMutableWorld()
{
	int result;

	stateInfo.localGameInfo = &localGameInfo;
	stateInfo.globalGameInfo = &globalGameInfo;

	result = initPlayer();
	if(result != 0) {
		printf("Could not init player. Exiting.");
		return -1;
	}


	result = initBall();
	if(result != 0) {
		printf("Could not init ball. Exiting.");
		return -1;
	}

	initActionImplementation();
	initActionInvocations();
	initGameAnalysis();

	return 0;
}

void updateMutableWorld()
{
	if(stateInfo.localGameInfo->gAI.pause == 0) {
		gameAnalysis();
		actionInvocations();
		actionImplementation();
		gameManipulation();
	}
}
void drawMutableWorld(double alpha)
{
	// players and ball are the building blocks of all the action on the screen.
	if(stateInfo.localGameInfo->gAI.pause == 0) {
		drawPlayer(alpha, localGameInfo.playerInfo);
		drawBall(alpha, &(localGameInfo.ballInfo));
	}
}
int cleanMutableWorld()
{
	int result;
	result = cleanBall();
	if(result != 0) {
		printf("Could not clean ball properly.\n");
		return -1;
	}
	result = cleanPlayer();
	if(result != 0) {
		printf("Could not clean player properly.\n");
		return -1;
	}
	return 0;
}
