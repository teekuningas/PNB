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
		         game->playerInfo[get_base_controller(game, BASE_THIRD)].bTPI.state == PLAYER_STATE_SAFE_ON_BASE)) {
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
			// In checkForOuts: "runToNextBase(..., index, (BaseID)i)" where i was the loop index of bases.
			// Referee logic: safetyToRemove = ballAtBase.
			// So we use safetyToRemove as the base we are running FROM (or associated with).
			// runToNextBase takes the player index and the base they are advancing FROM.
			runToNextBase(game, stateInfo->fieldPositions, i, pd->advanceTarget);
		}

		// B. Remove Safety
		if (pd->removeSafety) {
			game->referee.battingPlayers[i].currentSafetyBase = BASE_NONE;

			// If batter, lose batter status
			if (game->pII.batterIndex == i) {
				game->pII.batterIndex = -1;
			}
		}

		// B2. Change Wounding Type
		if (pd->changeWoundingType) {
			game->referee.battingPlayers[i].woundingType = pd->newWoundingType;
		}

		// C. Apply OUT
		if (pd->isOut) {
			game->playerInfo[i].bTPI.state = PLAYER_STATE_OUT;
			game->gameState.outs += 1;
			game->playerInfo[i].bTPI.baseId = BASE_NONE;
			game->playerCounters.battingTeamPlayersOnFieldCount--;

			movePlayerOut(game->playerInfo, game->playerRuntime, stateInfo->fieldPositions, i);
			
			// Critical Fix: Clear baseAtPitchStart so this player isn't resurrected by foulPlay
			game->referee.battingPlayers[i].baseAtPitchStart = BASE_NONE;

			// Ensure safety is removed (redundant but safe)
			// Referee probably set removeSafety=1 too, but let's be sure.
			// Actually Referee_Analyze sets removeSafety if Out.
		}

		// D. Apply RUN
		if (pd->isRun) {
			// Update flags
			if (game->playerInfo[i].bTPI.baseId == BASE_THIRD) {
				game->playerRuntime[i].hasMadeRunOnThirdBase = 1;
			}
			
			stateInfo->globalGameInfo->teams[battingTeamIndex].runs += 1;
			game->gameState.runsInTheInning += 1;
			
			if (game->gameState.runsInTheInning % 2 == 0) {
				game->playerCounters.nonJokerPlayersLeft = PLAYERS_IN_TEAM;
				if (game->playerInfo[i].bTPI.baseId == BASE_NONE) { // If home run (no base)? No wait.
					// If baseId became NONE (removed), then noMorePlayers = 0.
					// If baseId is still BASE_THIRD (Run of Honor), we don't reset noMorePlayers?
					// Legacy: if(playerInfo[index].bTPI.baseId == BASE_NONE) noMorePlayers = 0;
				}
			}
			
			// Cleanup player if they scored normally (at Home Scored)
			if (game->playerInfo[i].bTPI.baseId == BASE_HOME_SCORED) {
				game->playerCounters.battingTeamPlayersOnFieldCount--;
				game->playerInfo[i].bTPI.baseId = BASE_NONE;
				
				// Critical Fix: Clear baseAtPitchStart for scored players too
				game->referee.battingPlayers[i].baseAtPitchStart = BASE_NONE;

				if (game->gameState.runsInTheInning % 2 == 0) {
					game->playerCounters.noMorePlayers = 0;
				}
				// Remove 3rd base safety (Legacy: if(baseControlIndex[3] == index) ...)
				if (get_base_controller(game, BASE_THIRD) == i) {
				}
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
		        game->gameFlowState.woundingCatchCounter == -1 &&
		        game->gameFlowState.endOfInningCounter == -1 &&
		        game->gameState.outOfBounds == 0) {
			// The conditions for run checking were met, so we assume it was checked.
			game->gameControl.checkForRun = 0;
		}
	}
}
