#include <string.h>
#include "referee.h"
#include "rules_outs.h"
#include "rules_runs.h"
#include "base_logic.h"
#include "geometry.h"
#include "vector_math.h"
#include "base_control.h"
#include "common_logic.h"

#define BASE_RADIUS 2.0f
#define HOME_RADIUS 6.0f
#define HOME_LINE_Z -0.65f

// ============================================================================
// Referee Update Pipeline (Milestone 15)
// ============================================================================

static void update_safety_status(const StateInfo* stateInfo, RefereeState* referee)
{
	const LocalGameInfo* game = stateInfo->localGameInfo;

	// Process players in base order (HOME -> FIRST -> SECOND -> THIRD)
	// This ensures deterministic safety resolution when two players are at the same base.
	// Lead runners (higher bases at pitch start) are processed first and lose safety.
	// Rear runners (lower bases at pitch start) are processed last and win safety.

	// Build sorted list of active players by their baseAtPitchStart
	int sortedPlayers[PLAYERS_IN_TEAM + JOKER_COUNT];
	int playerCount = 0;

	for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		const PlayerInfo* player = &game->playerInfo[i];
		if (player->bTPI.baseId != BASE_NONE &&
		        (player->bTPI.state == PLAYER_STATE_ON_BASE || player->bTPI.state == PLAYER_STATE_AT_BAT)) {
			sortedPlayers[playerCount++] = i;
		}
	}

	// Sort by baseAtPitchStart (HIGHER bases first: THIRD=3, SECOND=2, FIRST=1, HOME=0)
	// So lead runners (higher bases) processed first, rear runners processed last and win conflicts
	for (int i = 0; i < playerCount - 1; i++) {
		for (int j = i + 1; j < playerCount; j++) {
			BaseID baseI = referee->battingPlayers[sortedPlayers[i]].baseAtPitchStart;
			BaseID baseJ = referee->battingPlayers[sortedPlayers[j]].baseAtPitchStart;
			if (baseI < baseJ) {  // REVERSED: higher bases first
				int temp = sortedPlayers[i];
				sortedPlayers[i] = sortedPlayers[j];
				sortedPlayers[j] = temp;
			}
		}
	}

	// 2.5 Safety Acquisition & Displacement
	// Process in field order (rear runners processed last, win safety conflicts)
	for (int idx = 0; idx < playerCount; idx++) {
		int i = sortedPlayers[idx];
		const PlayerInfo* player = &game->playerInfo[i];
		BaseID physicalBase = player->bTPI.baseId;

		// Player is physically at a base (we already filtered for ON_BASE/AT_BAT)
		// But Referee doesn't recognize them as the controller yet
		if (referee->battingPlayers[i].currentSafetyBase != physicalBase) {

			// Grant Safety
			referee->battingPlayers[i].currentSafetyBase = physicalBase;

			// Check if this arriving player is marked for a wound
			int isWoundPending = referee->battingPlayers[i].hasPendingWound || referee->woundingPlayersMarked[i];

			if (isWoundPending) {
				// TUPLAHAAVA LOGIC: Do not displace. Mark existing occupants for Tuplahaava.
				for (int j = 0; j < PLAYERS_IN_TEAM + JOKER_COUNT; j++) {
					if (i == j) continue;

					if (referee->battingPlayers[j].currentSafetyBase == physicalBase) {
						// Found an existing occupant. Mark them for Tuplahaava.

						// If the timer has already expired (arriving player is pending),
						// we must immediately set the occupant to pending as well.
						if (referee->battingPlayers[i].hasPendingWound) {
							referee->battingPlayers[j].hasPendingWound = 1;
						} else {
							// Otherwise, mark them for the timer to pick up.
							referee->woundingPlayersMarked[j] = 1;
						}

						referee->battingPlayers[j].woundingType = WOUNDING_TYPE_TUPLAHAAVA;
						referee->battingPlayers[j].woundingSourceBase = physicalBase;
						// Crucial: They KEEP safety for now.
					}
				}
				// Ensure arriving player is marked correctly
				referee->battingPlayers[i].woundingType = WOUNDING_TYPE_NORMAL;
			} else {
				// STANDARD LOGIC: Displace old owner (if any)
				for (int j = 0; j < PLAYERS_IN_TEAM + JOKER_COUNT; j++) {
					if (i == j) continue;

					if (referee->battingPlayers[j].currentSafetyBase == physicalBase) {
						referee->battingPlayers[j].currentSafetyBase = BASE_NONE;
					}
				}
			}
		}
	}
}

static void update_force_outs_and_tuplahaava(const StateInfo* stateInfo, RefereeState* referee, GameState* gameState, int ballAtBase)
{
	const LocalGameInfo* game = stateInfo->localGameInfo;

	// 3. Check for Outs (§33) and Tuplahaava Exceptions (§36)
	if (ballAtBase != -1) {
		// Default assumption: If ball is at a base, Run of Honor possibility is threatened.
		// decisions.canMakeRunOfHonor = 0; // We need to update GameModeState? Or pass it?
		// Referee_Apply handled this: if (decisions.canMakeRunOfHonor == 0) gameModeState.canMakeRunOfHonor = 0;
		// We should pass GameModeState too or handle it here if we had access.
		// For now, let's focus on Outs.

		for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
			const PlayerInfo* player = &game->playerInfo[i];

			// Only check active players
			if (player->bTPI.baseId == BASE_NONE) continue;

			// Check Run of Honor (Moved to GameModeState update later?)

			BaseID ballBaseId = (BaseID)ballAtBase;
			BaseID checkBaseId;

			if (ballBaseId == BASE_HOME) {
				checkBaseId = BASE_THIRD; // Home base forces runner from 3rd
			} else {
				checkBaseId = (BaseID)(ballAtBase - 1);
			}

			// A. Force Out / Burning (§33)
			int has_safety_at_current = (referee->battingPlayers[i].currentSafetyBase == player->bTPI.baseId);
			int is_protected = player_is_protected(player->bTPI.state);
			int is_safe_from_force_out = has_safety_at_current && is_protected;

			if (is_runner_forced_out(
			            player->bTPI.baseId,
			            is_safe_from_force_out,
			            checkBaseId,
			            player->bTPI.state == PLAYER_STATE_ADVANCING_FREELY,
			            gameState
			        )) {

				referee->battingPlayers[i].isOut = 1;
				gameState->outs += 1;
				gameState->event = EVENT_OUT; // Global event

				// Remove safety if they had it at this base
				if (has_safety_at_current) {
					referee->battingPlayers[i].currentSafetyBase = BASE_NONE;
				}

				// Critical Fix: Clear baseAtPitchStart so this player isn't resurrected by foulPlay
				referee->battingPlayers[i].baseAtPitchStart = BASE_NONE;
			}

			// B. Safety Removal (§36 Koppilyönti logic)
			BaseID player_safety_base = referee->battingPlayers[i].currentSafetyBase;

			if (player_safety_base != BASE_NONE && player_safety_base == (BaseID)ballAtBase) {
				if (!is_protected) {
					// Player has safety here but is "irti" - lose safety and must run
					referee->battingPlayers[i].currentSafetyBase = BASE_NONE;
					// Force advance logic handled by Reconcile
				}
			}

			// C. Tuplahaava Exceptions (Explicit Logic from game_analysis.c)
			if (referee->battingPlayers[i].hasPendingWound &&
			        referee->battingPlayers[i].woundingType == WOUNDING_TYPE_TUPLAHAAVA &&
			        !referee->battingPlayers[i].isOut) {

				BaseID source = referee->battingPlayers[i].woundingSourceBase;
				BaseID next = base_get_next(source);
				int is_in_between = (player->bTPI.state != PLAYER_STATE_ON_BASE);

				if (is_in_between) {
					// Exception 2: Ball at NEXT base -> OUT
					if (base_to_int_index(next) == ballAtBase) {
						referee->battingPlayers[i].isOut = 1;
						gameState->outs += 1;
						gameState->event = EVENT_OUT;
						referee->battingPlayers[i].currentSafetyBase = BASE_NONE; // Clear source safety
						referee->battingPlayers[i].baseAtPitchStart = BASE_NONE;
					}
					// Exception 1: Ball at SOURCE base -> Lose Safety
					else if (base_to_int_index(source) == ballAtBase) {
						referee->battingPlayers[i].currentSafetyBase = BASE_NONE;
						// "Transition to NORMAL wounding type"
						referee->battingPlayers[i].woundingType = WOUNDING_TYPE_NORMAL;
					}
				}
			}
		}
	}
}

static void update_runs(const StateInfo* stateInfo, RefereeState* referee, GameState* gameState, GameModeState* gameModeState, GameControlFlags* gameControl, PlayerCounters* playerCounters, GlobalGameInfo* globalGameInfo)
{
	const LocalGameInfo* game = stateInfo->localGameInfo;

	// 4. Check for Runs (§41/42)
	if (gameControl->checkForRun == 1) {
		if ((gameControl->firstCatchMade == 1 || game->ballInfo.hasHitGround == 1) &&
		        referee->woundingCatchTimer == -1 &&
		        game->gameFlowState.endOfInningCounter == -1 &&
		        gameState->outOfBounds == 0) {

			// Check all players
			for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
				if (game->playerInfo[i].bTPI.baseId != BASE_NONE) {
					int runScored = calculate_runs(
					                    game->playerInfo[i].bTPI.baseId,
					                    referee->battingPlayers[i].baseAtPitchStart,
					                    game->playerInfo[i].bTPI.state == PLAYER_STATE_WOUNDED,
					                    gameModeState,
					                    game->playerRuntime[i].hasMadeRunOnThirdBase
					                );

					if (runScored) {
						gameState->event = EVENT_RUN_SCORED;
						int battingTeamIndex = (globalGameInfo->inning + globalGameInfo->playsFirst + globalGameInfo->period) % 2;
						globalGameInfo->teams[battingTeamIndex].runs += 1;
						gameState->runsInTheInning += 1;

						referee->battingPlayers[i].hasScored = 1;
						referee->battingPlayers[i].baseAtPitchStart = BASE_NONE;

						if (gameState->runsInTheInning % 2 == 0) {
							playerCounters->nonJokerPlayersLeft = PLAYERS_IN_TEAM;
						}

						// §42 Overtaking Logic (Check if Kunniajuoksu overtakes someone)
						if (game->playerInfo[i].bTPI.baseId == BASE_THIRD) {
							int someone_else_has_third_safety = 0;
							for (int j = 0; j < PLAYERS_IN_TEAM + JOKER_COUNT; j++) {
								if (j != i && referee->battingPlayers[j].currentSafetyBase == BASE_THIRD) {
									someone_else_has_third_safety = 1;
									break;
								}
							}

							if (someone_else_has_third_safety) {
								referee->battingPlayers[i].isOut = 1;
								gameState->outs += 1;
							}
						}
					}
				}
			}

			// Period End Check
			int battingTeamIndex = (globalGameInfo->inning + globalGameInfo->playsFirst + globalGameInfo->period) % 2;
			int catchingTeamIndex = (battingTeamIndex + 1) % 2;
			int currentRuns = globalGameInfo->teams[battingTeamIndex].runs;
			int opponentRuns = globalGameInfo->teams[catchingTeamIndex].runs;

			if (globalGameInfo->period < 4) {
				if ((globalGameInfo->inning + 1) % globalGameInfo->halfInningsInPeriod == 0 ||
				        globalGameInfo->inning + 1 == globalGameInfo->halfInningsInPeriod * 2 + 2) {
					if (currentRuns > opponentRuns) {
						gameState->endPeriod = 1;
					}
					if (globalGameInfo->inning + 1 == globalGameInfo->halfInningsInPeriod * 2 &&
					        globalGameInfo->teams[battingTeamIndex].period0Runs > globalGameInfo->teams[catchingTeamIndex].period0Runs &&
					        opponentRuns == currentRuns) {
						gameState->endPeriod = 1;
					}
				}
			} else {
				if ((globalGameInfo->inning + 1) % 2 == 0) {
					if (currentRuns > opponentRuns) {
						gameState->endPeriod = 1;
					}
				}
			}

			// Mark check as done
			gameControl->checkForRun = 0;
		}
	}
}

void Referee_Update(const StateInfo* stateInfo, RefereeState* refereeState, GameState* gameState, GameModeState* gameModeState, GameControlFlags* gameControl, PlayerCounters* playerCounters, GlobalGameInfo* globalGameInfo)
{
	// 1. Where is the ball?
	int ballAtBase = get_ball_at_base_index(stateInfo);

	// 2. Run of Honor (Update directly if needed? Legacy did it in Analyze but applied in Apply)
	// Here we update it if ballAtBase is valid.
	if (ballAtBase != -1) {
		// Logic from Analyze: "Default assumption: If ball is at a base, Run of Honor possibility is threatened."
		int canMake = 0;
		const LocalGameInfo* game = stateInfo->localGameInfo;
		for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
			const PlayerInfo* player = &game->playerInfo[i];
			if (player->bTPI.baseId == BASE_NONE) continue;

			if (base_is_at_least(player->bTPI.baseId, BASE_SECOND) &&
			        refereeState->battingPlayers[i].baseAtPitchStart == BASE_HOME &&
			        ballAtBase != 3) {
				canMake = 1;
			}
		}
		gameModeState->canMakeRunOfHonor = canMake;
	}

	// 3. Safety Pipeline
	update_safety_status(stateInfo, refereeState);
	update_force_outs_and_tuplahaava(stateInfo, refereeState, gameState, ballAtBase);
	update_runs(stateInfo, refereeState, gameState, gameModeState, gameControl, playerCounters, globalGameInfo);
}

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

void initializeRefereeState(RefereeState* referee)
{
	int i;
	for (i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		referee->battingPlayers[i].baseAtPitchStart = BASE_NONE;
		referee->battingPlayers[i].hadSafetyAtPitchStart = 0;
		referee->battingPlayers[i].currentSafetyBase = BASE_NONE;
		referee->battingPlayers[i].isOut = 0;
		referee->battingPlayers[i].hasScored = 0;
		referee->battingPlayers[i].hasPendingWound = 0;
		referee->battingPlayers[i].woundingType = WOUNDING_TYPE_NONE;
		referee->battingPlayers[i].woundingSourceBase = BASE_NONE;
		referee->battingPlayers[i].baseAtLastEvent = BASE_NONE;
		referee->battingPlayers[i].hadSafetyAtLastEvent = 0;

		referee->woundingPlayersMarked[i] = 0;
	}
	referee->woundingCatchActive = 0;
	referee->foulPlayActive = 0;
	referee->strikesAtPitchStart = 0;
	referee->woundingCatchPending = 0;
	referee->woundingCatchHandled = 0;
	referee->woundingCatchTimer = -1;
}