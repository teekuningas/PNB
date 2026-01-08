#include <string.h>
#include "referee.h"
#include "rules_outs.h"
#include "rules_runs.h"
#include "base_logic.h"
#include "geometry.h"
#include "vector_math.h"
#include "base_control.h"

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
			// §33: Runner is OUT if they have pesäturva (safety) at a base,
			// are "irti" (not protected), and ball reaches the next base.

			// Does this player have safety at their current physical base?
			int has_safety_at_current = (game->referee.battingPlayers[i].currentSafetyBase == player->bTPI.baseId);

			// Is player protected (physically safe on base)?
			int is_protected = player_is_protected(player->bTPI.state);

			// §33 Pesäkilpa: Protected from force out if they have safety AND are physically safe
			int is_safe_from_force_out = has_safety_at_current && is_protected;

			if (is_runner_forced_out(
			            player->bTPI.baseId,
			            is_safe_from_force_out,
			            checkBaseId,
			            player->bTPI.state == PLAYER_STATE_ADVANCING_FREELY,
			            &game->gameState
			        )) {

				decisions.playerDecisions[i].isOut = 1;
				decisions.eventOut = 1;

				// Remove safety if they had it at this base
				if (has_safety_at_current) {
					decisions.playerDecisions[i].removeSafety = 1;
					decisions.playerDecisions[i].safetyToRemove = player->bTPI.baseId;
				}
			}

			// B. Safety Removal (§36 Koppilyönti logic)
			// If ball arrives at a base where player has safety, but player is "irti",
			// they lose their pesäturva and must advance.

			BaseID player_safety_base = game->referee.battingPlayers[i].currentSafetyBase;

			if (player_safety_base != BASE_NONE && player_safety_base == (BaseID)ballAtBase) {
				if (!is_protected) {
					// Player has safety here but is "irti" - lose safety and must run
					decisions.playerDecisions[i].removeSafety = 1;
					decisions.playerDecisions[i].safetyToRemove = (BaseID)ballAtBase;

					// Force advance
					decisions.playerDecisions[i].shouldAdvance = 1;
					decisions.playerDecisions[i].advanceTarget = (BaseID)ballAtBase;
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
		        game->referee.woundingCatchTimer == -1 &&
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
						// Check if another player has safety at BASE_THIRD
						if (game->playerInfo[i].bTPI.baseId == BASE_THIRD) {
							// Look through all players to see if someone else has safety there
							int someone_else_has_third_safety = 0;
							for (int j = 0; j < PLAYERS_IN_TEAM + JOKER_COUNT; j++) {
								if (j != i && game->referee.battingPlayers[j].currentSafetyBase == BASE_THIRD) {
									someone_else_has_third_safety = 1;
									break;
								}
							}

							if (someone_else_has_third_safety) {
								// Overtaking! The runner (i) is OUT
								decisions.playerDecisions[i].isOut = 1;
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

// Query functions for wounding system (Milestone 15 consolidation)

int is_wounding_catch_pending(const RefereeState* ref)
{
	return ref->woundingCatchPending;
}

int is_wounding_catch_handled(const RefereeState* ref)
{
	return ref->woundingCatchHandled;
}

int get_wounding_timer(const RefereeState* ref)
{
	return ref->woundingCatchTimer;
}

int is_player_marked_for_wound(const RefereeState* ref, int playerIndex)
{
	if (playerIndex < 0 || playerIndex >= PLAYERS_IN_TEAM + JOKER_COUNT) {
		return 0;
	}
	return ref->woundingPlayersMarked[playerIndex];
}