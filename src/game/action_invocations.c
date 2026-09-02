/*
    the main purpose of this code is to set flags for execute_actions when key combinations trigger some events.
    everything here is pretty straightforward.
*/

#include "globals.h"
#include "action_invocations.h"
#include "execute_actions.h" // intent_push — the channel op lives with the gate that drains it
#include "actions/throwing_system.h"
#include "actions/pitching_system.h"
#include "actions/batting_system.h" // the batter arc constants — the aim cursor moves across the same arc
#include "actions_pure/swing_geometry.h" // the swing minigame's shape, shared with the engine and the test
#include "actions_pure/batting_physics.h" // the flight solution, so the descent sizes itself from the ball
#include "rules_pure/player_utils.h"
#include "field_layout.h"
#include "vector_math.h"
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
// length by design — the engine clock drives only the render gather animation — kept close to
// THROW_WINDUP_MAX_FRAMES for visual coherence.
#define THROW_CHARGE_FRAMES 45 // frames to reach full charge on a held throw (~0.9s at 50Hz)

static int checkThrowGesture(
    MatchSession* match, ClientInputState* clientInput, const KeyStates* key_states, TeamControlMode control,
    IntentChannel* channel
);
static void advance_widget(InputWidget* w);
static float charge_to_power(int counter, int counter_max);
static void
checkDrop(MatchSession* match, const KeyStates* key_states, int key, TeamControlMode control, IntentChannel* channel);
static void checkMove(
    MatchSession* match, ClientInputState* clientInput, const KeyStates* key_states, TeamControlMode control,
    IntentChannel* channel
);
static void checkChangePlayer(
    MatchSession* match, const KeyStates* key_states, int key, TeamControlMode control, IntentChannel* channel
);
static void checkPitch(
    MatchSession* match, ClientInputState* clientInput, const KeyStates* key_states, int key, TeamControlMode control,
    IntentChannel* channel
);
static void checkBatterSelection(
    const MatchSession* match, const Scoreboard* scoreboard, const HalfInningState* halfInningState,
    ClientInputState* clientInput, const KeyStates* key_states, int change, int select, TeamControlMode control,
    IntentChannel* channel
);
static void checkFreeWalkDecision(
    const KeyStates* key_states, int accept, int reject, TeamControlMode control, IntentChannel* channel
);
static void checkBatterAngle(
    const MatchSession* match, ClientInputState* clientInput, const KeyStates* key_states, int increase, int decrease,
    TeamControlMode control, IntentChannel* channel
);
static void checkSwing(
    const MatchSession* match, const BetweenPitchState* betweenPitchState, ClientInputState* clientInput,
    const KeyStates* key_states, int swingKey, int withdrawKey, TeamControlMode control, IntentChannel* channel
);
static void checkBattingTeamRun(
    MatchSession* match, ClientInputState* clientInput, const KeyStates* key_states, int key, TeamControlMode control,
    BaseID base, const RefereeState* referee, IntentChannel* channel
);

void action_invocations(
    MatchSession* match, ClientInputState* clientInput, const KeyStates* key_states, const Scoreboard* scoreboard,
    const RefereeState* referee, const HalfInningState* halfInningState, const BetweenPitchState* betweenPitchState,
    IntentChannels* channels
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
        // A power sweep abandoned to a throw. KEY_2 is shared, and sampling the pitch on the press
        // edge means a press followed by a direction key arms this meter one frame before the throw
        // claims the key — where waiting for the release used to hide the overlap, since the throw
        // suppressed the pitch before any release arrived. Nothing was declared (the sweep is
        // client-local), so retiring it is all that is owed.
        if (w->mode == WIDGET_PING_PONG && match->pendingActionState.throwDeclaration.phase != THROW_DECL_IDLE) {
            w->mode = WIDGET_IDLE;
            w->counter_max = 0;
            w->dir = 0;
        } else if (w->mode == WIDGET_PING_PONG && w->dir == 0) {
            w->mode = WIDGET_IDLE;
            w->counter_max = 0;
        } else if (w->mode == WIDGET_DESCENT && match->pendingActionState.pitchDeclaration.phase == PITCH_DECL_IDLE) {
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
        if (w->mode != WIDGET_IDLE && match->pendingActionState.throwDeclaration.phase == THROW_DECL_IDLE) {
            w->mode = WIDGET_IDLE;
            w->counter_max = 0;
            w->dir = 0;
        }
    }

    // The action key (KEY_2) is shared. Held together with a direction key it winds up a throw (hold to
    // gather, release to fire); held alone it pitches / drops / changes. checkThrowGesture runs first and
    // reports whether a throw gesture owns KEY_2 this frame — if so we suppress the others, so a throw's
    // hold or release never doubles as a drop/pitch on the same key edge.
    int throw_engaged = checkThrowGesture(match, clientInput, key_states, catchingControl, &channels->catching);
    if (!throw_engaged) {
        if (match->pII.hasBallIndex == -1) {
            checkChangePlayer(match, key_states, KEY_2, catchingControl, &channels->catching);
        } else if (match->pII.controlIndex != match->pII.catcherOnBaseIndex[0]) {
            checkDrop(match, key_states, KEY_2, catchingControl, &channels->catching);
        } else {
            checkPitch(match, clientInput, key_states, KEY_2, catchingControl, &channels->catching);
        }
    }

    checkMove(match, clientInput, key_states, catchingControl, &channels->catching);

    // check these only if necessary. also if it happened to be so that
    // they are both asked the same time, choose the free walk first
    if (match->flowControl.waitingForFreeWalkDecision == 1) {
        checkFreeWalkDecision(key_states, KEY_2, KEY_1, battingControl, &channels->batting);
    } else if (match->flowControl.waitingForBatterDecision == 1) {
        checkBatterSelection(
            match, scoreboard, halfInningState, clientInput, key_states, KEY_1, KEY_2, battingControl,
            &channels->batting
        );
    }
    // No batter question on the table, so no answer is being held. Keyed on the QUESTION and not on
    // which branch above ran: a free walk offered in the same frame must not leave a stale choice
    // behind it. The widget self-clears the way every other client-local gesture does, which is why
    // a physical-world reset never has to reach in here.
    if (match->flowControl.waitingForBatterDecision != 1) {
        clientInput->batterWidget.selected = 0;
        clientInput->batterWidget.highlight = 0;
    }
    checkBatterAngle(match, clientInput, key_states, KEY_PLUS, KEY_MINUS, battingControl, &channels->batting);
    checkSwing(match, betweenPitchState, clientInput, key_states, KEY_2, KEY_1, battingControl, &channels->batting);

    checkBattingTeamRun(
        match, clientInput, key_states, KEY_DOWN, battingControl, BASE_HOME, referee, &channels->batting
    );
    checkBattingTeamRun(
        match, clientInput, key_states, KEY_LEFT, battingControl, BASE_FIRST, referee, &channels->batting
    );
    checkBattingTeamRun(
        match, clientInput, key_states, KEY_RIGHT, battingControl, BASE_SECOND, referee, &channels->batting
    );
    checkBattingTeamRun(
        match, clientInput, key_states, KEY_UP, battingControl, BASE_THIRD, referee, &channels->batting
    );
}

// Human throw gesture (hold-release) → a declared ThrowDeclaration, message by message, with the power declared as
// a VALUE sampled from a client-local CHARGE widget (the engine↔client contract — input NEVER reads the
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
    MatchSession* match, ClientInputState* clientInput, const KeyStates* key_states, TeamControlMode control,
    IntentChannel* channel
)
{
    if (control == CONTROL_AI) {
        return 0; // AI declares the throw COMMITTED directly in catching_ai.c
    }

    // Direction key per target base (index by BaseID: HOME/FIRST/SECOND/THIRD).
    static const int throwKeyForBase[BASE_COUNT] = {KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_UP};
    // Read-only: the declaration is the engine's, and the way to change it is to declare a new one.
    const ThrowDeclaration* decl = &match->pendingActionState.throwDeclaration;
    InputWidget* w = &clientInput->throwWidget;

    // The intent is INITIATED (direction declared, power pending): keep charging the client widget while
    // held; a KEY_2 release samples it and completes the intent (COMMITTED). The gesture owns KEY_2 for the
    // whole hold, so a lone arrow release does not cancel and the KEY_2 release does not also drop.
    if (decl->phase == THROW_DECL_INITIATED) {
        advance_widget(w); // client-local charge; independent of the engine clock
        if (key_states->released[control][KEY_2]) {
            ThrowDeclaration committed = *decl;
            committed.power = charge_to_power(w->counter, w->counter_max); // pass the meter's value in
            committed.phase = THROW_DECL_COMMITTED; // full intent assembled — the engine times the release
            intent_push(channel, (IntentMessage){.kind = INTENT_THROW, .as.throw = committed});
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
            // "I started, toward this base"; power pending (second frame)
            ThrowDeclaration initiated = {.phase = THROW_DECL_INITIATED, .target = heldBase, .power = 0.0f};
            intent_push(channel, (IntentMessage){.kind = INTENT_THROW, .as.throw = initiated});
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

// How far the computed destination may drift before it is worth restating. Normally zero work: a
// fielder walking along its heading recomputes the SAME fence point every frame, because the far end
// of a ray does not move as you travel along it. What this catches is the fielder being put somewhere
// else without the keys changing — a reset moving it home — where the old destination now describes a
// different journey. (Control passing to another player is caught by its own clause below, which is
// exact rather than distance-based.) Like the heartbeat, it reads only the client's own inputs and
// the physical world, never what the engine did with the last message.
#define MOVE_REDECLARE_DRIFT 1.0f

// The blind heartbeat: restate the destination at least this often even when nothing changed.
//
// The drift clause cannot see every way the engine can lose a destination. A reset clears it and
// moves the fielder — and if that move happens to be ALONG the held heading, the far end of the
// heading is the same point it always was, so nothing looks different from here and the fielder
// would stand there with the key held down. This closes that without the widget ever asking the
// engine what it currently holds, which is the part that matters: a client that watches the world
// and corrects itself is deriving intent from actualization, stale under input delay, and two peers
// watching slightly different worlds would correct themselves differently. A heartbeat watches
// nothing. Re-sending a value the engine already has is a no-op by construction, so it can cost a
// message and never a behaviour. The AI carries the same mechanism on its own side of the boundary.
#define MOVE_HEARTBEAT_FRAMES 30

// The move widget: held arrows in, a destination out.
//
// The producer never sends "north", "start" or "stop". It sends WHERE — the far end of the heading
// the keys describe, or, when nothing is held, the point the fielder is standing on, which is what
// "stop" means to an engine that only knows destinations. The velocity that comes out is identical to
// the one the four key flags used to produce, because the destination lies along the same vector.
//
// It speaks only when something changed: the held set, the fielder being steered, or the destination
// itself. Holding a key is a state, not a stream of events, so a widget that re-declared every frame
// would be paying messages to tell the engine what it already holds.
static void checkMove(
    MatchSession* match, ClientInputState* clientInput, const KeyStates* key_states, TeamControlMode control,
    IntentChannel* channel
)
{
    if (control == CONTROL_AI) return;

    MoveWidget* w = &clientInput->moveWidget;

    // KEY_2 held means the throw gesture owns the direction keys this frame, so nothing is being
    // steered — the same suppression the key path had, kept because it is an input-binding rule and
    // input bindings are the client's business.
    int held = 0;
    if (key_states->down[control][KEY_2] == 0) {
        if (key_states->down[control][KEY_UP]) held |= MOVE_HELD_UP;
        if (key_states->down[control][KEY_RIGHT]) held |= MOVE_HELD_RIGHT;
        if (key_states->down[control][KEY_DOWN]) held |= MOVE_HELD_DOWN;
        if (key_states->down[control][KEY_LEFT]) held |= MOVE_HELD_LEFT;
    }

    w->framesSinceDeclared++;

    int index = match->pII.controlIndex;
    if (index == -1) {
        // Nobody to steer: forget what was said, so whoever is handed control next is told afresh.
        w->declared = 0;
        w->held = held;
        w->controlIndex = -1;
        return;
    }

    Vector3D from = match->playerInfo[index].tPI.location;
    Vector3D point;
    if (held == 0) {
        point = from;
    } else {
        // The same axes the key flags fed: x is right minus left, z is down minus up.
        float dirX = ((held & MOVE_HELD_RIGHT) ? 1.0f : 0.0f) - ((held & MOVE_HELD_LEFT) ? 1.0f : 0.0f);
        float dirZ = ((held & MOVE_HELD_DOWN) ? 1.0f : 0.0f) - ((held & MOVE_HELD_UP) ? 1.0f : 0.0f);
        point = field_boundary_point_along(&from, dirX, dirZ);
    }

    int worth_saying =
        !w->declared || w->held != held || w->controlIndex != index ||
        w->framesSinceDeclared >= MOVE_HEARTBEAT_FRAMES ||
        !vec3_is_small_enough_circle_xz(point.x - w->point.x, point.z - w->point.z, MOVE_REDECLARE_DRIFT);
    if (!worth_saying) return;

    intent_push(channel, (IntentMessage){.kind = INTENT_MOVE_TARGET, .as.move_target.point = point});
    w->declared = 1;
    w->held = held;
    w->controlIndex = index;
    w->point = point;
    w->framesSinceDeclared = 0;
}

// Both of these declare a one-shot command on the same key edge. The producer's own check — that no
// catching action is running — is a controller reading the physical world to know what is worth
// declaring; the gate makes the binding decision, and disagreeing with the producer here would only
// cost a message, never a wrong action.
static void checkChangePlayer(
    MatchSession* match, const KeyStates* key_states, int key, TeamControlMode control, IntentChannel* channel
)
{
    if (control != CONTROL_AI) {
        if (match->pendingActionState.current_catching_action == CATCHING_ACTION_NONE) {
            if (key_states->released[control][key] == 1) {
                intent_push(channel, (IntentMessage){.kind = INTENT_CHANGE_PLAYER});
            }
        }
    } else {
        // The AI declares this on its own channel in catching_ai.c
    }
}

static void
checkDrop(MatchSession* match, const KeyStates* key_states, int key, TeamControlMode control, IntentChannel* channel)
{
    if (control != CONTROL_AI) {
        if (match->pendingActionState.current_catching_action == CATCHING_ACTION_NONE) {
            if (key_states->released[control][key] == 1) {
                intent_push(channel, (IntentMessage){.kind = INTENT_DROP_BALL});
            }
        }
    } else {
        // The AI declares this on its own channel in catching_ai.c
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
        // a ping-pong bounces; a charge holds at the top; a descent has already stopped by here
        w->dir = (w->mode == WIDGET_PING_PONG || w->mode == WIDGET_PING_PONG_LOOP) ? -1 : 0;
    } else if (w->counter <= 0) {
        w->counter = 0;
        // A looping ping-pong turns round instead of stopping. It is the swing's power meter, and it
        // needs no deadline of its own: a batter who never presses has not swung, which the engine
        // concludes from silence. Every other mode is finished at the bottom.
        w->dir = (w->mode == WIDGET_PING_PONG_LOOP) ? 1 : 0;
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
// PitchDeclaration.
//
// Sampled on the PRESS edge, not the release. A meter reading is a value the human is aiming at with
// their timing, so every frame between intending and declaring is error added to that value — and
// waiting for the release adds one systematically, in one direction. That is a whole frame out of a
// hit window that is single-digit frames wide at its tightest. A hold gesture (the throw) is the
// opposite case and keeps its release, because there the holding IS the meaning. Power selection is pre-windup and
// client-local (the declaration stays IDLE); the power click is what STARTS the engine windup (power-as-start), so
// there is no separate WINDUP phase:
//   press KEY_2 (idle)        → start the power ping-pong (0 → max → 0); nothing declared yet.
//   click during power sweep  → power = cursor fraction (right peak = max); phase POWER (engine windup
//                               begins); start the aim meter descending from the right.
//   click during aim descent  → direction = (cursor − FOCAL)·SCALE (FOCAL = on the plate / strike); AIMED.
// A meter that runs out unclicked: the power ping-pong cancels (press again to retry); the aim descent
// rests at the left and the engine windup elapses → valesyöttö (FAKE) — see update_pitch_actualization.
// The widget is client-local input interpretation only; the AI declares values directly and never touches it.
static void checkPitch(
    MatchSession* match, ClientInputState* clientInput, const KeyStates* key_states, int key, TeamControlMode control,
    IntentChannel* channel
)
{
    if (control == CONTROL_AI) {
        return; // AI declares the pitch directly in catching_ai.c
    }

    InputWidget* w = &clientInput->pitchWidget;
    // Read-only, for the same reason as the throw: declaring is sending, never writing.
    const PitchDeclaration* decl = &match->pendingActionState.pitchDeclaration;

    // (The widget is retired in action_invocations every frame — see the top of that function — so it
    // frees the batting meter even after the ball leaves the pitcher's hand and checkPitch goes quiet.)
    advance_widget(w); // move the cursor (if running) before reading a click

    if (key_states->pressed[control][key] != 1) {
        return; // a meter is sampled on the PRESS edge — see below
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
            PitchDeclaration declared = *decl;
            declared.power = (float)w->counter / (float)w->counter_max; // [0,1]
            declared.phase = PITCH_DECL_POWER;
            intent_push(channel, (IntentMessage){.kind = INTENT_PITCH, .as.pitch = declared});
            // start the aim meter: a one-way descent from the right, its length matching the pre-throw
            // windup (crouch + power-scaled hold) so the cursor reaches the left as the throw begins.
            int aim_len = PITCH_WINDUP_DOWN_FRAMES + (int)(declared.power * PITCH_WINDUP_HOLD_MAX);
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
        PitchDeclaration declared = *decl;
        declared.direction = dir;
        declared.phase = PITCH_DECL_AIMED;
        intent_push(channel, (IntentMessage){.kind = INTENT_PITCH, .as.pitch = declared});
        w->dir = 0; // stop the meter at the locked position; the engine releases at the windup end
    }
}

// §27's choice, made by a human: a cursor over the legal candidates, and a key that accepts one.
//
// The candidate list is rebuilt from the world every frame rather than remembered, which is what
// makes the cursor unable to go stale — and it is the same function the INGEST gate will judge the
// answer with, so the client can be helpful without owning a second copy of the rule. If a run
// scored mid-decision takes the regular out of the list, the list shortens and the cursor comes with
// it; there is nothing to withdraw, because nothing was stored. Consolidation used to carry that
// withdrawal, because the cursor was engine state.
//
// Once accepted, the choice is restated every frame the decision is open. That is not a retry: the
// seat may not be free yet — the previous batter can hold safety at home for some frames after
// leaving the plate — and a producer that says the same true thing every tick never has to know it.
static void checkBatterSelection(
    const MatchSession* match, const Scoreboard* scoreboard, const HalfInningState* halfInningState,
    ClientInputState* clientInput, const KeyStates* key_states, int change, int select, TeamControlMode control,
    IntentChannel* channel
)
{
    if (control == CONTROL_AI) return; // the AI declares its choice in batting_ai.c

    BatterSelectWidget* w = &clientInput->batterWidget;

    int candidates[BATTER_CANDIDATE_MAX];
    const int count = list_batter_candidates(match, scoreboard, halfInningState, candidates);
    if (count <= 0) return; // §12: nobody may bat — the half-inning is ending, not waiting

    if (w->highlight < 0 || w->highlight >= count) w->highlight = 0;

    if (key_states->released[control][change] == 1) {
        // Cycling is now purely a change of view. It used to be an engine action — a flag that moved
        // the engine's offer — so a human merely browsing the list was mutating state the rules then
        // had to be kept true of.
        w->highlight = (w->highlight + 1) % count;
        w->selected = 0;
    } else if (key_states->released[control][select] == 1) {
        w->selected = 1;
    }

    if (w->selected) {
        intent_push(
            channel,
            (IntentMessage){.kind = INTENT_SELECT_BATTER, .as.select_batter = {.index = candidates[w->highlight]}}
        );
    }
}

// One answer per tick: if both keys are released on the same frame, accepting wins. That was implicit
// before — the second writer found the flag already set and left it alone — and is said outright here.
static void checkFreeWalkDecision(
    const KeyStates* key_states, int accept, int reject, TeamControlMode control, IntentChannel* channel
)
{
    if (control != CONTROL_AI) {
        if (key_states->released[control][accept] == 1) {
            intent_push(channel, (IntentMessage){.kind = INTENT_TAKE_FREE_WALK, .as.free_walk = {.accept = 1}});
        } else if (key_states->released[control][reject] == 1) {
            intent_push(channel, (IntentMessage){.kind = INTENT_TAKE_FREE_WALK, .as.free_walk = {.accept = 0}});
        }
    } else {
        // The AI declares this on its own channel in batting_ai.c
    }
}

// The human's aim: held keys in, an ANGLE out.
//
// The cursor moves at the same rate the engine walks the body, so the batter tracks the keys exactly
// as before — the difference is what crosses the boundary. A key edge used to be the message, which
// meant the engine had to be told when the human started AND stopped pressing, and a lost "stop"
// left the batter walking. An angle is a complete value: the worst a lost message can do is leave
// the aim where it was.
//
// The cursor lives here rather than in the World because it is a gesture, and it self-clears when
// there is no batting to aim — the same rule every other client-local widget follows, and the reason
// a physical-world reset never has to reach into this file.
static void checkBatterAngle(
    const MatchSession* match, ClientInputState* clientInput, const KeyStates* key_states, int increase, int decrease,
    TeamControlMode control, IntentChannel* channel
)
{
    if (control == CONTROL_AI) return; // the AI declares its aim in batting_ai.c

    if (match->pRAI.batting_going_on != 1) {
        // Between at-bats the engine puts the batter back at zero (init_batter), so the cursor goes
        // with it. Without this the next batter would start walking toward the last one's aim.
        clientInput->batterAim = 0.0f;
        return;
    }

    if (key_states->down[control][increase] == 1) {
        clientInput->batterAim += BATTER_ANGLE_SPEED_CONSTANT;
    } else if (key_states->down[control][decrease] == 1) {
        clientInput->batterAim -= BATTER_ANGLE_SPEED_CONSTANT;
    }
    if (clientInput->batterAim > BATTER_ANGLE_LIMIT) clientInput->batterAim = BATTER_ANGLE_LIMIT;
    if (clientInput->batterAim < -BATTER_ANGLE_LIMIT) clientInput->batterAim = -BATTER_ANGLE_LIMIT;

    intent_push(
        channel, (IntentMessage){.kind = INTENT_SWING_ANGLE, .as.swing_angle = {.angle = clientInput->batterAim}}
    );
}

// What the swing meter currently reads, for the display. The power sweep shows its raw level; the
// vertical descent shows the VALUE it would declare, which is the same number the batter is aiming
// at. Render-only, and the only thing outside this file that looks at the widget.
float swing_widget_display(const SwingWidget* w)
{
    if (w->meter.mode == WIDGET_IDLE || w->meter.counter_max <= 0) return 0.0f;
    float level = (float)w->meter.counter / (float)w->meter.counter_max;
    if (w->meter.mode == WIDGET_DESCENT) return level * swing_marker_top(w->power);
    return level;
}

// The human's swing — a looping power ping-pong while the pitcher winds up, then a one-way descent
// while the ball is in the air, one press each. It is the pitch's gesture with the beats interleaved
// the other way round, and it declares VALUES at both.
//
//   press KEY_2 during the windup  → power = the sweep's level. The windup is where this belongs:
//                                    the batter loads while the pitcher is still crouching, reading
//                                    the crouch, which is the toss height made physical. That is the
//                                    only channel the fourth law allows — the pitcher's declared
//                                    power is not readable from here and must not become so.
//   press KEY_2 during the flight  → vertical = where the marker has fallen to. Contact is timed off
//                                    the ball, so this is the swing's whole timing.
//   press KEY_1 after a power      → withdraw (SWING_PASS). The "väärä!" call: having loaded, the
//                                    batter sees the pitch is not coming to the plate and stops. It
//                                    is worth a ball where swinging and missing is worth a strike,
//                                    which is why it is a message and not merely an absence.
//
// Nothing here reads a declaration back. The gesture's own progress lives in the widget: `power` is
// the client's copy of what it said, because scaling the descent from the ENGINE's copy is exactly
// the read-back that still ties the pitch's widget to the world. And the descent is sized from the
// ball the client can see, through the same pure solution the engine uses, rather than from the
// engine's contact frame — the pitch's aim meter sizes itself from the windup constants for the same
// reason.
static void checkSwing(
    const MatchSession* match, const BetweenPitchState* betweenPitchState, ClientInputState* clientInput,
    const KeyStates* key_states, int swingKey, int withdrawKey, TeamControlMode control, IntentChannel* channel
)
{
    if (control == CONTROL_AI) return; // the AI decides two numbers and says them; it has no gesture

    SwingWidget* w = &clientInput->swingWidget;
    const SwingActualization* swing = &match->pendingActionState.swing;
    const int airborne = (match->pRAI.pitch_state == PITCH_STAGE_AIRBORNE);

    // The gesture is over when there is no swing to declare into. Asked of the physical world, never
    // of what the engine has stored from us — so a swing cut short by a batter forced to run, a foul
    // reset or the end of an at-bat retires this widget without anything having to reach into it.
    if (!swing_may_be_declared(match, betweenPitchState)) {
        w->meter.mode = WIDGET_IDLE;
        w->meter.counter_max = 0;
        w->meter.dir = 0;
        w->framesAirborne = -1;
        w->power = 0.0f;
        return;
    }

    if (airborne) {
        w->framesAirborne = (w->framesAirborne < 0) ? 0 : w->framesAirborne + 1;
    }

    // Arm the right meter for the beat we are on. The power sweep loops for as long as the batter has
    // not committed; the descent starts the moment he has AND the ball is up, so its length can be
    // the flight he actually has left.
    if (!swing->powerActive && w->meter.mode != WIDGET_PING_PONG_LOOP) {
        w->meter.mode = WIDGET_PING_PONG_LOOP;
        w->meter.counter = 0;
        w->meter.counter_max = SWING_POWER_SWEEP_FRAMES;
        w->meter.dir = 1;
    } else if (swing->powerActive && airborne && w->meter.mode != WIDGET_DESCENT) {
        int flight = calculate_pitch_frame_time(match->ballInfo.velocity.y, GRAVITY, 0.0f, SWING_CONTACT_TWEAK_FRAMES);
        int remaining = flight - w->framesAirborne;
        int sweep = swing_vertical_sweep_frames(remaining);
        w->meter.mode = WIDGET_DESCENT;
        w->meter.counter = sweep; // starts at the marker's top and falls
        w->meter.counter_max = sweep;
        w->meter.dir = -1;
    }

    advance_widget(&w->meter);

    // Withdrawing is only meaningful once something has been committed and before the swing is spent.
    if (key_states->pressed[control][withdrawKey] == 1 && swing->powerActive) {
        intent_push(channel, (IntentMessage){.kind = INTENT_SWING_PASS});
        return;
    }

    if (key_states->pressed[control][swingKey] != 1) return;

    if (!swing->powerActive) {
        float power = (float)w->meter.counter / (float)w->meter.counter_max;
        w->power = power; // our own copy: the descent is scaled by it, and we do not ask the engine
        intent_push(channel, (IntentMessage){.kind = INTENT_SWING_POWER, .as.swing_power = {.power = power}});
        // The descent is not armed here. It is armed above, on a frame where the ball is actually in
        // the air — which may be this one or may be several beats away, and the widget does not care.
        w->meter.mode = WIDGET_IDLE;
        w->meter.counter_max = 0;
        w->meter.dir = 0;
    } else if (w->meter.mode == WIDGET_DESCENT && !swing->verticalActive) {
        // The declared value is the marker's position on the meter the human is watching: the level
        // scaled by this power's top. The sweet spot sits at SWING_VERTICAL_FOCAL for every power,
        // which is what makes one learned rhythm work for a bunt and for a full swing alike.
        float vertical = swing_marker_top(w->power) * (float)w->meter.counter / (float)w->meter.counter_max;
        intent_push(
            channel, (IntentMessage){.kind = INTENT_SWING_VERTICAL, .as.swing_vertical = {.vertical = vertical}}
        );
        w->meter.dir = 0; // stop where it was read; the engine owns what happens at contact
    }
}

static void checkBattingTeamRun(
    MatchSession* match, ClientInputState* clientInput, const KeyStates* key_states, int key, TeamControlMode control,
    BaseID base, const RefereeState* referee, IntentChannel* channel
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
                RunIntent command;
                if (clientInput->run_press_window[base] > 0) {
                    // Second press inside the window → deliberate "run now".
                    command = RUN_COMMIT;
                    clientInput->run_press_window[base] = 0;
                } else {
                    // First press → arm (settled) or come back (off the base); open the double-press window.
                    PlayerUnitState state = match->playerInfo[index].bTPI.state;
                    if (state == PLAYER_STATE_ON_BASE || state == PLAYER_STATE_AT_BAT) {
                        command = RUN_FORWARD;
                    } else {
                        command = RUN_BACK;
                    }
                    clientInput->run_press_window[base] = RUN_DOUBLE_PRESS_WINDOW;
                }
                intent_push(
                    channel, (IntentMessage){.kind = INTENT_BASE_RUN, .as.base_run = {.base = base, .command = command}}
                );
            }
        }
    } else {
        // The AI declares the run command on its own channel in batting_ai.c
    }
}
