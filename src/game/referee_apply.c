#include <string.h>
#include "game_analysis.h"
#include "referee.h"
#include "base_logic.h"
#include "common_logic.h" // For movePlayerOut, runToNextBase
#include "base_control.h"

#include "rules_runs.h"

// The "Write" half of the decoupling
void Referee_Apply(StateInfo* stateInfo, const RefereeDecisions* decisions)
{
	LocalGameInfo* game = stateInfo->localGameInfo;

	// 1. Apply Global Flags
	if (decisions->ballHome) {
		game->gameState.ballHome = 1;

		// Camera logic from checkForOuts
		if (game->gameFlowState.homeRunCameraCounter == -1 &&
		        game->cameraState.homeRunCameraFlag == 1 &&
		        (get_base_controller(game, BASE_THIRD) == -1 ||
		         game->playerInfo[get_base_controller(game, BASE_THIRD)].bTPI.state == PLAYER_STATE_ON_BASE)) {
			game->gameFlowState.homeRunCameraCounter = 0;
		}
	}

	if (decisions->eventOut) {
		game->gameState.event = EVENT_OUT;
	}

	if (decisions->eventRun) {
		game->gameState.event = EVENT_RUN_SCORED;
	}

	if (decisions->isPeriodEnd) {
		game->gameState.endPeriod = 1;
	}

	// 2. Apply Run of Honor Logic
	// If Referee says it's 0 (revoked), we update the state.
	// If 1, we leave it (it might already be 0, or 1).
	if (decisions->canMakeRunOfHonor == 0) {
		game->gameModeState.canMakeRunOfHonor = 0;
	}

	// 3. Apply Player Decisions
	int battingTeamIndex = (stateInfo->globalGameInfo->inning + stateInfo->globalGameInfo->playsFirst + stateInfo->globalGameInfo->period) % 2;

	for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		const PlayerDecision* pd = &decisions->playerDecisions[i];

		// A. Forced Advance ("Panic Run")
		if (pd->shouldAdvance) {
			// Referee no longer forces movement.
			// gameManipulation will detect lack of safety and trigger run.
		}

		// B. Remove Safety
		if (pd->removeSafety) {
			game->referee.battingPlayers[i].currentSafetyBase = BASE_NONE;

			// If batter, lose batter status
			if (game->pII.batterIndex == i) {
				game->pII.batterIndex = -1;
			}
		}

		// B1. Grant Safety
		if (pd->grantSafety) {
			game->referee.battingPlayers[i].currentSafetyBase = pd->safetyToGrant;
		}

		// B2. Change Wounding Type
		if (pd->changeWoundingType) {
			game->referee.battingPlayers[i].woundingType = pd->newWoundingType;
		}

		// C. Apply OUT
		if (pd->isOut) {
			game->referee.battingPlayers[i].isOut = 1;
			game->gameState.outs += 1;

			// Critical Fix: Clear baseAtPitchStart so this player isn't resurrected by foulPlay
			game->referee.battingPlayers[i].baseAtPitchStart = BASE_NONE;
		}

		// D. Apply RUN
		if (pd->isRun) {
			stateInfo->globalGameInfo->teams[battingTeamIndex].runs += 1;
			game->gameState.runsInTheInning += 1;

			game->referee.battingPlayers[i].hasScored = 1;
			game->referee.battingPlayers[i].baseAtPitchStart = BASE_NONE;

			if (game->gameState.runsInTheInning % 2 == 0) {
				game->playerCounters.nonJokerPlayersLeft = PLAYERS_IN_TEAM;
			}
		}
	}

	// Reset checkForRun only if we actually processed the check
	// Referee_Analyze only returns decisions if check was valid.
	// But how do we know if Referee analyzed runs?
	// We can check if any decision was made? No.
	// We should replicate the condition:
	if (game->gameControl.checkForRun == 1) {
		if ((game->gameControl.firstCatchMade == 1 || game->ballInfo.hasHitGround == 1) &&
		        game->referee.woundingCatchTimer == -1 &&
		        game->gameFlowState.endOfInningCounter == -1 &&
		        game->gameState.outOfBounds == 0) {
			// The conditions for run checking were met, so we assume it was checked.
			game->gameControl.checkForRun = 0;
		}
	}
}

void Referee_Execute(StateInfo* stateInfo)
{
	// Phase 1: Pure Analysis (Read-Only)
	RefereeDecisions decisions = Referee_Analyze(stateInfo);

	// Phase 2: State Mutation (Write)
	Referee_Apply(stateInfo, &decisions);
}
