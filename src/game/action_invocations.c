/*
    the main purpose of this code is to set flags for execute_actions when key combinations trigger some events.
    everything here is pretty straightforward.
*/

#include "globals.h"
#include "action_invocations.h"
#include "rules_pure/player_utils.h"
#include "rules_pure/base_control.h"

// How long after a base-run key release a second release still counts as a double-press (= RUN_COMMIT).
#define RUN_DOUBLE_PRESS_WINDOW 20

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
static void checkBattingTeamRun(
    MatchSession* match, const KeyStates* key_states, int key, TeamControlMode control, BaseID base,
    const RefereeState* referee
);

void init_action_invocations(StateInfo* stateInfo)
{
    // Placeholder for... future?
}

void action_invocations(
    MatchSession* match, const KeyStates* key_states, const Scoreboard* scoreboard, const RefereeState* referee
)
{
    int battingTeamIndex = get_batting_team_index(scoreboard);
    TeamControlMode battingControl = scoreboard->teams[battingTeamIndex].control;
    TeamControlMode catchingControl = scoreboard->teams[(battingTeamIndex + 1) % 2].control;

    // Tick down the human base-run double-press windows once per frame (see checkBattingTeamRun).
    for (int b = 0; b < BASE_COUNT; b++) {
        if (match->pendingActionState.run_press_window[b] > 0) match->pendingActionState.run_press_window[b]--;
    }

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

    checkBattingTeamRun(match, key_states, KEY_DOWN, battingControl, BASE_HOME, referee);
    checkBattingTeamRun(match, key_states, KEY_LEFT, battingControl, BASE_FIRST, referee);
    checkBattingTeamRun(match, key_states, KEY_RIGHT, battingControl, BASE_SECOND, referee);
    checkBattingTeamRun(match, key_states, KEY_UP, battingControl, BASE_THIRD, referee);
}

static void checkThrow(
    MatchSession* match, const KeyStates* key_states, int key, int actionKey, TeamControlMode control, BaseID base
)
{
    if (control != CONTROL_AI) {
        if (key_states->down[control][key] && key_states->down[control][actionKey]) {
            if (match->aF.cTAF.throw_to_base[base] == ACTION_IDLE &&
                match->pendingActionState.current_catching_action == CATCHING_ACTION_NONE) {
                match->aF.cTAF.throw_to_base[base] = ACTION_TRIGGER_START;
            }
        } else if ((key_states)->released[control][actionKey] &&
                   match->pendingActionState.current_catching_action == CATCHING_ACTION_THROWING) {
            match->aF.cTAF.throw_to_base[base] = ACTION_TRIGGER_STOP;
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
        if (match->pendingActionState.current_catching_action == CATCHING_ACTION_NONE) {
            if (key_states->released[control][key] == 1) {
                if (match->aF.cTAF.change_player == ACTION_IDLE) {
                    match->aF.cTAF.change_player = ACTION_TRIGGER_START;
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
        if (match->pendingActionState.current_catching_action == CATCHING_ACTION_NONE) {
            if (key_states->released[control][key] == 1) {
                if (match->aF.cTAF.drop_ball == ACTION_IDLE) {
                    match->aF.cTAF.drop_ball = ACTION_TRIGGER_START;
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
            if (match->pendingActionState.pitch_phase == PITCH_PHASE_ANGLE_WAIT) {
                match->aF.cTAF.pitch = PITCH_ACTION_ANGLE_SET;
            } else if (match->pendingActionState.current_catching_action == CATCHING_ACTION_NONE) {
                match->aF.cTAF.pitch = PITCH_ACTION_START;
            }
        } else if (key_states->released[control][key] == 1) {
            if (match->pendingActionState.pitch_phase == PITCH_PHASE_POWER_WAIT) {
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

            if (match->aF.bTAF.choose_batter == CHOOSE_BATTER_IDLE) {
                match->aF.bTAF.choose_batter = CHOOSE_BATTER_NEXT;
            }
        } else if (key_states->released[control][select] == 1) {
            if (match->aF.bTAF.choose_batter == CHOOSE_BATTER_IDLE) {
                match->aF.bTAF.choose_batter = CHOOSE_BATTER_SELECT;
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

            if (match->aF.bTAF.take_free_walk == FREE_WALK_IDLE) {
                match->aF.bTAF.take_free_walk = FREE_WALK_ACCEPT;
            }
        } else if (key_states->released[control][reject] == 1) {
            if (match->aF.bTAF.take_free_walk == FREE_WALK_IDLE) {
                match->aF.bTAF.take_free_walk = FREE_WALK_REJECT;
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
            if (match->pRAI.batting_going_on == 1) {
                if (match->aF.bTAF.increase_batter_angle == ACTION_IDLE) {
                    match->aF.bTAF.increase_batter_angle = ACTION_TRIGGER_START;
                }
            }
        } else if (key_states->released[control][increase] == 1) {
            if (match->pRAI.batting_going_on == 1) {
                if (match->aF.bTAF.increase_batter_angle != ACTION_IDLE) {
                    match->aF.bTAF.increase_batter_angle = ACTION_TRIGGER_STOP;
                }
            }
        }
        if (key_states->down[control][decrease] == 1) {
            if (match->pRAI.batting_going_on == 1) {
                if (match->aF.bTAF.decrease_batter_angle == ACTION_IDLE) {
                    match->aF.bTAF.decrease_batter_angle = ACTION_TRIGGER_START;
                }
            }
        } else if (key_states->released[control][decrease] == 1) {
            if (match->pRAI.batting_going_on == 1) {
                if (match->aF.bTAF.decrease_batter_angle != ACTION_IDLE) {
                    match->aF.bTAF.decrease_batter_angle = ACTION_TRIGGER_STOP;
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

static void checkBattingTeamRun(
    MatchSession* match, const KeyStates* key_states, int key, TeamControlMode control, BaseID base,
    const RefereeState* referee
)
{
    if (control != CONTROL_AI) {
        // One key per base, mapping the human's single/double press to the explicit RunIntent
        // (globals.h): a SINGLE tap = RUN_FORWARD ("arm / lead") on a settled runner, or RUN_BACK
        // ("come back") on a runner who has stepped off / is mid-run; a DOUBLE tap (a second release
        // within run_press_window frames) = RUN_COMMIT ("run now" — the steal / leg it to the next
        // base). The engine actualizes for the runner's exact state (see execute_actions::base_run).
        // This is honest input→intent interpretation in the input layer — not game logic, not an AI
        // click-sim (the AI declares RUN_COMMIT directly in batting_ai.c).
        if (key_states->released[control][key]) {
            int index = get_base_controller(match, referee, base);
            if (index != -1) {
                if (match->pendingActionState.run_press_window[base] > 0) {
                    // Second press inside the window → deliberate "run now".
                    match->aF.bTAF.base_run[base] = RUN_COMMIT;
                    match->pendingActionState.run_press_window[base] = 0;
                } else {
                    // First press → arm (settled) or come back (off the base); open the double-press window.
                    PlayerUnitState state = match->playerInfo[index].bTPI.state;
                    if (state == PLAYER_STATE_ON_BASE || state == PLAYER_STATE_AT_BAT) {
                        match->aF.bTAF.base_run[base] = RUN_FORWARD;
                    } else {
                        match->aF.bTAF.base_run[base] = RUN_BACK;
                    }
                    match->pendingActionState.run_press_window[base] = RUN_DOUBLE_PRESS_WINDOW;
                }
            }
        }
    } else {
        // AI declares the run command directly in batting_ai.c
    }
}
