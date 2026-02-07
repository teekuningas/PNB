/*
	this section is going to be the core part of non-platfrom specific code and going to handle
	things happening behind of what is seen on the screen.
*/

#include "globals.h"
#include "ball.h"
#include "player.h"
#include "action_implementation.h"
#include "action_invocations.h"
#include "game_consolidation.h"
#include "game_manipulation.h"

#include "mutable_world.h"
#include "common_logic.h"
#include "game_setup.h"
#include "../renderer/player_renderer.h" // Include player_renderer.h
#include "state_validator.h"
#include "referee.h"
#include "rules_pure/player_utils.h"

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

	// Consolidated Init (Game Flow + Reset Logic)
	GameConsolidation_Init(&(stateInfo->match->gameFlowState));

	initGameManipulation(&(stateInfo->match->gameFlowState));

	return 0;
}

void updateMutableWorld(StateInfo* stateInfo, MenuInfo* menuInfo, unsigned int* rng_seed)
{
	if(stateInfo->match->flowControl.pause == 0) {
		// 1. Inputs
		actionInvocations(stateInfo);

		// 2. Physics & Logic
		actionImplementation(stateInfo, rng_seed);
		gameManipulation(stateInfo);

		// 3. Referee (Legal State Authority)
		// Runs AFTER physics to ensure legal state matches physical events
		MatchSession* game = stateInfo->match;
		update_referee(
		    stateInfo,
		    &game->referee,
		    &game->halfInningState,
		    &game->betweenPitchState,
		    &game->playerCounters,
		    &stateInfo->match->scoreboard
		);

		// 4. Consolidation (Reaction Phase)
		// - Updates Game Flow (innings, user prompts)
		// - Handles Physical Resets (Foul Play)
		// - Enforces Legal State (Outs, Scoring)
		GameConsolidation_Update(stateInfo, menuInfo, rng_seed);

		// 5. Capture snapshot after all updates when pitch is released
		if (stateInfo->match->gameEvents.pitchReleased) {
			StateValidator_CaptureSnapshot(stateInfo, "PITCH_START");
		}

		// Validate state consistency (Debug only)
		if (!StateValidator_Check(stateInfo)) {
			StateValidator_Dump(stateInfo, "State Consistency Check Failed");
			stateInfo->match->flowControl.pause = 1;
		}

		// Clear transient events for the next frame
		clearFrameEvents(&stateInfo->match->gameEvents);
	}
}
void drawMutableWorld(const StateInfo* stateInfo, double alpha, ResourceManager* rm)
{
	// players and ball are the building blocks of all the action on the screen.
	if(stateInfo->match->flowControl.pause == 0) {
#ifndef NO_RENDER
		drawPlayerRenderer(stateInfo, stateInfo->match->playerInfo, alpha, rm);
		drawBall(&(stateInfo->match->ballInfo), alpha, rm);
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
