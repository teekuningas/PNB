#include "game_consolidation.h"
#include "common_logic.h"
#include "game_manipulation.h"
#include "game_setup.h" // For setRunnerAndBatter and initialization helpers
#include "base_logic.h"
#include "base_control.h"
#include "rules_pure/player_utils.h"
#include "action_implementation.h" // For prepareBatter if needed, or we move it? prepareBatter is in action_implementation.c
#include "game_reset.h"
#include "referee.h"

// ===============================================================================================
// FORWARD DECLARATIONS
// ===============================================================================================

static void enforce_legal_state(StateInfo* stateInfo, const RefereeState* referee, const BetweenPitchState* bps);
static int handle_foul_play_reset(StateInfo* stateInfo, const RefereeState* referee, unsigned int* rng_seed);
static void update_game_flow(
    StateInfo* stateInfo, const RefereeState* referee, const BetweenPitchState* bps, const HalfInningState* his,
    const Scoreboard* scoreboard, MenuInfo* menuInfo, unsigned int* rng_seed
);

static void check_next_batter_decision(
    StateInfo* stateInfo, const RefereeState* referee, const BetweenPitchState* bps, const Scoreboard* scoreboard
);
static void handle_strikes_and_balls(StateInfo* stateInfo, const HalfInningState* his);
static int check_end_of_inning(
    StateInfo* stateInfo, const RefereeState* referee, const Scoreboard* scoreboard, MenuInfo* menuInfo,
    unsigned int* rng_seed
);
static int check_next_pair(
    StateInfo* stateInfo, const RefereeState* referee, const Scoreboard* scoreboard, unsigned int* rng_seed
);
static void populate_game_conclusion(StateInfo* stateInfo, const Scoreboard* scoreboard, int winner);

// ===============================================================================================
// PUBLIC API
// ===============================================================================================

void consolidation_init(GameFlowState* gameFlowState)
{
    gameFlowState->closeToGround = 0;
    gameFlowState->homeRunCameraCounter = -1;
}

void consolidation_update(
    StateInfo* stateInfo, const RefereeState* referee, const BetweenPitchState* bps, const HalfInningState* his,
    const Scoreboard* scoreboard, MenuInfo* menuInfo, unsigned int* rng_seed
)
{
    // 0. Check for Physical Resets FIRST (these reset the world)
    // If any reset happens, abort all further processing for this frame

    // Check for end-of-inning reset
    if (check_end_of_inning(stateInfo, referee, scoreboard, menuInfo, rng_seed)) {
        return;
    }

    // Check for next pair reset (homerun contest)
    if (check_next_pair(stateInfo, referee, scoreboard, rng_seed)) {
        return;
    }

    // Check for foul play (out of bounds) reset
    if (handle_foul_play_reset(stateInfo, referee, rng_seed)) {
        return;
    }

    // 1. Game Flow Analysis (The Game Master)
    // Decides if we need to pause for input, etc.
    update_game_flow(stateInfo, referee, bps, his, scoreboard, menuInfo, rng_seed);

    // 2. State Enforcement (The Enforcer)
    // Ensures physical entities obey legal outcomes (Outs, Scores, Safety).
    enforce_legal_state(stateInfo, referee, bps);
}

// ===============================================================================================
// INTERNAL: PHYSICAL ENFORCEMENT (formerly reconcileLegalAndPhysicalState)
// ===============================================================================================

static void enforce_legal_state(StateInfo* stateInfo, const RefereeState* referee, const BetweenPitchState* bps)
{
    MatchSession* game = stateInfo->match;
    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        // 1. React to OUT
        if (referee->battingPlayers[i].status == PLAYER_STATUS_OUT) {
            if (game->playerInfo[i].bTPI.state != PLAYER_STATE_OUT) {
                game->playerInfo[i].bTPI.state = PLAYER_STATE_OUT;
                game->playerInfo[i].bTPI.baseId = BASE_NONE;
                movePlayerOut(game->playerInfo, game->playerRuntime, stateInfo->fieldPositions, i);
            }
        }

        // 2. React to SCORE
        if (referee->battingPlayers[i].hasScored && game->playerInfo[i].bTPI.state != PLAYER_STATE_SCORED) {
            game->playerInfo[i].bTPI.state = PLAYER_STATE_SCORED;
            game->playerInfo[i].bTPI.baseId = BASE_NONE;
            movePlayerOut(game->playerInfo, game->playerRuntime, stateInfo->fieldPositions, i);
        }

        // 3. React to WOUNDED
        if (referee->battingPlayers[i].status == PLAYER_STATUS_WOUNDED) {
            if (game->playerInfo[i].bTPI.state != PLAYER_STATE_WOUNDED) {
                game->playerInfo[i].bTPI.state = PLAYER_STATE_WOUNDED;
                game->playerInfo[i].bTPI.baseId = BASE_NONE;
                movePlayerOut(game->playerInfo, game->playerRuntime, stateInfo->fieldPositions, i);
            }
        }

        // 4. React to displacement (Panic Run)
        if (game->playerInfo[i].bTPI.state == PLAYER_STATE_ON_BASE ||
            game->playerInfo[i].bTPI.state == PLAYER_STATE_LEADING) {
            BaseID physBase = game->playerInfo[i].bTPI.baseId;
            if (referee->battingPlayers[i].currentSafetyBase != physBase) {
                // Don't force run if player is wounded (WOUNDED status)
                // They will be removed from the field instead
                int isWounded = (referee->battingPlayers[i].status == PLAYER_STATUS_WOUNDED);

                if (!isWounded) {
                    // Player is physically at base but legally has no safety there.
                    // They must run forward.
                    runToNextBase(game, stateInfo->fieldPositions, i, physBase);
                }
            }
        }
    }

    // 4. React to Pitch Resolution
    if (bps->pitchResult != PITCH_RESULT_NONE && game->pRAI.pitchState != PITCH_STAGE_NONE) {
        game->pRAI.pitchState = PITCH_STAGE_NONE;
        // On ball: reset free walk calculation so it is re-evaluated
        if (bps->pitchResult == PITCH_RESULT_BALL) {
            game->flowControl.freeWalkCalculationMade = 0;
            game->flowControl.freeWalkIndex = -1;
            game->flowControl.freeWalkBase = BASE_NONE;
        }
    }
}

// ===============================================================================================
// INTERNAL: FOUL PLAY RESET
// ===============================================================================================

static int handle_foul_play_reset(StateInfo* stateInfo, const RefereeState* referee, unsigned int* rng_seed)
{
    // Check if Referee has triggered the reset state
    if (referee->foulState == FOUL_STATE_RESETTING) {
        reset_for_foul_play(stateInfo, referee, rng_seed);

        // Note: Referee will transition state to NONE in referee_finalize
        // No manual counter manipulation needed here anymore.
        return 1; // Signal that we reset
    }
    return 0; // No reset
}

// Formerly applyFoulPlayReset in game_setup.c
// Now handled by game_reset.c: reset_for_foul_play

// ===============================================================================================
// INTERNAL: GAME FLOW (formerly game_analysis)
// ===============================================================================================

static void update_game_flow(
    StateInfo* stateInfo, const RefereeState* referee, const BetweenPitchState* bps, const HalfInningState* his,
    const Scoreboard* scoreboard, MenuInfo* menuInfo, unsigned int* rng_seed
)
{
    // when player from third base starts running, we change camera view. when the situation is over we
    // wait 50 update frames, before moving to normal camera
    if (stateInfo->match->gameFlowState.homeRunCameraCounter >= 0) {
        stateInfo->match->gameFlowState.homeRunCameraCounter++;
        if (stateInfo->match->gameFlowState.homeRunCameraCounter > 50) {
            stateInfo->match->cameraState.homeRunCameraFlag = 0;
            stateInfo->match->gameFlowState.homeRunCameraCounter = -1;
        }
    }

    check_next_batter_decision(stateInfo, referee, bps, scoreboard);
    handle_strikes_and_balls(stateInfo, his);
}

static void check_next_batter_decision(
    StateInfo* stateInfo, const RefereeState* referee, const BetweenPitchState* bps, const Scoreboard* scoreboard
)
{
    // Cancel pending batter request if inning is ending
    if (referee->endOfInningState != END_INNING_STATE_NONE) {
        stateInfo->match->flowControl.waitingForBatterDecision = 0;
        return;
    }

    // so this function's idea is to make progress in selecting a new batter if old one's gone.
    // so this will be called only once when possible.
    if (scoreboard->period >= 4) {

    } else if (get_active_batter_index(stateInfo->match) == -1 &&
               stateInfo->match->flowControl.waitingForBatterDecision == 0 &&
               referee->endOfInningState == END_INNING_STATE_NONE) {
        // there have to be a player available
        if (stateInfo->match->playerCounters.nonJokerPlayersLeft + stateInfo->match->playerCounters.jokersLeft > 0) {
            // have to check that there is only three players in the field too and that it is not a out of bounds
            // situation.
            if (count_active_batting_players(stateInfo->match->playerInfo) < BASE_COUNT &&
                referee->foulState == FOUL_STATE_NONE) {
                // also we cannot know yet if it will be out of position situation so we have to wait that the ball will
                // land in some way.
                if (bps->hasBallHitGround == 1 || bps->catchHasBeenMade == 1) {
                    // if that happens we can now start.
                    int battingTeamIndex = get_batting_team_index(scoreboard);
                    // this will give work to action_invocatin.c and action_implementation.c
                    stateInfo->match->flowControl.waitingForBatterDecision = 1;
                    // we just select the batterSelectionIndex here. if there are nonJokerPlayerLeft, we
                    // just select the next batter in order there. if not, we select the first joker we find that is
                    // still unused. one of these must be true, as we checked there is joker or non-joker left before.
                    if (stateInfo->match->playerCounters.nonJokerPlayersLeft != 0) {
                        stateInfo->match->pII.batterSelectionIndex =
                            scoreboard->teams[battingTeamIndex]
                                .batterOrder[scoreboard->teams[battingTeamIndex].batterOrderIndex];
                    } else {
                        int i;
                        for (i = 0; i < JOKER_COUNT; i++) {
                            if (stateInfo->match->playerInfo[stateInfo->match->pII.jokerIndices[i]].bTPI.joker ==
                                JOKER_AVAILABLE) {
                                stateInfo->match->pII.batterSelectionIndex = stateInfo->match->pII.jokerIndices[i];
                                break;
                            }
                        }
                    }
                }
            }
        } else {
            stateInfo->match->playerCounters.noMorePlayers = 1;
        }
    }
}

// Here we handle strikes and balls related consequences. Batter can't have more than 3 strikes,
// so something must be done, and if pitcher pitches balls, compensation is needed.
static void handle_strikes_and_balls(StateInfo* stateInfo, const HalfInningState* his)
{
    // if there are three strikes
    if (his->strikes >= 3) {
        // We restore automatic force running to resolve control ambiguity.
        // The batter is now "forced" to run by the rules.
        int index = get_base_controller(stateInfo->match, BASE_HOME);

        // Only force run if player is still there and NOT already running.
        // This prevents re-triggering every frame while preserving the 3 strikes state
        // until the next batter resets it.
        if (index != -1 && stateInfo->match->playerInfo[index].bTPI.state != PLAYER_STATE_RUNNING) {
            runToNextBase(stateInfo->match, stateInfo->fieldPositions, index, BASE_HOME);
            // Remove safety from home base handled by Referee update_safety_status eventually
            // or explicitly here if needed, but runToNextBase sets state to RUNNING which
            // allows reconciliation logic to work.
        }
    }
    // we calculate the player and the base he has right to go freely only once, and that is when
    // the ball happens. if player moves to next base and user after that decides to make the free walk
    // that wont have any effect.
    if (stateInfo->match->flowControl.freeWalkCalculationMade == 0) {
        if (count_active_batting_players(stateInfo->match->playerInfo) == 1) {
            // if only one player on the field, thats the batter, and then free walks can be made after one pitch.
            if (his->balls >= 1) {
                // calculate the index and the base.
                calculateFreeWalk(stateInfo->match);
                // and tell action_implementation.c to take care of the rest.
                stateInfo->match->flowControl.waitingForFreeWalkDecision = 1;
            }
        } else {
            // otherwise there is some non-batter leadrunner and he can have free walks after too balls.
            if (his->balls >= 2) {
                // calculate the index and the base.
                calculateFreeWalk(stateInfo->match);
                // and tell action_implementation.c to take care of the rest.
                stateInfo->match->flowControl.waitingForFreeWalkDecision = 1;
            }
        }
        stateInfo->match->flowControl.freeWalkCalculationMade = 1;
    } else {
        // so that if player just ran without taking hiw free walk, and got wounded or out, then stop asking
        // that question.
        if (stateInfo->match->flowControl.waitingForFreeWalkDecision == 1) {
            if (stateInfo->match->playerInfo[stateInfo->match->flowControl.freeWalkIndex].bTPI.state ==
                    PLAYER_STATE_WOUNDED ||
                stateInfo->match->playerInfo[stateInfo->match->flowControl.freeWalkIndex].bTPI.state ==
                    PLAYER_STATE_OUT) {
                stateInfo->match->flowControl.waitingForFreeWalkDecision = 0;
            }
        }
    }
}

static int check_end_of_inning(
    StateInfo* stateInfo, const RefereeState* referee, const Scoreboard* scoreboard, MenuInfo* menuInfo,
    unsigned int* rng_seed
)
{
    // Scoreboard advancement and period logic handled by referee at DETECTED→RESETTING.
    // We only react when Referee signals RESETTING: perform physical reset + menu routing.

    if (referee->endOfInningState != END_INNING_STATE_RESETTING) {
        return 0;
    }

    PeriodTransitionType transition = referee->periodTransition;

    if (transition == PERIOD_TRANSITION_NONE) {
        // Normal next-inning within same period — just reset physical world
        reset_for_new_half_inning(stateInfo, rng_seed);
    } else {
        // Period transition — route to appropriate menu
        switch (transition) {
        case PERIOD_TRANSITION_INTER_PERIOD:
            menuInfo->mode = MENU_ENTRY_INTER_PERIOD;
            break;
        case PERIOD_TRANSITION_SUPER_INNING:
            menuInfo->mode = MENU_ENTRY_SUPER_INNING;
            break;
        case PERIOD_TRANSITION_HOMERUN_CONTEST:
            menuInfo->mode = MENU_ENTRY_HOMERUN_CONTEST;
            break;
        case PERIOD_TRANSITION_GAME_OVER:
            populate_game_conclusion(stateInfo, scoreboard, referee->periodTransitionWinner);
            menuInfo->mode = MENU_ENTRY_GAME_OVER;
            break;
        default:
            break;
        }
        stateInfo->screen = SCREEN_MAIN_MENU;
        stateInfo->changeScreen = 1;
        stateInfo->updated = 0;
    }

    return 1; // Signal that we handled end-of-inning
}

static int
check_next_pair(StateInfo* stateInfo, const RefereeState* referee, const Scoreboard* scoreboard, unsigned int* rng_seed)
{
    if (scoreboard->period >= 4) {

        // Milestone 17.5: Timer and logic moved to Referee (State Machine).
        // We only react when Referee signals RESETTING (State 2).

        HomeRunPairState currentState = referee->nextPairTransitionState;

        if (currentState == HR_PAIR_STATE_RESETTING) {

            // if equality holds, ending of inning will load the settings.
            if (stateInfo->match->homeRunContestState.runnerBatterPairCounter != scoreboard->pairCount) {
                // Physical Reset for Next Pair
                reset_for_next_pair(stateInfo, rng_seed);
            }
            return 1; // Signal that we reset
        }
    }
    return 0; // No reset
}

static void populate_game_conclusion(StateInfo* stateInfo, const Scoreboard* scoreboard, int winner)
{
    stateInfo->gameConclusion->winner = winner;
    stateInfo->gameConclusion->isCupGame = scoreboard->isCupGame;
    stateInfo->gameConclusion->period0Runs[0] = scoreboard->teams[0].period0Runs;
    stateInfo->gameConclusion->period0Runs[1] = scoreboard->teams[1].period0Runs;
    stateInfo->gameConclusion->period1Runs[0] = scoreboard->teams[0].period1Runs;
    stateInfo->gameConclusion->period1Runs[1] = scoreboard->teams[1].period1Runs;
    stateInfo->gameConclusion->period2Runs[0] = scoreboard->teams[0].period2Runs;
    stateInfo->gameConclusion->period2Runs[1] = scoreboard->teams[1].period2Runs;
    stateInfo->gameConclusion->period3Runs[0] = scoreboard->teams[0].period3Runs;
    stateInfo->gameConclusion->period3Runs[1] = scoreboard->teams[1].period3Runs;
}
