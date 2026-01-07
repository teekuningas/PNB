#include <string.h>
#include "referee.h"
#include "rules_outs.h"
#include "rules_runs.h"
#include "base_logic.h"
#include "geometry.h"
#include "vector_math.h"

#define BASE_RADIUS 2.0f
#define HOME_RADIUS 6.0f
#define HOME_LINE_Z -0.65f

// Helper to analyze ball position
static int get_ball_at_base_index(const StateInfo* stateInfo)
{
	if (stateInfo->localGameInfo->pII.hasBallIndex == -1) {
		return -1; // No one has ball
	}

	// Use catcher position directly or ball position?
	// Legacy logic uses ballInfo.location, assuming ball is with player.
	Vector3D ballLoc = stateInfo->localGameInfo->ballInfo.location;

	// Check Home Base (Index 0)
	// Special check: inside homeline-middlepoint centered disk and at homebase side
	float dx = ballLoc.x - stateInfo->fieldPositions->pitchPlate.x;
	float dz = ballLoc.z - HOME_LINE_Z;
	if (ballLoc.z > HOME_LINE_Z && vec3_is_small_enough_circle_xz(dx, dz, HOME_RADIUS)) {
		return 0; // Home Base
	}

	// Check Bases 1-3
	for (int i = 1; i < BASE_COUNT; i++) {
		Vector3D baseLoc;
		if (i == 1) baseLoc = stateInfo->fieldPositions->firstBase;
		else if (i == 2) baseLoc = stateInfo->fieldPositions->secondBase;
		else if (i == 3) baseLoc = stateInfo->fieldPositions->thirdBase;
		else continue;

		dx = ballLoc.x - baseLoc.x;
		dz = ballLoc.z - baseLoc.z;

		if (vec3_is_small_enough_circle_xz(dx, dz, BASE_RADIUS)) {
			return i;
		}
	}

	return -1;
}

RefereeDecisions Referee_Analyze(const StateInfo* stateInfo)
{
	RefereeDecisions decisions;
	memset(&decisions, 0, sizeof(RefereeDecisions));
	decisions.canMakeRunOfHonor = 1; // Default to true (don't revoke)

	const LocalGameInfo* game = stateInfo->localGameInfo;

	// 1. Where is the ball?
	int ballAtBase = get_ball_at_base_index(stateInfo);

	// 2. Ball Home Logic (affects camera/game flow)
	if (ballAtBase == 0) {
		decisions.ballHome = 1;
	}

	// 3. Check for Outs (§33) and Tuplahaava Exceptions (§36)
	if (ballAtBase != -1) {
		// Default assumption: If ball is at a base, Run of Honor possibility is threatened.
		// We set it to 0 here, and if we find a valid candidate below, we set it back to 1.
		// If ballAtBase == -1, this logic doesn't run, preserving the flag.
		decisions.canMakeRunOfHonor = 0;

		for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
			const PlayerInfo* player = &game->playerInfo[i];

			// Only check active players
			if (player->bTPI.baseId == BASE_NONE) continue;

			// Check Run of Honor preservation
			// "if the batter manages to get over second base he still has chance... with exception of third base"
			if (base_is_at_least(player->bTPI.baseId, BASE_SECOND) &&
			        game->referee.battingPlayers[i].baseAtPitchStart == BASE_HOME &&
			        ballAtBase != 3) { // i != 3 in legacy loop means Base 3
				decisions.canMakeRunOfHonor = 1;
			}

			// --- Logic from checkForOuts ---

			// Determine "Target Base" for Force Out logic
			// In Pesäpallo, if ball is at Base 2, we check if runner coming FROM Base 1 is out.
			// The `is_runner_forced_out` function expects `ball_at_base_index` to match the runner's FROM base.
			// Wait, let's re-read `rules_outs.c`:
			// "The third parameter (ball_at_base_index) represents the player's FROM base."
			// "Force out condition: if (player_base == ball_at_base_index) return 1;"
			// So if Ball is at Base 2 (index 2), we want to check players at Base 1.
			// BUT `checkForOuts` iterates `i` (bases) and checks players.
			// It calculates `baseIndexId` as `i - 1`.
			// And passes `baseIndexId` as the 3rd param to `is_runner_forced_out`.

			// So:
			// Ball at Base 2 (i=2).
			// baseIndexId = 1.
			// is_runner_forced_out(player_base, ..., baseIndexId=1, ...)
			// If player is at Base 1 -> OUT. Correct.

			BaseID ballBaseId = (BaseID)ballAtBase;
			BaseID checkBaseId;

			if (ballBaseId == BASE_HOME) {
				checkBaseId = BASE_THIRD; // Home base forces runner from 3rd
			} else {
				checkBaseId = (BaseID)(ballAtBase - 1);
			}

			// A. Force Out / Burning (§33)
			// Check if player is currently at `checkBaseId`
			// Note: `base_to_int_index` might be needed if baseId is not just int

			int is_actually_safe = 0;
			int base_idx = base_to_int_index(player->bTPI.baseId);
			if (base_idx != -1 && game->pII.baseControlIndex[base_idx] == i && player_is_protected(player->bTPI.state)) {
				is_actually_safe = 1;
			}

			if (is_runner_forced_out(
			            player->bTPI.baseId,
			            is_actually_safe,
			            checkBaseId,
			            player->bTPI.state == PLAYER_STATE_ADVANCING_FREELY,
			            &game->gameState
			        )) {

				decisions.playerDecisions[i].isOut = 1;
				decisions.eventOut = 1;

				// Side effect: Remove safety from previous base
				if (base_idx != -1 && game->pII.baseControlIndex[base_idx] == i) {
					decisions.playerDecisions[i].removeSafety = 1;
					decisions.playerDecisions[i].safetyToRemove = (BaseID)base_idx;
				}
			}

			// B. Safety Removal (Koppilyönti §36 logic inside checkForOuts)
			// "Mikäli sisäpelaaja on irti pesästä kopinottohetkellä... hänet voidaan polttaa."
			// If ball arrives at NEXT base (ballAtBase) while player is running to it?
			// `checkForOuts` has logic:
			// "remove safety from last base... happens if player is out of base and ball arrives the previous one."
			// Wait, let's look at `checkForOuts` again.
			/*
				if(stateInfo->localGameInfo->pII.baseControlIndex[i] == index) {
					if((stateInfo->localGameInfo->playerInfo[index].bTPI.state != PLAYER_STATE_SAFE_ON_BASE) &&
							(stateInfo->localGameInfo->playerInfo[index].bTPI.state != PLAYER_STATE_AT_BAT)) {
						// runToNextBase...
						// remove safety
					}
				}
			*/
			// Loop `i` there is the base index. `pII.baseControlIndex[i] == index`.
			// So if ball is at Base `i`, and Player `index` holds safety for Base `i`,
			// BUT player is NOT physically safe (LEADING/RUNNING)...
			// Then player loses safety at Base `i` and is forced to run to `i+1`.

			int safetyIndex = -1;
			// Find which base this player holds safety for (if any)
			for (int b = 0; b < BASE_COUNT; b++) {
				if (game->pII.baseControlIndex[b] == i) {
					safetyIndex = b;
					break;
				}
			}

			// If ball is at the base the player holds safety for
			if (safetyIndex == ballAtBase) {
				if (!player_is_protected(player->bTPI.state)) {
					// Player is "irti" (Leading/Running) from this base, and ball arrived there.
					// They lose safety and must advance.
					decisions.playerDecisions[i].removeSafety = 1;
					decisions.playerDecisions[i].safetyToRemove = (BaseID)ballAtBase;

					// Force advance
					decisions.playerDecisions[i].shouldAdvance = 1;
					decisions.playerDecisions[i].advanceTarget = (BaseID)ballAtBase; // logic uses 'i' in checkForOuts, assuming it advances FROM i?
					// `runToNextBase(..., index, (BaseID)i)`
					// This sets target to `i+1`.
				}
			}

			// C. Tuplahaava Exceptions (Explicit Logic from game_analysis.c)
			if (game->referee.battingPlayers[i].hasPendingWound &&
			        game->referee.battingPlayers[i].woundingType == WOUNDING_TYPE_TUPLAHAAVA &&
			        !decisions.playerDecisions[i].isOut) { // Don't check if already out

				BaseID source = game->referee.battingPlayers[i].woundingSourceBase;
				BaseID next = base_get_next(source);
				int is_in_between = (player->bTPI.state != PLAYER_STATE_SAFE_ON_BASE);

				if (is_in_between) {
					// Exception 2: Ball at NEXT base -> OUT
					if (base_to_int_index(next) == ballAtBase) {
						decisions.playerDecisions[i].isOut = 1;
						decisions.eventOut = 1;
						// Clear pending logic
						decisions.playerDecisions[i].removeSafety = 1; // Clear source safety
						decisions.playerDecisions[i].safetyToRemove = source;
					}
					// Exception 1: Ball at SOURCE base -> Lose Safety
					else if (base_to_int_index(source) == ballAtBase) {
						decisions.playerDecisions[i].removeSafety = 1;
						decisions.playerDecisions[i].safetyToRemove = source;
						// "Transition to NORMAL wounding type"
						decisions.playerDecisions[i].changeWoundingType = 1;
						decisions.playerDecisions[i].newWoundingType = WOUNDING_TYPE_NORMAL;
					}
				}
			}
		}
	}

	// 4. Check for Runs (§41/42)
	// Runs are processed only when play "settles" (ball caught or ground) and not during wounding check.
	// Logic copied from legacy checkForRuns.
	if (game->gameControl.checkForRun == 1) {
		if ((game->gameControl.firstCatchMade == 1 || game->ballInfo.hasHitGround == 1) &&
		        game->gameFlowState.woundingCatchCounter == -1 &&
		        game->gameFlowState.endOfInningCounter == -1 &&
		        game->gameState.outOfBounds == 0) {

			// Check all players
			for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
				if (game->playerInfo[i].bTPI.baseId != BASE_NONE) {
					int runScored = calculate_runs(
					                    game->playerInfo[i].bTPI.baseId,
					                    game->referee.battingPlayers[i].baseAtPitchStart,
					                    game->playerInfo[i].bTPI.state == PLAYER_STATE_WOUNDED,
					                    &game->gameModeState,
					                    game->playerRuntime[i].hasMadeRunOnThirdBase
					                );

					if (runScored) {
						decisions.playerDecisions[i].isRun = 1;
						decisions.eventRun = 1;

						// §42 Overtaking Logic (Check if Kunniajuoksu overtakes someone)
						// If base is 3rd (Run of Honor) and 3rd is occupied by someone else...
						if (game->playerInfo[i].bTPI.baseId == BASE_THIRD) {
							if (game->pII.baseControlIndex[3] != -1 &&
							        game->pII.baseControlIndex[3] != i) {
								// Overtaking! The RUNNER (i) is removed/out?
								// Logic in legacy: "kunniajuoksun tehnyt pelaaja... siirtyy kotipuolelle"
								// Legacy code removed player 'i'.
								decisions.playerDecisions[i].isOut = 1; // Mark as "Out" to remove from field?
								// Or effectively just remove. Legacy used movePlayerOut.
								// isOut triggers removal and movePlayerOut.
							}
						}
					}

					// If at HOME_SCORED, must remove even if not run (already handled?)
					// Legacy: "if baseId == BASE_HOME_SCORED... remove player".
					// This logic is intertwined with calculate_runs return.
					// If baseId is HOME_SCORED, calculate_runs returns 1.
					// So isRun=1. Apply handles removal.
				}
			}

			// Period End Check (Mercy Rule / Winning Run)
			// Need to calculate hypothetical runs to see if period ends?
			// This is complex because it depends on HOW MANY runs are scored this frame.
			// Legacy logic did it inside the loop.
			// We can replicate that logic in Apply, or simulate it here.
			// Ideally, Analyze determines "IsPeriodEnd".
			// Let's count runs.
			int runsScoredCount = 0;
			for(int i=0; i<PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
				if (decisions.playerDecisions[i].isRun) runsScoredCount++;
			}

			if (runsScoredCount > 0) {
				int battingTeamIndex = (stateInfo->globalGameInfo->inning + stateInfo->globalGameInfo->playsFirst + stateInfo->globalGameInfo->period) % 2;
				int catchingTeamIndex = (battingTeamIndex + 1) % 2;
				int currentRuns = stateInfo->globalGameInfo->teams[battingTeamIndex].runs + runsScoredCount;
				int opponentRuns = stateInfo->globalGameInfo->teams[catchingTeamIndex].runs;

				// Logic for period ending
				if (stateInfo->globalGameInfo->period < 4) {
					if ((stateInfo->globalGameInfo->inning + 1) % stateInfo->globalGameInfo->halfInningsInPeriod == 0 ||
					        stateInfo->globalGameInfo->inning + 1 == stateInfo->globalGameInfo->halfInningsInPeriod * 2 + 2) {
						if (currentRuns > opponentRuns) {
							decisions.isPeriodEnd = 1;
						}
						// Super inning specific check
						if (stateInfo->globalGameInfo->inning + 1 == stateInfo->globalGameInfo->halfInningsInPeriod * 2 &&
						        stateInfo->globalGameInfo->teams[battingTeamIndex].period0Runs > stateInfo->globalGameInfo->teams[catchingTeamIndex].period0Runs &&
						        opponentRuns == currentRuns) {
							decisions.isPeriodEnd = 1;
						}
					}
				} else {
					if ((stateInfo->globalGameInfo->inning + 1) % 2 == 0) {
						if (currentRuns > opponentRuns) {
							decisions.isPeriodEnd = 1;
						}
					}
				}
			}
		}
	}

	return decisions;
}
