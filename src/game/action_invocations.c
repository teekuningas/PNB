/*
    the main purpose of this code is to set flags for execute_actions when key combinations trigger some events.
    everything here is pretty straightforward.
*/

#include "globals.h"
#include "action_invocations.h"
#include "actions/throwing_system.h"
#include "rules_pure/player_utils.h"
#include "rules_pure/base_control.h"

// How long after a base-run key release a second release still counts as a double-press (= RUN_COMMIT).
#define RUN_DOUBLE_PRESS_WINDOW 20

// Ramp length (frames) of the human pitch sampling widget — how long it takes to sweep 0→full before
// wrapping. A pure feel knob (client-local); the engine windup clock is separate and authoritative.
#define PITCH_WIDGET_MAX 30

static int checkThrowCharge(
    MatchSession* match, ClientInputState* clientInput, const KeyStates* key_states, TeamControlMode control
);
static void checkDrop(MatchSession* match, const KeyStates* key_states, int key, TeamControlMode control);
static void
checkMove(MatchSession* match, const KeyStates* key_states, int key, TeamControlMode control, int direction);
static void checkChangePlayer(MatchSession* match, const KeyStates* key_states, int key, TeamControlMode control);
static void checkPitch(
    MatchSession* match, ClientInputState* clientInput, const KeyStates* key_states, int key, TeamControlMode control
);
static void
checkBatterSelection(MatchSession* match, const KeyStates* key_states, int change, int select, TeamControlMode control);
static void checkFreeWalkDecision(
    MatchSession* match, const KeyStates* key_states, int accept, int reject, TeamControlMode control
);
static void
checkBatterAngle(MatchSession* match, const KeyStates* key_states, int increase, int decrease, TeamControlMode control);
static void checkSwing(MatchSession* match, const KeyStates* key_states, int key, TeamControlMode control);
static void checkBattingTeamRun(
    MatchSession* match, ClientInputState* clientInput, const KeyStates* key_states, int key, TeamControlMode control,
    BaseID base, const RefereeState* referee
);

void init_action_invocations(StateInfo* stateInfo)
{
    // Placeholder for... future?
}

void action_invocations(
    MatchSession* match, ClientInputState* clientInput, const KeyStates* key_states, const Scoreboard* scoreboard,
    const RefereeState* referee
)
{
    int battingTeamIndex = get_batting_team_index(scoreboard);
    TeamControlMode battingControl = scoreboard->teams[battingTeamIndex].control;
    TeamControlMode catchingControl = scoreboard->teams[(battingTeamIndex + 1) % 2].control;

    // Tick down the human base-run double-press windows once per frame (see checkBattingTeamRun).
    for (int b = 0; b < BASE_COUNT; b++) {
        if (clientInput->run_press_window[b] > 0) clientInput->run_press_window[b]--;
    }

    // The action key (KEY_2) is shared. Held together with a direction key it charges a throw; held
    // alone it pitches / drops / changes. checkThrowCharge runs first and reports whether a throw gesture
    // owns KEY_2 this frame — if so we suppress the others, so declaring (or cancelling) a throw never
    // doubles as a drop/pitch on the same key edge.
    int throw_engaged = checkThrowCharge(match, clientInput, key_states, catchingControl);
    if (!throw_engaged) {
        if (match->pII.hasBallIndex == -1) {
            checkChangePlayer(match, key_states, KEY_2, catchingControl);
        } else if (match->pII.controlIndex != match->pII.catcherOnBaseIndex[0]) {
            checkDrop(match, key_states, KEY_2, catchingControl);
        } else {
            checkPitch(match, clientInput, key_states, KEY_2, catchingControl);
        }
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

    checkBattingTeamRun(match, clientInput, key_states, KEY_DOWN, battingControl, BASE_HOME, referee);
    checkBattingTeamRun(match, clientInput, key_states, KEY_LEFT, battingControl, BASE_FIRST, referee);
    checkBattingTeamRun(match, clientInput, key_states, KEY_RIGHT, battingControl, BASE_SECOND, referee);
    checkBattingTeamRun(match, clientInput, key_states, KEY_UP, battingControl, BASE_THIRD, referee);
}

// Human throw-charge gesture (client-local input → ThrowIntent). Returns 1 if a throw gesture owns the
// action key (KEY_2) this frame — the caller then suppresses pitch/drop/change so the same key edge is
// not consumed twice.
//
// The gesture, all driven by KEY_2 (see ThrowCharge in globals.h):
//   - KEY_2 held + a direction key → start the gesture and latch that base; the meter charges while KEY_2
//     is held. Pressing a different direction REDIRECTS (and restarts the meter).
//   - Once latched, the direction stays latched for the rest of the hold — releasing the arrow does NOT
//     cancel (you can release the arrow first, then KEY_2, and the throw still fires). There is no cancel.
//   - KEY_2 released → declare ThrowIntent{ latched base, power=charge }.
// The AI never charges — it declares the ThrowIntent atomically — so AI control returns early here.
static int checkThrowCharge(
    MatchSession* match, ClientInputState* clientInput, const KeyStates* key_states, TeamControlMode control
)
{
    if (control == CONTROL_AI) {
        return 0; // AI declares the throw command directly in catching_ai.c
    }

    // Direction key per target base (index by BaseID: HOME/FIRST/SECOND/THIRD).
    static const int throwKeyForBase[BASE_COUNT] = {KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_UP};

    ThrowCharge* tc = &clientInput->throw_charge;
    int enterDown = key_states->down[control][KEY_2];
    int enterReleased = key_states->released[control][KEY_2];

    int canThrow =
        match->pII.hasBallIndex != -1 && match->pendingActionState.current_catching_action == CATCHING_ACTION_NONE;

    // Disengage entirely if the action key is not involved this frame, or a throw is no longer possible.
    if ((!enterDown && !enterReleased) || !canThrow) {
        tc->base = BASE_NONE;
        tc->power = 0;
        tc->engaged = 0;
        return 0;
    }

    // Count held direction keys and remember the single one (if exactly one).
    int arrowsHeld = 0;
    BaseID heldBase = BASE_NONE;
    for (BaseID b = 0; b < BASE_COUNT; b++) {
        if (key_states->down[control][throwKeyForBase[b]]) {
            arrowsHeld++;
            heldBase = b;
        }
    }

    if (enterReleased) {
        // Declare for the latched direction (NOT the keys held this exact frame — releasing the arrow and
        // KEY_2 together must still throw). A cancel already cleared tc->base, so it declares nothing then.
        if (tc->engaged && tc->base != BASE_NONE && match->aF.cTAF.throw.target == BASE_NONE) {
            match->aF.cTAF.throw.target = tc->base;
            match->aF.cTAF.throw.power = throw_charge_to_power(tc->power);
        }
        int wasEngaged = tc->engaged;
        tc->base = BASE_NONE;
        tc->power = 0;
        tc->engaged = 0;
        return wasEngaged; // a gesture-owned release must not also drop/pitch
    }

    // KEY_2 is held and a throw is possible. A single fresh direction starts the gesture or redirects it
    // (restarting the meter); after that the direction stays LATCHED — releasing the arrow does NOT
    // cancel, it only stops redirecting. Power keeps building from the KEY_2 hold until release.
    if (arrowsHeld == 1 && heldBase != tc->base) {
        tc->base = heldBase; // start, or switch direction → restart the meter
        tc->power = 0;
        tc->engaged = 1;
        return 1;
    }
    if (tc->engaged) {
        // Gesture in progress (same direction held, or arrow released, or an ambiguous multi-press) —
        // keep charging the latched direction. No cancel: the throw resolves on the KEY_2 release.
        if (tc->power < THROW_CHARGE_MAX) tc->power++;
        return 1;
    }

    // KEY_2 held alone, no direction ever pressed → not a throw gesture; let the caller pitch/drop.
    return 0;
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

// Human pitch input — the 3-click dance, each click advancing the phased PitchDeclaration:
//   click 1 (start) → WINDUP: begin the windup; the sampling widget starts ramping.
//   click 2 (power) → POWER:  sample the widget → declared power [0,1]; widget restarts for the aim.
//   click 3 (aim)   → AIMED:  sample the widget → declared direction [-1,1] (mid = on the plate / strike).
// If the human never reaches AIMED before the engine windup elapses, it resolves to a valesyöttö (the
// engine's job — see update_pitch_actualization). The widget is client-local input interpretation only;
// the AI declares values directly and never touches it.
static void checkPitch(
    MatchSession* match, ClientInputState* clientInput, const KeyStates* key_states, int key, TeamControlMode control
)
{
    if (control == CONTROL_AI) {
        return; // AI declares the pitch directly in catching_ai.c
    }

    InputWidget* w = &clientInput->pitchWidget;
    PitchDeclaration* decl = &match->aF.cTAF.pitch;

    // Ramp the sampling widget while the human is still gathering (between clicks).
    int gathering = (decl->phase == PITCH_DECL_WINDUP || decl->phase == PITCH_DECL_POWER);
    if (gathering && w->counter_max > 0) {
        w->counter++;
        if (w->counter > w->counter_max) w->counter = 0; // sawtooth — time the click to catch the value
    }

    if (key_states->released[control][key] != 1) {
        return; // declarations happen on the click (the release edge)
    }

    if (decl->phase == PITCH_DECL_IDLE && match->pendingActionState.current_catching_action == CATCHING_ACTION_NONE) {
        // click 1: begin the windup; start sampling.
        decl->phase = PITCH_DECL_WINDUP;
        w->counter = 0;
        w->counter_max = PITCH_WIDGET_MAX;
    } else if (decl->phase == PITCH_DECL_WINDUP) {
        // click 2: declare power from the widget; restart the widget for the aim.
        decl->power = (float)w->counter / (float)w->counter_max; // [0,1]
        decl->phase = PITCH_DECL_POWER;
        w->counter = 0;
    } else if (decl->phase == PITCH_DECL_POWER) {
        // click 3: declare direction from the widget (mid = on the plate → strike).
        decl->direction = 2.0f * ((float)w->counter / (float)w->counter_max) - 1.0f; // [-1,1]
        decl->phase = PITCH_DECL_AIMED;
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
    MatchSession* match, ClientInputState* clientInput, const KeyStates* key_states, int key, TeamControlMode control,
    BaseID base, const RefereeState* referee
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
                if (clientInput->run_press_window[base] > 0) {
                    // Second press inside the window → deliberate "run now".
                    match->aF.bTAF.base_run[base] = RUN_COMMIT;
                    clientInput->run_press_window[base] = 0;
                } else {
                    // First press → arm (settled) or come back (off the base); open the double-press window.
                    PlayerUnitState state = match->playerInfo[index].bTPI.state;
                    if (state == PLAYER_STATE_ON_BASE || state == PLAYER_STATE_AT_BAT) {
                        match->aF.bTAF.base_run[base] = RUN_FORWARD;
                    } else {
                        match->aF.bTAF.base_run[base] = RUN_BACK;
                    }
                    clientInput->run_press_window[base] = RUN_DOUBLE_PRESS_WINDOW;
                }
            }
        }
    } else {
        // AI declares the run command directly in batting_ai.c
    }
}
