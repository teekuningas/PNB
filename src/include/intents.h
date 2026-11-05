#ifndef INTENTS_H
#define INTENTS_H

// The intent channel and the messages on it — the tick's input — and the INGEST gate's answer to each,
// Permission with the RuleId that refused it. Neither World: a message lives from the CONTROL stage to the
// gate of the same tick.

#include "foundation.h"

typedef enum { FREE_WALK_IDLE = 0, FREE_WALK_ACCEPT = 1, FREE_WALK_REJECT = 2 } FreeWalkAction;

// Base-running command intent (single-frame, consumed by actualize.c's base_run).
// The producer (human input or AI) declares *which* of two distinct intents it wants — it is never
// inferred from murky live-ball state (that inference, reading the batter's §32 right to leave home as if
// it said "the ball is live", was the base-run slice's offense regression: a pre-pitch ARM got
// mis-actualized as a RUN now):
//   RUN_FORWARD — "arm / lead": the single-press intent. A settled runner arms to advance on the
//                 next pitch (and leads off 1st/2nd). It NEVER starts a settled runner moving this
//                 frame. This is the deferred command.
//   RUN_COMMIT  — "run now": the double-press intent. Start for the next base immediately (a steal,
//                 or chaining a hit once the ball is live). This is the immediate command.
//   RUN_BACK    — disarm a pending lead, or run back to the previous base.
// Both the human (action_invocations) and the AI (batting_ai) emit this same command — no click sim.
typedef enum { RUN_NONE = 0, RUN_FORWARD = 1, RUN_COMMIT = 2, RUN_BACK = 3 } RunIntent;

// ---------------------------------------------------------------------------
// THE INTENT CHANNEL — one per team, a PARAMETER of the tick, never World state.
//
// A controller (human input, AI, a scripted test, one day a remote peer) produces nothing but value
// messages into its own team's channel during the CONTROL stage at the frame top. The INGEST gate
// (ingest.c) drains both channels in the same tick, decides what each message is
// PERMITTED to do, and interprets it into engine-shaped form. The channel is therefore filled and
// emptied inside one tick and is empty at every frame boundary — the state validator checks exactly
// that, every frame, so "a parameter, not state" is a fact rather than an intention.
//
// Two properties this shape buys, both load-bearing later:
//   - It never enters a snapshot. Intent that outlived its tick would make two peers' worlds diverge
//     on nothing more than when a key was pressed.
//   - It is blittable — no pointers, fixed capacity, no heap. This struct is the future wire unit, so
//     it has to be something you can memcpy onto a socket without walking it.
//
// A message is a complete, timeless VALUE ("run forward from second"), never an engine-relative edge
// ("release now"). Duplicates of one kind within a tick are last-write-wins, and ingestion writes
// disjoint engine state per kind, so the order messages happen to sit in the ring can never become a
// hidden input to the simulation.
//
// Every action a producer can take is listed below. Nothing reaches the engine any other way: the
// persistent flag struct these grew out of held its last field, the swing's phase, until the swing
// became two declared values and there was nothing left for it to hold.
typedef enum {
    INTENT_NONE = 0,
    INTENT_BASE_RUN, // batting: {base, command} — run/arm/come back, for the runner controlling `base`
    INTENT_TAKE_FREE_WALK, // batting: {accept} — answer an offered free walk (§26)
    INTENT_SELECT_BATTER, // batting: {index} — seat this player as the next batter (§27)
    INTENT_SWING_ANGLE, // batting: {angle} — where on the arc the batter aims from
    INTENT_SWING_POWER, // batting: {power} — how hard this swing is, [0,1]
    INTENT_SWING_VERTICAL, // batting: {vertical} — the swing's elevation, [0,1] about the sweet spot
    INTENT_SWING_PASS, // batting: this batter declines to swing at the pitch
    INTENT_DROP_BALL, // catching: put the held ball on the ground deliberately
    INTENT_CHANGE_PLAYER, // catching: hand control to the next fielder in the ranked order
    INTENT_MOVE_TARGET, // catching: {point} — where the controlled fielder is headed
    INTENT_PITCH_POWER, // catching: {power} — the toss height; begins the windup
    INTENT_PITCH_AIM, // catching: {direction} — where this pitch goes; held until release
    INTENT_THROW_TARGET, // catching: {base} — the base to throw to; begins the gather
    INTENT_THROW_POWER // catching: {power} — how hard; the throw leaves once its windup allows
} IntentKind;

typedef struct _BaseRunIntent {
    BaseID base;
    RunIntent command;
} BaseRunIntent;

typedef struct _FreeWalkIntent {
    int accept; // 1 = take the walk, 0 = decline it
} FreeWalkIntent;

// Who takes the bat next: a player, named. §12 and §27 name players too ("numero 7 tulee lyömään"),
// and naming one is what makes the message mean the same thing however often it arrives — a "cycle
// to the next candidate" edge means something different every time it is applied, and nothing at all
// when it is applied to a world one tick further on.
//
// The producer restates it every frame while the decision is open rather than sending it once. That
// is not chatter: the per-tick value stream is the ground truth this design is built on, and
// declaring only on change is the compression of it, not the other way round. Restating also makes
// the engine's own timing invisible to the producer — the seat is not free until the previous batter
// has lost home, and nobody has to know that to be answered correctly.
typedef struct _SelectBatterIntent {
    int index; // a playerInfo slot in the batting side's 0..PLAYERS_IN_TEAM+JOKER_COUNT range
} SelectBatterIntent;

// Where on the arc around the pitching plate the batter aims from. The batting physics doubles this
// into the launch heading, so it is the batting side's aim — the exact mirror of the fielder's
// destination, and absolute for the same reason: an angle means the same thing however often it
// arrives, where "nudge left" means something different every time and nothing at all applied to a
// world one tick further on.
//
// The value is never validated — the arc's ends are a physical property the engine's walk enforces,
// so a producer that declares something unreachable walks to the end of the arc and stands there.
// That is the bounded failure the whole message shape is chosen for, and the batting AI uses it
// deliberately: its "wounding swing" declares an angle far outside the arc, meaning as far as this
// batter can go.
typedef struct _SwingAngleIntent {
    float angle; // radians about the batter's zero, clamped to the arc by the engine
} SwingAngleIntent;

// The swing's two declared values, each its own message and each a complete value — never a phase.
//
// A phase-shaped message ("advance my swing from POWER to AIMED") means something different applied
// to a different world, which is what forces a producer to read its own declaration back before it
// dares send one. These say the same thing in every world, so re-delivering one is a no-op and a
// producer never has to ask the engine where it had got to. What ORDER they may arrive in is not
// carried by the messages at all: it is one state-legality question, asked once at the gate.
//
// They are independent, and that is a fact about the physics rather than a convenience: substituting
// the meter's displayed position into the old launch-elevation expression cancels power out of it
// completely (see swing_geometry.h). Power sets how far the ball goes; vertical sets how high.
typedef struct _SwingPowerIntent {
    float power; // [0,1]; below SWING_BUNT_MAX_POWER the body bunts instead of swinging
} SwingPowerIntent;

typedef struct _SwingVerticalIntent {
    float vertical; // [0,1]; SWING_VERTICAL_FOCAL is a level hit, away from it lofts or drives down
} SwingVerticalIntent;

// Where the controlled fielder is going: an absolute point on the field, held until replaced.
//
// Absolute rather than a direction or a key edge, and that is the whole point of the shape. A
// direction means something different when applied twice and nothing at all when applied to a world
// one tick further on; a destination means the same thing however often it arrives and whenever it
// arrives. That is what makes "declare only when it changes" a lossless compression of a per-tick
// value stream rather than a different design, and what keeps rollback (which repeats the last input
// it has) correct by construction. It also bounds the damage of a message that never lands: the
// fielder walks to a stale point and stops, instead of running forever in a stale direction.
typedef struct _MoveTargetIntent {
    Vector3D point;
} MoveTargetIntent;

// The pitch and the throw, as values. Each is two numbers a producer may say on different ticks, and
// the first one is what BEGINS the action: the toss height is what sizes the pitcher's windup, and the
// base is what the thrower turns to face. The second may follow in the same tick (the AI says both at
// once) or arrive while the body is already moving (a human clicks again). None of them is a phase:
// "the power is P" means the same thing in every world, so a producer never has to look at what the
// engine made of its last message before sending the next one. What may follow what is the gate's
// question, asked of the world — a windup running, a throw gathering — and never carried by a message.
typedef struct _PitchPowerIntent {
    float power; // [0,1]
} PitchPowerIntent;

typedef struct _PitchAimIntent {
    float direction; // [-1,1]; 0 is straight up over the plate
} PitchAimIntent;

typedef struct _ThrowTargetIntent {
    BaseID base;
} ThrowTargetIntent;

typedef struct _ThrowPowerIntent {
    float power; // [THROW_POWER_MIN, 1]
} ThrowPowerIntent;

typedef struct _IntentMessage {
    IntentKind kind;
    union {
        BaseRunIntent base_run;
        FreeWalkIntent free_walk;
        SelectBatterIntent select_batter;
        SwingAngleIntent swing_angle;
        SwingPowerIntent swing_power;
        SwingVerticalIntent swing_vertical;
        MoveTargetIntent move_target;
        PitchPowerIntent pitch_power;
        PitchAimIntent pitch_aim;
        ThrowTargetIntent throw_target;
        ThrowPowerIntent throw_power;
    } as;
} IntentMessage;

// Capacity is the worst honest frame, not a guess at an average. The batting AI is the demanding
// producer: it can arm the batter and every occupied base in one tick and then, after a hit, commit
// all four again, with a free-walk answer alongside — nine messages, before any of the actions still
// to move here arrive. Measured against real play, a full AI-vs-AI half-inning never exceeds four per
// team per tick, so the number below is headroom over the structural worst case rather than a guess
// tuned to the average. A full channel drops the message and raises `overflowed`, which the state
// validator treats as a violation: silently losing an intent is exactly the kind of divergence that
// would be undiagnosable once two machines are involved.
#define INTENT_CHANNEL_CAPACITY 16

typedef struct _IntentChannel {
    IntentMessage message[INTENT_CHANNEL_CAPACITY];
    int count;
    int overflowed;
    int malformed; // a kindless message, or one belonging to the other team — see the gate
} IntentChannel;

typedef struct _IntentChannels {
    IntentChannel batting;
    IntentChannel catching;
} IntentChannels;

// A refusal from the INGEST gate, with its reason. A bool would be a one-way door: the moment an
// intent is dropped silently the reason is gone, and anything that wants it later — an on-screen hint
// saying why the pitch would not go, a legible replay, a rule set that can be asked which refusals it
// still makes — has to re-derive the rule somewhere else, which is two homes for one rule.
typedef enum {
    RULE_NONE = 0,
    RULE_CHANGE_PLAYER_NEEDS_EMPTY_HANDS, // control passes only while nobody holds the ball
    RULE_DROP_NEEDS_BALL_IN_HAND, // you cannot drop what you are not holding
    RULE_DROP_NOT_WHILE_THROWING, // a gathered throw owns the ball until it is released
    RULE_DROP_NOT_WHILE_PITCHING, // likewise a pitch in progress
    RULE_FREE_WALK_NEEDS_AN_OFFER, // §26: there is nothing to answer unless one was offered
    RULE_MOVE_NEEDS_A_CONTROLLED_FIELDER, // there is nobody to send anywhere
    RULE_BATTER_SEAT_NEEDS_AN_OFFER, // §27: nobody was asked to take the bat
    RULE_BATTER_NOT_IN_BATTING_TURN, // §27: a regular whose turn in the order it is not
    RULE_BATTER_ORDER_SPENT, // §12: the order has come round; only a joker may extend the turn
    RULE_BATTER_JOKER_ALREADY_USED, // §7: three jokers per half-inning, each once
    RULE_SWING_NEEDS_A_PITCH, // there is no pitch to swing at, or this batter's swing is already over
    RULE_CATCHING_ACTION_IN_PROGRESS, // the ball-holder is already pitching or throwing
    RULE_ONE_CATCHING_ACTION_PER_TICK, // a throw, a drop and a pitch claim the same hands; one is taken
    RULE_PITCH_NEEDS_BALL_AT_HOME, // the pitcher at the plate must be the one holding the ball
    RULE_PITCH_PREVIOUS_UNRESOLVED, // the last pitch has not been judged yet
    RULE_PITCH_NEEDS_A_READY_BATTER, // nobody is standing ready to be pitched to
    RULE_PITCHER_OFF_THE_PLATE, // the pitcher has to be at the pitching plate
    RULE_PITCH_NOT_DURING_FREE_WALK_OFFER, // §26: the batting side has a free walk to answer first
    RULE_PITCH_BATTER_IS_A_RUNNER, // §18(1): three correct pitches spent — nobody is in the batting turn
    RULE_PITCH_AIM_NEEDS_A_WINDUP, // there is no pitch being wound up to aim
    RULE_THROW_NEEDS_BALL_IN_HAND, // you cannot throw what you are not holding
    RULE_THROW_TARGET_TOO_CLOSE, // the thrower is already standing on that base
    RULE_THROW_POWER_NEEDS_A_THROW, // there is no throw gathering to give a power to
    RULE_BASE_RUN_NEEDS_A_RUNNER // nobody holds that base to run from it
} RuleId;

typedef struct _Permission {
    int allowed;
    RuleId denied_by; // RULE_NONE when allowed
} Permission;

#endif /* INTENTS_H */
