#ifndef CATCHING_AI_STRATEGY_H
#define CATCHING_AI_STRATEGY_H

#include "globals.h"

#ifdef __cplusplus
extern "C" {
#endif

// How near the ball's predicted meeting point is near enough to stop chasing it. A CONTROLLER
// decision — what is worth asking for — and deliberately not an engine one. Measured 2026-08-26:
// deleting it along with the old 10-frame key throttle made fielding WORSE (mean recovery 102 → 108
// frames, games ~44% longer), and keeping it as strategy beat the baseline outright. The engine
// walks; the controller decides whether the walk is worth asking for.
//
// It applies to the CHASE and to nothing else. A metre of slack is free when the goal is a guess
// about where a moving ball will be; it is not free when the goal is an exact spot, and the two
// exact spots that matter are both at home plate. Stop a metre short of the pitcher's post and the
// ball sits a few centimetres behind the home line, so the ball never counts as home and the batting
// side never sends up a batter; stop a metre short of a base and a carrier is inside the radius
// where the engine refuses the throw, holding a ball it cannot deliver. Both are deadlocks, and both
// became reachable only when the mover became exact — the old key path overshot by up to ten frames
// of run, which carried the fielder past the slack and hid the hazard for years.
#define AI_MOVE_DEAD_ZONE 1.0f

// How far the destination has to drift before restating it is worth a message. This is compression,
// not policy: every value in between is one the engine already holds, and re-sending it would change
// nothing. Zero here would mean declaring every tick, which is what the design spike did.
#define AI_MOVE_DECLARE_THRESHOLD 0.5f

// The blind heartbeat: restate the destination at least this often even when nothing changed.
//
// It is what keeps the controller's memory of what it last said from going stale without the
// controller ever LOOKING at the engine to find out. A controller may not read its own declaration
// back (that is the no-read-back law, and two peers observing slightly different worlds would emit
// different corrections), so the only safe repair is one that observes nothing at all. Re-sending a
// value the engine already holds is a no-op by construction, so the heartbeat costs a message and
// can never cost a behaviour — and after a reset has emptied the engine's destination, it is what
// gets the fielder moving again.
#define AI_MOVE_HEARTBEAT_FRAMES 60

// How far from the base it means to throw to the controller keeps a ball-carrier.
//
// Built out of the radius it has to clear rather than picked: inside THROW_TO_BASE_DISTANCE the
// engine refuses the throw, and the fielder comes to rest within TARGET_ACHIEVED_THRESHOLD of where
// it was sent, so the stand-off has to clear the refusal radius with room to spare. Walking a
// carrier inside that radius is a DEADLOCK, not a nuisance: the engine has no hand-over, so a
// carrier standing on the base can neither throw the ball nor deliver it, and the inning never ends.
#define AI_THROW_STANDOFF (THROW_TO_BASE_DISTANCE + 2.0f)

// What the controller decides to say about movement this tick, if anything.
typedef struct {
    int declare; // 1 = push a MOVE_TARGET this tick
    Vector3D point; // the destination to declare (meaningless when declare == 0)
} MoveDeclaration;

typedef struct {
    int isOnBase;
    int takingFreeWalk;
    BaseID base;
    int leading;
} CatchingRunnerInfo;

// Where to stand in order to meet a batted ball: the predicted meeting point, or — once the fielder
// is inside the dead zone around it — exactly where the fielder already is, which is this
// controller's way of saying "stop" without there being a stop message to send.
Vector3D chase_point(const Vector3D* fielder, const Vector3D* predicted);

// Where to stand in order to throw to a base: the point at exactly AI_THROW_STANDOFF from the base,
// on the carrier's side of it. Usually that means walking IN — carrying the ball closer before
// throwing it. When the carrier picked the ball up nearer than the stand-off it means stepping back
// OUT, and that is not a nicety: from inside the refusal radius there is no throw and no hand-over,
// so a carrier that stays put is a carrier holding the ball forever. A fielder backing off a step to
// make his throw is ordinary play; a fielder standing on the plate holding the ball is a hung game.
Vector3D carry_to_throw_point(const Vector3D* carrier, const Vector3D* base);

// Whether `desired` is worth saying out loud this tick. Say it if nothing has been said yet, if it
// has drifted past the threshold, or if the heartbeat is due — otherwise stay quiet, because the
// engine already holds a value that close and re-sending it would change nothing.
//
// It decides only the CADENCE. WHERE the fielder should go is the caller's judgement (chase_point,
// carry_to_throw_point, or a post to walk back to), which is what stops one goal's slack rule
// leaking into a goal it was never measured against.
//
// Pure: it reads no world, keeps no state, and returns what to say rather than saying it.
MoveDeclaration decide_move_declaration(
    const Vector3D* desired, int hasDeclared, const Vector3D* lastDeclared, int framesSinceDeclared
);

int should_ai_throw(
    const PlayerIndexInfo* playerIndices, int catcherNearHome, int replacerIndex, int replacerStage, int replacerBase,
    int replacerMoving, int targetBase
);

int should_ai_drop_ball(
    const RefereeState* ref,

    const BetweenPitchState* betweenPitchState,

    BaseID runner3BaseAtPitchStart, int runner3IsOnBase,

    BaseID runner2BaseAtPitchStart, int runner2IsOnBase,

    int catcherHomeIndex, int hasBallIndex
);

BaseID determine_lead_base(const CatchingRunnerInfo* runners, int runnerCount, int randomValue);

#ifdef __cplusplus
}
#endif

#endif
