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
	initGameAnalysis(&(stateInfo->match->gameFlowState));
	initGameManipulation(&(stateInfo->match->gameFlowState));

	return 0;
}

void reconcileLegalAndPhysicalState(StateInfo* stateInfo)
{
	MatchSession* game = stateInfo->match;
	for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		// 1. React to OUT
		if (game->referee.battingPlayers[i].isOut) {
			if (game->playerInfo[i].bTPI.state != PLAYER_STATE_OUT) {
				game->playerInfo[i].bTPI.state = PLAYER_STATE_OUT;
				game->playerInfo[i].bTPI.baseId = BASE_NONE;
				movePlayerOut(game->playerInfo, game->playerRuntime, stateInfo->fieldPositions, i);
			} else {
			}
		}

		// 2. React to SCORE
		if (game->referee.battingPlayers[i].hasScored &&game->playerInfo[i].bTPI.state != PLAYER_STATE_SCORED) {
			game->playerInfo[i].bTPI.state = PLAYER_STATE_SCORED;
			game->playerInfo[i].bTPI.baseId = BASE_NONE;
			movePlayerOut(game->playerInfo, game->playerRuntime, stateInfo->fieldPositions, i);
		}

		// 3. React to displacement (Panic Run)
		if (game->playerInfo[i].bTPI.state == PLAYER_STATE_ON_BASE || game->playerInfo[i].bTPI.state == PLAYER_STATE_LEADING) {
			BaseID physBase = game->playerInfo[i].bTPI.baseId;
			if (game->referee.battingPlayers[i].currentSafetyBase != physBase) {
				// Player is physically at base but legally has no safety there.
				// They must run forward.
				runToNextBase(game, stateInfo->fieldPositions, i, physBase);
			}
		}
	}

	// 4. React to Pitch Resolution (Milestone 17)
	// If the Referee has adjudicated the pitch (Strike/Ball), we must close the pitch state
	// to prevent double-counting and to signal the action system that the pitch is over.
	if (game->gameControl.pitchResolutionProcessed) {
		game->pRAI.pitchState = PITCH_STAGE_NONE;
		game->gameControl.pitchResolutionProcessed = 0; // Consume the flag
	}
}

void updateMutableWorld(StateInfo* stateInfo, MenuInfo* menuInfo, unsigned int* rng_seed)
{
	if(stateInfo->match->gameControl.pause == 0) {
		gameAnalysis(stateInfo, menuInfo, rng_seed);
		actionInvocations(stateInfo);
		actionImplementation(stateInfo, rng_seed);
		gameManipulation(stateInfo);

		// Referee logic now runs AFTER physics/manipulation to ensure legal state matches physical state
		MatchSession* game = stateInfo->match;
		Referee_Update(
		    stateInfo,
		    &game->referee,
		    &game->halfInningState,
		    &game->gameControl,
		    &game->playerCounters,
		    &stateInfo->match->scoreboard
		);

		// Handle Foul Play Reset (Milestone 17)
		// Timer logic: Count frames when outOfBounds is active
		static int outOfBoundsTimer = 0;
		if (game->gameControl.outOfBounds) {
			outOfBoundsTimer++;

			// After ~2 seconds (OUT_OF_BOUNDS_THRESHOLD frames), trigger reset
			if (outOfBoundsTimer > (int)(2.0f * (1 / (UPDATE_INTERVAL*1.0f/1000)))) {
				applyFoulPlayReset(stateInfo, rng_seed);
				// Clear state to prevent loop
				game->gameControl.outOfBounds = 0;
				outOfBoundsTimer = 0;
			}
		} else {
			outOfBoundsTimer = 0;
		}

		// React to the new legal state (e.g. panic run if safety lost)
		reconcileLegalAndPhysicalState(stateInfo);

		// Validate state consistency (Debug only)
		if (!StateValidator_Check(stateInfo)) {
			StateValidator_Dump(stateInfo, "State Consistency Check Failed");
			stateInfo->match->gameControl.pause = 1;
		}

		// Clear transient events for the next frame
		clearFrameEvents(&stateInfo->match->gameEvents);
	}
}
void drawMutableWorld(const StateInfo* stateInfo, double alpha, ResourceManager* rm)
{
	// players and ball are the building blocks of all the action on the screen.
	if(stateInfo->match->gameControl.pause == 0) {
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
