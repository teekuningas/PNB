/*
    execute_actions.c — Intent execution.

    Translates ActionFlags (set by human input or AI) into physical state changes:
    throwing, pitching, batting, base running, player selection.
*/

#include "globals.h"
#include "execute_actions.h"
#include "common_logic.h"
#include "actions/pitching_system.h"
#include "actions/batting_system.h"
#include "actions/throwing_system.h"
#include "ai/catching_ai.h"
#include "ai/batting_ai.h"
#include "base_logic.h"
#include "base_control.h"
#include "rules_pure/player_utils.h"

#define ANIMATION_FREQUENCY 3

#define CLICK_BREAK_CONSTANT 3

static void change_batter(StateInfo* stateInfo);
static void take_free_walk_decision(StateInfo* stateInfo);
static void base_run(StateInfo* stateInfo, BaseID base);
;

void init_execute_actions(StateInfo* stateInfo)
{
    // just initialize everyone of these static variables to zero
    int i;

    stateInfo->match->pendingActionState.meterCounter = 0;
    stateInfo->match->pendingActionState.meterCounterMax = 0;
    stateInfo->match->pendingActionState.currentCatchingAction = CATCHING_ACTION_NONE;
    for (i = 0; i < BASE_COUNT; i++) {
        stateInfo->match->pendingActionState.doubleClickCounter[i] = -1;
    }

    reset_pitching_system(stateInfo);
    init_batting_system(stateInfo);
    init_throwing_system(stateInfo);
    stateInfo->match->pendingActionState.runBatFlag = 0;

    // ai uses a few flags..

    init_catching_ai(&(stateInfo->match->aiState));
    stateInfo->match->pendingActionState.aiActionEventLock = -1;
    stateInfo->match->pendingActionState.aiLockUpdate = 0;

    init_batting_ai(&(stateInfo->match->aiState));
}

void execute_actions(StateInfo* stateInfo)
{
    int i;

    // double click counter
    for (i = 0; i < BASE_COUNT; i++) {
        if (stateInfo->match->pendingActionState.doubleClickCounter[i] >= 0) {
            stateInfo->match->pendingActionState.doubleClickCounter[i]++;
            if (stateInfo->match->pendingActionState.doubleClickCounter[i] >= 20) {
                stateInfo->match->pendingActionState.doubleClickCounter[i] = -1;
            }
        }
    }

    /*
     * CATCHING TEAM
     */

    for (i = 0; i < BASE_COUNT; i++) {
        // for every direction we check if throw key has been pressed
        if (stateInfo->match->aF.cTAF.throwToBase[i] == ACTION_TRIGGER_START) {
            // can throw only if someone has the ball and no throw is already going on
            if (stateInfo->match->pII.hasBallIndex != -1) {
                int j;
                for (j = 0; j < BASE_COUNT; j++) {
                    if (j != i) stateInfo->match->aF.cTAF.throwToBase[j] = ACTION_IDLE;
                }
                // stop pitching if throwing
                if (stateInfo->match->pRAI.pitchState != PITCH_STAGE_NONE) {
                    stateInfo->match->aF.cTAF.pitch = PITCH_ACTION_IDLE;
                    stateInfo->match->pRAI.pitchState = PITCH_STAGE_NONE;
                    stateInfo->match->pendingActionState.pitchPhase = PITCH_PHASE_NONE;
                    // when pitching the ball is moved to the center of the plate so now when we are terminating the
                    // pitch to throw, we must move the ball back to the player
                    stateInfo->match->ballInfo.location.x =
                        stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].tPI.location.x;
                    stateInfo->match->ballInfo.location.z =
                        stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].tPI.location.z;
                }
                // throwGoingToBase variables are used to have better control
                // over basemen who are wanting go out of base catching the ball.
                // throws can be directed only towards bases.
                prepare_throw(stateInfo, i);
                // start by loading
                throw_load(stateInfo, i);
                stateInfo->match->pendingActionState.currentCatchingAction = CATCHING_ACTION_THROWING;
            }
            // consume the intent regardless of success
            stateInfo->match->aF.cTAF.throwToBase[i] = ACTION_IDLE;
        }
        // if throw release intent received while a throw is in progress
        else if (stateInfo->match->aF.cTAF.throwToBase[i] == ACTION_TRIGGER_STOP) {
            stateInfo->match->aF.cTAF.throwToBase[i] = ACTION_IDLE;
            if (stateInfo->match->pendingActionState.currentCatchingAction == CATCHING_ACTION_THROWING) {
                throw_release(stateInfo);
                stateInfo->match->pendingActionState.currentCatchingAction = CATCHING_ACTION_NONE;
            }
        }
    }
    // Auto-clear: if throw was interrupted externally (e.g., ball caught by game_manipulation
    // cleared throwGoingOn), reset the action state so other actions can proceed.
    if (stateInfo->match->pendingActionState.currentCatchingAction == CATCHING_ACTION_THROWING &&
        stateInfo->match->pendingActionState.throwGoingOn == 0) {
        stateInfo->match->pendingActionState.currentCatchingAction = CATCHING_ACTION_NONE;
    }
    // Safety: if throwGoingOn is stuck but no throw action is active, clear it.
    // This catches the case where a reset cleared currentCatchingAction but missed throwGoingOn.
    if (stateInfo->match->pendingActionState.throwGoingOn == 1 &&
        stateInfo->match->pendingActionState.currentCatchingAction != CATCHING_ACTION_THROWING) {
        stateInfo->match->pendingActionState.throwGoingOn = 0;
        stateInfo->match->pRAI.throwGoingToBase = -1;
    }
    // if move keys have been pressed, depending on if its down or release
    // call corresponding function for every direction
    for (i = 0; i < DIRECTION_COUNT; i++) {
        if (stateInfo->match->aF.cTAF.move[i] == ACTION_TRIGGER_START) {
            fielder_move(stateInfo, i);
        } else if (stateInfo->match->aF.cTAF.move[i] == ACTION_TRIGGER_STOP) {
            fielder_stop_move(stateInfo, i);
        }
    }

    // if change player key has been pressed
    if (stateInfo->match->aF.cTAF.change_player == ACTION_TRIGGER_START) {
        // no one must have the ball
        if (stateInfo->match->pII.hasBallIndex == -1) {
            // we go to next element in changePlayerArray.
            stateInfo->match->pII.changePlayerArrayIndex =
                (stateInfo->match->pII.changePlayerArrayIndex + 1) % CHANGE_PLAYER_COUNT;
            // and try to ensure that there is difference. we dont want to end up in a endless loop
            // though so we do it only once.
            if (stateInfo->match->pII.controlIndex ==
                stateInfo->match->pII.fielderRankedIndices[stateInfo->match->pII.changePlayerArrayIndex]) {
                stateInfo->match->pII.changePlayerArrayIndex =
                    (stateInfo->match->pII.changePlayerArrayIndex + 1) % CHANGE_PLAYER_COUNT;
            }
            // and then set the flag, so that other parts of code can handle
            // the job
            change_player(stateInfo->match);
        }
        stateInfo->match->aF.cTAF.change_player = ACTION_IDLE;
    }
    // if drop ball key has been pressed, try dropping
    if (stateInfo->match->aF.cTAF.dropBall == ACTION_TRIGGER_START) {
        drop_ball(stateInfo);
    }
    // pitching
    if (stateInfo->match->aF.cTAF.pitch == PITCH_ACTION_START) {
        start_pitch(stateInfo);
    } else if (stateInfo->match->aF.cTAF.pitch == PITCH_ACTION_POWER_SET) {
        continue_pitch(stateInfo);
    } else if (stateInfo->match->aF.cTAF.pitch == PITCH_ACTION_ANGLE_SET) {
        release_pitch(stateInfo);
    }
    // Safety auto-clear: if pitching action is stuck but pitchState is NONE, release it.
    if (stateInfo->match->pendingActionState.currentCatchingAction == CATCHING_ACTION_PITCHING &&
        stateInfo->match->pRAI.pitchState == PITCH_STAGE_NONE &&
        stateInfo->match->pendingActionState.pitchPhase == PITCH_PHASE_NONE) {
        stateInfo->match->pendingActionState.currentCatchingAction = CATCHING_ACTION_NONE;
    }

    /*
     * BATTING TEAM
     */
    // when there's no batter, user is prompted to select the next batter
    if (stateInfo->match->aF.bTAF.chooseBatter == CHOOSE_BATTER_NEXT) {
        change_batter(stateInfo);
    } else if (stateInfo->match->aF.bTAF.chooseBatter == CHOOSE_BATTER_SELECT) {
        select_batter(stateInfo);
    }
    // free walk decisions, takeFreeWalk can be 0, 1 or 2. if its 2
    // takeFreeWalkDecision() is called but will basically just set takeFreeWalk to 0.
    if (stateInfo->match->aF.bTAF.takeFreeWalk > FREE_WALK_IDLE) {
        take_free_walk_decision(stateInfo);
    }
    // batter angles
    if (stateInfo->match->aF.bTAF.increaseBatterAngle == ACTION_TRIGGER_START) {
        start_increase_batter_angle(stateInfo);
    } else if (stateInfo->match->aF.bTAF.increaseBatterAngle == ACTION_TRIGGER_STOP) {
        stop_increase_batter_angle(stateInfo);
    }
    if (stateInfo->match->aF.bTAF.decreaseBatterAngle == ACTION_TRIGGER_START) {
        start_decrease_batter_angle(stateInfo);
    } else if (stateInfo->match->aF.bTAF.decreaseBatterAngle == ACTION_TRIGGER_STOP) {
        stop_decrease_batter_angle(stateInfo);
    }
    // batting
    if (stateInfo->match->aF.bTAF.swing == BAT_ACTION_POWER_SET) {
        select_power(stateInfo);
    } else if (stateInfo->match->aF.bTAF.swing == BAT_ACTION_ANGLE_SET) {
        select_angle(stateInfo);
    }
    // baserunners must be able to run!
    for (i = 0; i < BASE_COUNT; i++) {
        base_run(stateInfo, i);
    }
    // this is used to handle a lot of stuff happening between and after the decisions.
    update_batting(stateInfo);
}

static void take_free_walk_decision(StateInfo* stateInfo)
{
    if (stateInfo->match->aF.bTAF.takeFreeWalk == FREE_WALK_ACCEPT) {
        int index = stateInfo->match->flowControl.freeWalkIndex;
        BaseID base = stateInfo->match->flowControl.freeWalkBase;
        if (index != -1) {
            // there can be a little gap between the decision and when the possibility to decide came
            // so player might have run already to the following base, and free walk actually
            // gave him the right to go to just that base.
            // so if he still has the same base as before we can go on
            if (stateInfo->rules->scoreboard.period >= 4) {
                // REFEREE MIGRATION: Logic moved to referee.c
                // We just signal the event here.
                stateInfo->match->gameEvents.freeWalkAccepted = 1;

            } else {
                BaseID currentBaseId = stateInfo->match->playerInfo[index].bTPI.baseId;

                if (currentBaseId == base) {
                    if (base != BASE_THIRD) {
                        // Start running to the next base
                        run_to_next_base(stateInfo->match, stateInfo->fieldPositions, index, base);
                        // Protected from wounds/tags while advancing freely
                        stateInfo->match->playerInfo[index].bTPI.state = PLAYER_STATE_ADVANCING_FREELY;
                    }
                    // 3rd base: no physical run needed — referee scores the run directly
                    // and sets hasScored, which causes enforceLegalState to remove the player
                }
                // REFEREE MIGRATION: Logic moved to referee.c
                // We just signal the event here.
                stateInfo->match->gameEvents.freeWalkAccepted = 1;
            }
        }
    }
    // no more decision to make.
    stateInfo->match->flowControl.waitingForFreeWalkDecision = 0;
    stateInfo->match->aF.bTAF.takeFreeWalk = FREE_WALK_IDLE;
}
// so when there is no batter and few other conditions hold
// we can select the batter from one player from the normal ordering of players and three joker players
static void change_batter(StateInfo* stateInfo)
{
    int done = 0;
    int counter = 0;
    // index in a teams[] array
    int battingTeamIndex = get_batting_team_index(&stateInfo->rules->scoreboard);
    int index;

    stateInfo->match->aF.bTAF.chooseBatter = 0;
    // batterSelect variable will point to the current player in selection
    // and now as we are changing the selection, we add one to it.
    stateInfo->match->pendingActionState.batterSelect++;
    // here we have a loop that basically just searches through the possible players and selects
    // the next one. batterSelect == 0 indicates that it is a normal player, batterSelect != 0 indicates
    // it is a joker player.
    // there must be at least one player as this function cannot get called without
    // waitingForBatterDecision-flag, and that can flag cant be true if
    // there is not at least one player.
    while (done == 0) {
        if (stateInfo->match->pendingActionState.batterSelect == 0) {
            if (stateInfo->rules->playerCounters.nonJokerPlayersLeft != 0)
                done = 1;
            else
                stateInfo->match->pendingActionState.batterSelect = 1;
        } else if (stateInfo->match->pendingActionState.batterSelect == 4) {
            if (stateInfo->rules->playerCounters.nonJokerPlayersLeft != 0) {
                stateInfo->match->pendingActionState.batterSelect = 0;
                done = 1;
            } else
                stateInfo->match->pendingActionState.batterSelect = 1;

        } else {
            if (stateInfo->match
                    ->playerInfo[stateInfo->match->pII
                                     .jokerIndices[stateInfo->match->pendingActionState.batterSelect - 1]]
                    .bTPI.joker == JOKER_USED)
                stateInfo->match->pendingActionState.batterSelect++;
            else
                done = 1;
        }
        if (counter == 4) done = 1;
        counter++;
    }
    // now we have the batterSelect value and we just need to find a corresponding index for that
    // player.
    if (stateInfo->match->pendingActionState.batterSelect == 0) {
        index = stateInfo->rules->scoreboard.teams[battingTeamIndex]
                    .batterOrder[stateInfo->rules->scoreboard.teams[battingTeamIndex].batterOrderIndex];
    } else {
        index = stateInfo->match->pII.jokerIndices[stateInfo->match->pendingActionState.batterSelect - 1];
    }
    // and set it here.
    stateInfo->match->pII.batterSelectionIndex = index;
}

void generic_sling_ball(BallInfo* ballInfo, float x, float y, float z)
{
    // Make ball visible and moving
    ballInfo->visible = 1;
    ballInfo->moving = 1;

    // Set the velocity
    set_vector_xyz(&(ballInfo->velocity), x, y, z);
}

// so baserunning.
// idea is just to update willStartRunning in every button press. and in special double click case we just run.
static void base_run(StateInfo* stateInfo, BaseID base)
{
    // so baserunning.
    // idea is just to update willStartRunning in every button press. and in special double click case we just run.
    if (get_base_controller(stateInfo->match, &stateInfo->rules->referee, base) != -1) {
        if (stateInfo->match->aF.bTAF.baseRun[base] == ACTION_TRIGGER_START) {
            int index = get_base_controller(stateInfo->match, &stateInfo->rules->referee, base);
            if (stateInfo->match->playerInfo[index].bTPI.state == PLAYER_STATE_ON_BASE ||
                stateInfo->match->playerInfo[index].bTPI.state == PLAYER_STATE_AT_BAT) {
                if (stateInfo->match->pRAI.willStartRunning[base] == 0) {
                    if (index != -1 && stateInfo->match->playerInfo[index].cPI.moving == 0) {
                        stateInfo->match->pRAI.willStartRunning[base] = 1;
                        if (base == BASE_FIRST || base == BASE_SECOND) {
                            lead_from_base(
                                stateInfo->match->playerInfo, stateInfo->match->playerRuntime,
                                stateInfo->fieldPositions, index
                            );
                        }
                    }
                } else {
                    stateInfo->match->pRAI.willStartRunning[base] = 0;
                }
            } else {
                stateInfo->match->pRAI.willStartRunning[base] = 0;
                if (index != -1) {
                    if (stateInfo->match->playerInfo[index].bTPI.state != PLAYER_STATE_ON_BASE &&
                        stateInfo->match->playerInfo[index].bTPI.state != PLAYER_STATE_AT_BAT) {
                        run_to_previous_base(stateInfo->match, stateInfo->fieldPositions, index, base);
                    }
                }
            }
            if (stateInfo->match->pendingActionState.doubleClickCounter[base] == -1) {
                stateInfo->match->pendingActionState.doubleClickCounter[base] = 0;
            } else {
                if (stateInfo->match->pendingActionState.doubleClickCounter[base] >= 0) {
                    if (index != -1) {
                        run_to_next_base(stateInfo->match, stateInfo->fieldPositions, index, base);
                    }
                }
                stateInfo->match->pendingActionState.doubleClickCounter[base] = -1;
            }
        }
    }
    stateInfo->match->aF.bTAF.baseRun[base] = ACTION_IDLE;
}

void update_meters(StateInfo* stateInfo)
{
    update_pitching_meter(stateInfo);

    if (stateInfo->match->pendingActionState.throwGoingOn == 1) {
        if (stateInfo->match->pendingActionState.meterCounter < stateInfo->match->pendingActionState.meterCounterMax) {
            stateInfo->match->pendingActionState.meterCounter += 1;
        }
        stateInfo->match->pRAI.meterValue = 1.0f * stateInfo->match->pendingActionState.meterCounter /
                                            stateInfo->match->pendingActionState.meterCounterMax;
    } else {
        update_batting_meter(stateInfo);
    }
}

void ai_update(StateInfo* stateInfo, unsigned int* rng_seed)
{
    int battingTeamIndex = get_batting_team_index(&stateInfo->rules->scoreboard);
    TeamControlMode battingControl = stateInfo->rules->scoreboard.teams[battingTeamIndex].control;
    TeamControlMode catchingControl = stateInfo->rules->scoreboard.teams[(battingTeamIndex + 1) % 2].control;

    // first ai for catching team

    if (team_is_ai(catchingControl)) {
        update_catching_ai(stateInfo, rng_seed);
    }
    // then ai for batting team
    if (team_is_ai(battingControl)) {
        update_batting_ai(stateInfo, rng_seed);
    }
}