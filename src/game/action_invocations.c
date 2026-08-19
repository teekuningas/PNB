/*
    the main purpose of this code is to set flags for execute_actions when key combinations trigger some events.
    everything here is pretty straightforward.
*/

#include "globals.h"
#include "action_invocations.h"
#include "actions/throwing_system.h"
#include "actions/pitching_system.h" // windup segment constants — the aim descent matches the engine windup
#include "rules_pure/player_utils.h"
#include "rules_pure/base_control.h"

// How long after a base-run key release a second release still counts as a double-press (= RUN_COMMIT).
#define RUN_DOUBLE_PRESS_WINDOW 20

// Human pitch meter feel knobs (client-local; the engine windup clock is separate and authoritative).
#define PITCH_POWER_WIDGET_MAX 30 // power ping-pong half-length (frames); pre-windup & untimed — right peak = max power
#define PITCH_AIM_FOCAL                                                                                                \
    0.31f // cursor fraction of the on-the-plate strike (the meter's warm spot ≈ legacy
          // 4/13); direction is 0 here
#define PITCH_AIM_SCALE 1.45f // maps the [0,1] cursor to direction [-1,1] about FOCAL (≈ 1/(1-FOCAL))
// The aim meter is a one-way right→left descent whose length matches the pre-throw windup (crouch DOWN +
// power-scaled HOLD), so the cursor reaches the left exactly as the throw (UP) begins — an honest aim lands
// in time, a missed one rests at the left (valesyöttö). Sized from the windup CONSTANTS (client-side, not the
// live engine clock), so it stays in sync.

// Human throw charge meter feel knob (client-local). The charge widget rises for CHARGE_FRAMES then holds
// at full; a release samples it to a declared power in [THROW_POWER_MIN, 1]. Independent of the AI windup
// length by design (§5.9 — the engine clock drives only the render gather animation), kept close to
// THROW_WINDUP_MAX_FRAMES for visual coherence.
#define THROW_CHARGE_FRAMES 45 // frames to reach full charge on a held throw (~0.9s at 50Hz)

static int checkThrowGesture(
    MatchSession* match, ClientInputState* clientInput, const KeyStates* key_states, TeamControlMode control
);
static void advance_widget(InputWidget* w);
static float charge_to_power(int counter, int counter_max);
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

    // Retire a finished pitch widget EVERY frame, not only inside checkPitch (which stops being called the
    // instant the ball leaves the pitcher's hand). A power ping-pong that ran out cancels; an aim widget
    // left over once the engine cleared the pitch (released or faked) resets. Without this the widget stays
    // "active" forever after a pitch, so update_meters keeps showing it instead of advancing the batting
    // meter — starving the batter's swing power to ~0 (the "hit ball floats at the plate" bug).
    {
        InputWidget* w = &clientInput->pitchWidget;
        if (w->mode == WIDGET_PING_PONG && w->dir == 0) {
            w->mode = WIDGET_IDLE;
            w->counter_max = 0;
        } else if (w->mode == WIDGET_DESCENT && match->aF.cTAF.pitch.phase == PITCH_DECL_IDLE) {
            w->mode = WIDGET_IDLE;
            w->counter_max = 0;
            w->dir = 0;
        }
    }

    // Retire the throw charge widget whenever the declaration is back to IDLE, whatever cleared it.
    // Must run here rather than inside checkThrowGesture, which stops firing the moment the ball leaves
    // the hand — the same gap that caused bug #4 on the pitch widget. An AI throw arms none of this.
    {
        InputWidget* w = &clientInput->throwWidget;
        if (w->mode != WIDGET_IDLE && match->aF.cTAF.throw.phase == THROW_DECL_IDLE) {
            w->mode = WIDGET_IDLE;
            w->counter_max = 0;
            w->dir = 0;
        }
    }

    // The action key (KEY_2) is shared. Held together with a direction key it winds up a throw (hold to
    // gather, release to fire); held alone it pitches / drops / changes. checkThrowGesture runs first and
    // reports whether a throw gesture owns KEY_2 this frame — if so we suppress the others, so a throw's
    // hold or release never doubles as a drop/pitch on the same key edge.
    int throw_engaged = checkThrowGesture(match, clientInput, key_states, catchingControl);
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

// Human throw gesture (hold-release) → the phased ThrowDeclaration (cTAF.throw), with the power declared as
// a VALUE sampled from a client-local CHARGE widget (engine↔client contract §8.7 — the input NEVER reads the
// engine windup clock; that clock drives only the render gather animation). Returns 1 if a throw gesture
// owns the action key (KEY_2) this frame — the caller then suppresses pitch/drop/change so one key edge is
// not consumed twice.
//
//   KEY_2 held + exactly one direction (and a throw can begin) → declare INITIATED with that base and START
//     the charge widget: the engine begins the windup (its clock drives the gather animation) and the widget
//     starts rising. The direction is latched (the windup has physically begun — no redirect).
//   KEY_2 released while INITIATED → SAMPLE the charge widget → complete the intent as COMMITTED{target,
//     power}: a longer hold charged the widget further = a harder throw. A bare tap still throws, at floor
//     power. The widget is retired here (its value is now captured on the declaration). The engine — not the
//     client — decides WHEN the ball leaves: once COMMITTED it releases when its windup clock reaches
//     windup(power), which (having run since INITIATED, concurrently with the human's power pick) is usually
//     already elapsed → immediate. This is the same COMMITTED the AI declares, just reached in two frames.
// The AI never uses this — it declares COMMITTED directly in catching_ai.c — so AI control returns early.
static int checkThrowGesture(
    MatchSession* match, ClientInputState* clientInput, const KeyStates* key_states, TeamControlMode control
)
{
    if (control == CONTROL_AI) {
        return 0; // AI declares the throw COMMITTED directly in catching_ai.c
    }

    // Direction key per target base (index by BaseID: HOME/FIRST/SECOND/THIRD).
    static const int throwKeyForBase[BASE_COUNT] = {KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_UP};
    ThrowDeclaration* decl = &match->aF.cTAF.throw;
    InputWidget* w = &clientInput->throwWidget;

    // The intent is INITIATED (direction declared, power pending): keep charging the client widget while
    // held; a KEY_2 release samples it and completes the intent (COMMITTED). The gesture owns KEY_2 for the
    // whole hold, so a lone arrow release does not cancel and the KEY_2 release does not also drop.
    if (decl->phase == THROW_DECL_INITIATED) {
        advance_widget(w); // client-local charge; independent of the engine clock
        if (key_states->released[control][KEY_2]) {
            decl->power = charge_to_power(w->counter, w->counter_max); // pass the meter's value in
            decl->phase = THROW_DECL_COMMITTED; // full intent assembled — the engine times the release
            w->mode = WIDGET_IDLE; // captured into the declaration — retire the widget
            w->counter_max = 0;
            w->dir = 0;
        }
        return 1;
    }

    // Not started yet — initiate a throw only if the action key is held together with exactly one direction,
    // a ball-holder is controlled, and no catching action is already in progress.
    if (decl->phase == THROW_DECL_IDLE && key_states->down[control][KEY_2] && match->pII.hasBallIndex != -1 &&
        match->pendingActionState.current_catching_action == CATCHING_ACTION_NONE) {
        int arrowsHeld = 0;
        BaseID heldBase = BASE_NONE;
        for (BaseID b = 0; b < BASE_COUNT; b++) {
            if (key_states->down[control][throwKeyForBase[b]]) {
                arrowsHeld++;
                heldBase = b;
            }
        }
        if (arrowsHeld == 1) {
            decl->phase = THROW_DECL_INITIATED; // "I started, toward this base"; power pending (second frame)
            decl->target = heldBase;
            decl->power = 0.0f;
            // Start the charge widget: rise from 0 and hold at max (sampled on release).
            w->mode = WIDGET_CHARGE;
            w->counter = 0;
            w->counter_max = THROW_CHARGE_FRAMES;
            w->dir = 1;
            return 1;
        }
    }
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

// Advance a sampling widget's cursor one frame, per its mode (client-local input interpretation only; never
// reads or feeds an engine clock). PING_PONG bounces off the top edge (0 → max → 0 then stop); DESCENT and
// CHARGE do not bounce — DESCENT starts at the top and stops at 0, CHARGE rises and HOLDS at the top (the
// human keeps holding until release). All reach a terminal edge and stop (dir = 0), except PING_PONG which
// reverses at the top.
static void advance_widget(InputWidget* w)
{
    if (w->dir == 0) return;
    w->counter += w->dir;
    if (w->counter >= w->counter_max) {
        w->counter = w->counter_max;
        w->dir = (w->mode == WIDGET_PING_PONG) ? -1 : 0; // ping-pong bounces; charge holds at the top
    } else if (w->counter <= 0) {
        w->counter = 0;
        w->dir = 0; // reached the bottom — stop (a ping-pong that ran out unclicked, or a finished descent)
    }
}

// Map a throw charge widget reading to a declared power in [THROW_POWER_MIN, 1] — the client-side value the
// human declares on release (mirroring the AI's declared power). Floored so a bare tap still throws.
static float charge_to_power(int counter, int counter_max)
{
    if (counter_max <= 0) return THROW_POWER_MIN;
    float f = (float)counter / (float)counter_max; // [0,1]
    if (f < 0.0f) f = 0.0f;
    if (f > 1.0f) f = 1.0f;
    return THROW_POWER_MIN + (1.0f - THROW_POWER_MIN) * f;
}

// Human pitch input — a power meter then an aim meter, one click each, advancing the phased
// PitchDeclaration. Power selection is pre-windup and client-local (the declaration stays IDLE); the power
// click is what STARTS the engine windup (power-as-start), so there is no separate WINDUP phase:
//   press KEY_2 (idle)        → start the power ping-pong (0 → max → 0); nothing declared yet.
//   click during power sweep  → power = cursor fraction (right peak = max); phase POWER (engine windup
//                               begins); start the aim meter descending from the right.
//   click during aim descent  → direction = (cursor − FOCAL)·SCALE (FOCAL = on the plate / strike); AIMED.
// A meter that runs out unclicked: the power ping-pong cancels (press again to retry); the aim descent
// rests at the left and the engine windup elapses → valesyöttö (FAKE) — see update_pitch_actualization.
// The widget is client-local input interpretation only; the AI declares values directly and never touches it.
static void checkPitch(
    MatchSession* match, ClientInputState* clientInput, const KeyStates* key_states, int key, TeamControlMode control
)
{
    if (control == CONTROL_AI) {
        return; // AI declares the pitch directly in catching_ai.c
    }

    InputWidget* w = &clientInput->pitchWidget;
    PitchDeclaration* decl = &match->aF.cTAF.pitch;

    // (The widget is retired in action_invocations every frame — see the top of that function — so it
    // frees the batting meter even after the ball leaves the pitcher's hand and checkPitch goes quiet.)
    advance_widget(w); // move the cursor (if running) before reading a click

    if (key_states->released[control][key] != 1) {
        return; // declarations happen on the click (the release edge)
    }

    if (decl->phase == PITCH_DECL_IDLE) {
        if (w->mode == WIDGET_IDLE) {
            // this click STARTS the power meter (client-local; the declaration stays IDLE until power is
            // locked). Only when a pitch could legally begin.
            if (match->pendingActionState.current_catching_action == CATCHING_ACTION_NONE) {
                w->mode = WIDGET_PING_PONG;
                w->counter = 0;
                w->counter_max = PITCH_POWER_WIDGET_MAX;
                w->dir = 1; // ping-pong: rise first
            }
        } else if (w->mode == WIDGET_PING_PONG) {
            // click while the power meter sweeps → lock power (right peak = max) and START the windup.
            decl->power = (float)w->counter / (float)w->counter_max; // [0,1]
            decl->phase = PITCH_DECL_POWER;
            // start the aim meter: a one-way descent from the right, its length matching the pre-throw
            // windup (crouch + power-scaled hold) so the cursor reaches the left as the throw begins.
            int aim_len = PITCH_WINDUP_DOWN_FRAMES + (int)(decl->power * PITCH_WINDUP_HOLD_MAX);
            if (aim_len < 1) aim_len = 1;
            w->mode = WIDGET_DESCENT;
            w->counter = aim_len; // start fully to the right
            w->counter_max = aim_len; // meter reads 1.0 and descends to 0.0
            w->dir = -1; // one-way right → left
        }
    } else if (decl->phase == PITCH_DECL_POWER && w->mode == WIDGET_DESCENT && w->dir != 0) {
        // click while the aim meter descends → lock direction (FOCAL = on the plate → strike) → AIMED.
        float f = (float)w->counter / (float)w->counter_max;
        float dir = (f - PITCH_AIM_FOCAL) * PITCH_AIM_SCALE;
        if (dir < -1.0f) dir = -1.0f;
        if (dir > 1.0f) dir = 1.0f;
        decl->direction = dir;
        decl->phase = PITCH_DECL_AIMED;
        w->dir = 0; // stop the meter at the locked position; the engine releases at the windup end
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
