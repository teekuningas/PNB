/*
    the main purpose of this code is to set flags for execute_actions when key combinations trigger some events.
    everything here is pretty straightforward.
*/

#include "globals.h"
#include "action_invocations.h"
#include "rules_pure/player_utils.h"

static void checkThrow(
    MatchSession* match, const KeyStates* key_states, int key, int actionKey, TeamControlMode control, BaseID base
);
static void checkDrop(MatchSession* match, const KeyStates* key_states, int key, TeamControlMode control);
static void
checkMove(MatchSession* match, const KeyStates* key_states, int key, TeamControlMode control, int direction);
static void checkChangePlayer(MatchSession* match, const KeyStates* key_states, int key, TeamControlMode control);
static void checkPitch(MatchSession* match, const KeyStates* key_states, int key, TeamControlMode control);
static void
checkBatterSelection(MatchSession* match, const KeyStates* key_states, int change, int select, TeamControlMode control);
static void checkFreeWalkDecision(
    MatchSession* match, const KeyStates* key_states, int accept, int reject, TeamControlMode control
);
static void
checkBatterAngle(MatchSession* match, const KeyStates* key_states, int increase, int decrease, TeamControlMode control);
static void checkSwing(MatchSession* match, const KeyStates* key_states, int key, TeamControlMode control);
static void
checkBattingTeamRun(MatchSession* match, const KeyStates* key_states, int key, TeamControlMode control, BaseID base);

void init_action_invocations(StateInfo* stateInfo)
{
    // Placeholder for... future?
}

void action_invocations(MatchSession* match, const KeyStates* key_states, const Scoreboard* scoreboard)
{
    int battingTeamIndex = get_batting_team_index(scoreboard);
    TeamControlMode battingControl = scoreboard->teams[battingTeamIndex].control;
    TeamControlMode catchingControl = scoreboard->teams[(battingTeamIndex + 1) % 2].control;

    checkThrow(match, key_states, KEY_DOWN, KEY_2, catchingControl, BASE_HOME);
    checkThrow(match, key_states, KEY_LEFT, KEY_2, catchingControl, BASE_FIRST);
    checkThrow(match, key_states, KEY_RIGHT, KEY_2, catchingControl, BASE_SECOND);
    checkThrow(match, key_states, KEY_UP, KEY_2, catchingControl, BASE_THIRD);

    if (match->pII.hasBallIndex == -1) {
        checkChangePlayer(match, key_states, KEY_2, catchingControl);
    } else if (match->pII.controlIndex != match->pII.catcherOnBaseIndex[0]) {
        checkDrop(match, key_states, KEY_2, catchingControl);
    } else {
        checkPitch(match, key_states, KEY_2, catchingControl);
    }

    checkMove(match, key_states, KEY_UP, catchingControl, 0);
    checkMove(match, key_states, KEY_RIGHT, catchingControl, 1);
    checkMove(match, key_states, KEY_DOWN, catchingControl, 2);
    checkMove(match, key_states, KEY_LEFT, catchingControl, 3);

    // check these only if necessary. also if it happened to be so that
    // they are both asked the same time, choose the free walk first
    if (match->flowControl.waitingForFreeWalkDecision == 1) {
        checkFreeWalkDecision(match, key_states, KEY_2, KEY_1, battingControl);
    } else if (match->flowControl.waitingForBatterDecision == 1) {
        checkBatterSelection(match, key_states, KEY_1, KEY_2, battingControl);
    }
    checkBatterAngle(match, key_states, KEY_PLUS, KEY_MINUS, battingControl);
    checkSwing(match, key_states, KEY_2, battingControl);

    checkBattingTeamRun(match, key_states, KEY_DOWN, battingControl, BASE_HOME);
    checkBattingTeamRun(match, key_states, KEY_LEFT, battingControl, BASE_FIRST);
    checkBattingTeamRun(match, key_states, KEY_RIGHT, battingControl, BASE_SECOND);
    checkBattingTeamRun(match, key_states, KEY_UP, battingControl, BASE_THIRD);
}

static void checkThrow(
    MatchSession* match, const KeyStates* key_states, int key, int actionKey, TeamControlMode control, BaseID base
)
{
    if (control != CONTROL_AI) {
        if (key_states->down[control][key] && key_states->down[control][actionKey]) {
            if (match->aF.cTAF.throwToBase[base] == ACTION_IDLE) {
                match->aF.cTAF.throwToBase[base] = ACTION_TRIGGER_START;
                // prevent change player event or drop event from happening when we are throwing
                match->aF.cTAF.actionKeyLock = 1;
            }
        } else if ((key_states)->released[control][actionKey] &&
                   (match->aF.cTAF.throwToBase[base] == ACTION_ACTIVE ||
                    match->aF.cTAF.throwToBase[base] == ACTION_TRIGGER_START)) {
            match->aF.cTAF.throwToBase[base] = ACTION_TRIGGER_STOP;
        }
    } else {
        // AI sets flags directly in AI logic files
    }
}

static void checkMove(MatchSession* match, const KeyStates* key_states, int key, TeamControlMode control, int direction)
{
    if (control != CONTROL_AI) {
        if (key_states->down[control][key] == 1 && key_states->down[control][KEY_2] == 0) {
            if (match->aF.cTAF.move[direction] == ACTION_IDLE) {
                match->aF.cTAF.move[direction] = ACTION_TRIGGER_START;
            }
        } else if (key_states->released[control][key] == 1 ||
                   (key_states->down[control][key] == 1 && key_states->down[control][KEY_2] == 1)) {
            if (match->aF.cTAF.move[direction] !=
                ACTION_IDLE) { // to avoid something weird when this is changed to 1 when ball is catched
                match->aF.cTAF.move[direction] = ACTION_TRIGGER_STOP;
            }
        }
    } else {
        // AI sets flags directly
    }
}

static void checkChangePlayer(MatchSession* match, const KeyStates* key_states, int key, TeamControlMode control)
{
    if (control != CONTROL_AI) {
        if (match->aF.cTAF.actionKeyLock == 0) {
            if (key_states->released[control][key] == 1) {
                if (match->aF.cTAF.change_player == ACTION_IDLE) {
                    match->aF.cTAF.change_player = ACTION_TRIGGER_START;
                    match->aF.cTAF.actionKeyLock = 1;
                }
            }
        }
    } else {
        // AI sets flags directly
    }
}

static void checkDrop(MatchSession* match, const KeyStates* key_states, int key, TeamControlMode control)
{
    if (control != CONTROL_AI) {
        if (match->aF.cTAF.actionKeyLock == 0) {
            if (key_states->released[control][key] == 1) {
                if (match->aF.cTAF.dropBall == ACTION_IDLE) {
                    match->aF.cTAF.dropBall = ACTION_TRIGGER_START;
                    match->aF.cTAF.actionKeyLock = 1;
                }
            }
        }
    } else {
        // AI sets flags directly
    }
}

static void checkPitch(MatchSession* match, const KeyStates* key_states, int key, TeamControlMode control)
{
    if (control != CONTROL_AI) {
        if (key_states->down[control][key] == 1) {
            if (match->aF.cTAF.pitch == PITCH_ACTION_IDLE) {
                if (match->aF.cTAF.actionKeyLock == 0) {
                    match->aF.cTAF.pitch = PITCH_ACTION_START;
                    match->aF.cTAF.actionKeyLock = 1;
                }
            } else if (match->aF.cTAF.pitch == PITCH_ACTION_ANGLE_WAIT) {
                match->aF.cTAF.pitch = PITCH_ACTION_ANGLE_SET;
            }
        } else if (key_states->released[control][key] == 1) {
            if (match->aF.cTAF.pitch == PITCH_ACTION_POWER_WAIT) {
                match->aF.cTAF.pitch = PITCH_ACTION_POWER_SET;
            }
        }
    } else {
        // AI sets flags directly
    }
}

static void
checkBatterSelection(MatchSession* match, const KeyStates* key_states, int change, int select, TeamControlMode control)
{
    if (control != CONTROL_AI) {
        if (key_states->released[control][change] == 1) {

            if (match->aF.bTAF.chooseBatter == CHOOSE_BATTER_IDLE) {
                match->aF.bTAF.chooseBatter = CHOOSE_BATTER_NEXT;
            }
        } else if (key_states->released[control][select] == 1) {
            if (match->aF.bTAF.chooseBatter == CHOOSE_BATTER_IDLE) {
                match->aF.bTAF.chooseBatter = CHOOSE_BATTER_SELECT;
            }
        }
    } else {
        // AI sets flags directly
    }
}

static void
checkFreeWalkDecision(MatchSession* match, const KeyStates* key_states, int accept, int reject, TeamControlMode control)
{
    if (control != CONTROL_AI) {
        if (key_states->released[control][accept] == 1) {

            if (match->aF.bTAF.takeFreeWalk == FREE_WALK_IDLE) {
                match->aF.bTAF.takeFreeWalk = FREE_WALK_ACCEPT;
            }
        } else if (key_states->released[control][reject] == 1) {
            if (match->aF.bTAF.takeFreeWalk == FREE_WALK_IDLE) {
                match->aF.bTAF.takeFreeWalk = FREE_WALK_REJECT;
            }
        }
    } else {
        // AI sets flags directly
    }
}

static void
checkBatterAngle(MatchSession* match, const KeyStates* key_states, int increase, int decrease, TeamControlMode control)
{
    if (control != CONTROL_AI) {
        if (key_states->down[control][increase] == 1) {
            if (match->pRAI.battingGoingOn == 1) {
                if (match->aF.bTAF.increaseBatterAngle == ACTION_IDLE) {
                    match->aF.bTAF.increaseBatterAngle = ACTION_TRIGGER_START;
                }
            }
        } else if (key_states->released[control][increase] == 1) {
            if (match->pRAI.battingGoingOn == 1) {
                if (match->aF.bTAF.increaseBatterAngle != ACTION_IDLE) {
                    match->aF.bTAF.increaseBatterAngle = ACTION_TRIGGER_STOP;
                }
            }
        }
        if (key_states->down[control][decrease] == 1) {
            if (match->pRAI.battingGoingOn == 1) {
                if (match->aF.bTAF.decreaseBatterAngle == ACTION_IDLE) {
                    match->aF.bTAF.decreaseBatterAngle = ACTION_TRIGGER_START;
                }
            }
        } else if (key_states->released[control][decrease] == 1) {
            if (match->pRAI.battingGoingOn == 1) {
                if (match->aF.bTAF.decreaseBatterAngle != ACTION_IDLE) {
                    match->aF.bTAF.decreaseBatterAngle = ACTION_TRIGGER_STOP;
                }
            }
        }
    } else {
        // AI sets flags directly
    }
}

static void checkSwing(MatchSession* match, const KeyStates* key_states, int key, TeamControlMode control)
{
    if (control != CONTROL_AI) {
        if (key_states->down[control][key] == 1) {
            if (match->aF.bTAF.swing == BAT_ACTION_WAIT_FOR_BALL) {
                match->aF.bTAF.swing = BAT_ACTION_POWER_SET;
            }
        } else if (key_states->released[control][key] == 1) {
            if (match->aF.bTAF.swing == BAT_ACTION_ANGLE_WAIT) {
                match->aF.bTAF.swing = BAT_ACTION_ANGLE_SET;
            }
        }
    } else {
        // AI sets flags directly
    }
}

static void
checkBattingTeamRun(MatchSession* match, const KeyStates* key_states, int key, TeamControlMode control, BaseID base)
{
    if (control != CONTROL_AI) {
        if ((key_states)->released[control][key]) {
            match->aF.bTAF.baseRun[base] = ACTION_TRIGGER_START;
        }
    } else {
        // AI sets flags directly in AI logic files
    }
}
