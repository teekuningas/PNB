#include "actions/fielder_movement.h"
#include "vector_math.h"
#include <math.h>

/*
    fielder_movement.c — the controlled fielder's walk.

    One engine behaviour, one destination. Every producer — human, AI, scripted, one day a peer —
    declares a point and this walks the fielder to it at RUN_SPEED, orienting along the path and
    handing a carried ball the same velocity. The producer never says "start", "stop", "north" or
    "now": it says where, and re-saying where is a no-op.
*/

// Whether the engine may move the controlled fielder's feet at all this tick.
//
// These are physical claims on the fielder, not rules of pesäpallo, and they belong here rather than
// at the INGEST gate for a reason worth stating: they hold whoever declared what. A fielder mid-throw
// stays put even if nobody declared anything this tick, so the suppression has to live where the
// walking happens; asking the same questions at the gate as well would be two homes for one rule,
// and two homes drift. The gate asks the one question that IS about the message — whether there is a
// controlled fielder to send anywhere at all.
//
// The pitch clause is deliberate engine behaviour, not §31: the rulebook's fielders-in-bounds
// requirement is about where fielders are standing when the ball leaves the hand, and this engine
// makes no positioning check at all. Holding the fielders still through a
// pitch is a design decision of this game, and calling it §31 would be exactly the silent
// interpretation the rules work is trying to end.
static int controlled_fielder_may_walk(const MatchSession* match)
{
    if (match->pII.controlIndex == -1) return 0;
    // A gathered throw owns the thrower's feet until the ball leaves the hand.
    if (match->pendingActionState.current_catching_action == CATCHING_ACTION_THROWING) return 0;
    // Fielders hold their positions while a pitch is in progress.
    if (match->pRAI.pitch_state != PITCH_STAGE_NONE) return 0;
    // A throw's recoil animation has to finish before the model may be changed under it.
    if (match->playerInfo[match->pII.controlIndex].cTPI.throwRecoil != 0) return 0;
    return 1;
}

void update_controlled_fielder_movement(MatchSession* match)
{
    if (match->catchingState.controlledMoveTargetActive == 0) return;
    if (!controlled_fielder_may_walk(match)) return;
    // A destination belongs to the fielder it was given to. Control can pass mid-walk — a catch
    // hands it to whoever took the ball — and the newcomer has not been told anything yet.
    if (match->catchingState.controlledMoveTargetFor != match->pII.controlIndex) return;

    int index = match->pII.controlIndex;
    PlayerInfo* player = &match->playerInfo[index];
    int carries_the_ball = (match->pII.hasBallIndex == index);

    float dx = match->catchingState.controlledMoveTarget.x - player->tPI.location.x;
    float dz = match->catchingState.controlledMoveTarget.z - player->tPI.location.z;

    // Arrival is adjudicated here and nowhere else. looksForTarget is the AUTO-movers' arrival
    // machinery in game_manipulation (runners on their paths, ranked fielders chasing a ball); a
    // player that was auto-chasing a moment before it became the controlled one would otherwise be
    // walked by one mover and stopped by another, on two different notions of "there".
    player->cPI.looksForTarget = 0;

    if (vec3_is_small_enough_circle_xz(dx, dz, TARGET_ACHIEVED_THRESHOLD)) {
        // Already standing on it: nothing to do, and doing nothing is the point. This branch is what
        // makes a restated destination a no-op rather than a fresh departure.
        if (player->cPI.moving == 1) {
            vec3_set_xz(&player->tPI.velocity, 0.0f, 0.0f);
            player->cPI.moving = 0;
            player->cPI.lastLastLocationUpdate = 1;
            player->cPI.model = carries_the_ball ? PLAYER_ANIM_STAND_WITH_BALL : PLAYER_ANIM_STAND_NO_BALL;
            // A carried ball comes to rest with its carrier.
            match->ballInfo.needsMoveUpdate = 1;
        }
        return;
    }

    float norm = (float)sqrt(dx * dx + dz * dz);
    if (norm < EPSILON) norm = 1.0f;

    // Facing along the path. For a fielder NOT carrying the ball, game_manipulation turns this toward
    // the ball again later in the same frame — the override the key-driven path lived under too, kept
    // unchanged here.
    player->tPI.orientation.x = dx;
    player->tPI.orientation.z = dz;

    vec3_set_xz(&player->tPI.velocity, dx * RUN_SPEED / norm, dz * RUN_SPEED / norm);
    // Only a raised needsMoveUpdate hands the carrier's velocity to the ball it is holding.
    match->ballInfo.needsMoveUpdate = 1;

    player->cPI.model = carries_the_ball ? PLAYER_ANIM_RUN_WITH_BALL : PLAYER_ANIM_RUN_NO_BALL;

    // The stride restarts when the walk does and never while it is running. This function runs every
    // tick; resetting the animation stage on each one would freeze the fielder mid-step. It is also
    // the resume path: an engine interruption that stops the fielder (a catch, a movement reset)
    // leaves the destination standing, so the walk picks itself up here on the next tick.
    if (player->cPI.moving == 0) {
        player->cPI.moving = 1;
        player->cPI.animationStage = 0;
        player->cPI.animationStageCount = 20;
        player->cPI.animationFrequency = 3;
    }
}
