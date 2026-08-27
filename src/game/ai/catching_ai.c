#include "ai/catching_ai.h"
#include "execute_actions.h"
#include "actions/throwing_system.h"
#include "actions/pitching_system.h"
#include "common_logic.h"
#include "vector_math.h"
#include "catching_ai_strategy.h"
#include "pitching_ai_strategy.h"
#include "rng.h"
#include "base_logic.h"
#include "base_control.h"

#define ANIMATION_FREQUENCY 3

// How long (frames) after the batter is ready the AI waits before pitching — a deliberate pause so a
// human batter can settle, and so the fielders can get home. (Was the legacy batterReadyTimer>70 gate +
// the pitchTime>=100 ramp, collapsed into one clock.)
#define AI_PITCH_DELAY 150

static void update_ai_pitching(
    MatchSession* match, const HalfInningState* halfInningState, AIControllerState* aiController, IntentChannel* channel
)
{
    int pitcherIndex = match->pII.catcherOnBaseIndex[0];

    // Try to pitch when: the pitcher holds the ball at home, no catching action is in progress, no pitch
    // is already going, the batter has had a moment to settle, and the fielders are home. The AI then
    // DECLARES the pitch directly through the phased PitchDeclaration — no meter to puppeteer, no lock
    // machine to mirror. The engine owns the windup + release.
    if (match->pII.hasBallIndex == pitcherIndex && match->playerInfo[pitcherIndex].cTPI.isNearHomeLocation == 1 &&
        match->pendingActionState.current_catching_action == CATCHING_ACTION_NONE &&
        match->pRAI.pitch_state == PITCH_STAGE_NONE && match->aiState.batterReadyTimer > AI_PITCH_DELAY) {

        int i;
        int fielders_home = 1;
        for (i = PLAYERS_IN_TEAM + JOKER_COUNT; i < 2 * PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
            if (match->playerInfo[i].cTPI.isNearHomeLocation == 0) fielders_home = 0;
        }

        if (fielders_home) {
            int rand_power = seeded_rand(&aiController->rngSeed, 5);
            int rand_dir = seeded_rand(&aiController->rngSeed, 7);
            int rand_choice = seeded_rand(&aiController->rngSeed, 10);
            PitchAim aim = decide_pitch_aim(
                count_active_batting_players(match->playerInfo), halfInningState->strikes, halfInningState->balls,
                rand_power, rand_dir, rand_choice
            );
            // Commit the complete aim at once (the staggered reveal the batter reacts to comes with the
            // swing slice). One message, complete: the engine actualizes from there.
            PitchDeclaration declared = {.phase = PITCH_DECL_AIMED, .power = aim.power, .direction = aim.direction};
            intent_push(channel, (IntentMessage){.kind = INTENT_PITCH, .as.pitch = declared});
        }
    }

    // batterReadyTimer: the AI's pre-pitch pause clock (counts up while the pitcher holds the ball with a
    // ready batter; reset whenever the batter is not ready).
    if (match->pRAI.batter_ready == 1 && match->pII.catcherOnBaseIndex[0] == match->pII.hasBallIndex &&
        match->aiState.batterReadyTimer == -1) {
        match->aiState.batterReadyTimer = 0;
    } else if (match->pRAI.batter_ready == 0) {
        match->aiState.batterReadyTimer = -1;
    }
    if (match->aiState.batterReadyTimer != -1) {
        match->aiState.batterReadyTimer++;
    }
}

// Steer the controlled fielder by saying where it should be, and say it only when saying it means
// something. The controller decides WHAT is worth declaring — the dead zone, the drift threshold,
// the heartbeat — and the engine decides how the walk happens. Nothing here simulates a keypress,
// counts a throttle, or reads back what it said last time.
static void
steer_controlled_fielder(MatchSession* match, AIControllerState* ai, const Vector3D* desired, IntentChannel* channel)
{
    int index = match->pII.controlIndex;
    if (index == -1) return;

    // A different fielder is a different conversation: whatever was said about the last one says
    // nothing about this one. controlIndex is durable world state, so noticing this is observation,
    // not read-back.
    if (ai->lastSteeredFielder != index) {
        ai->hasDeclaredMoveTarget = 0;
        ai->lastSteeredFielder = index;
    }

    MoveDeclaration decision = decide_move_declaration(
        desired, ai->hasDeclaredMoveTarget, &ai->lastDeclaredMoveTarget, ai->framesSinceMoveDeclared
    );

    if (!decision.declare) {
        ai->framesSinceMoveDeclared++;
        return;
    }

    intent_push(channel, (IntentMessage){.kind = INTENT_MOVE_TARGET, .as.move_target.point = decision.point});
    ai->lastDeclaredMoveTarget = decision.point;
    ai->hasDeclaredMoveTarget = 1;
    ai->framesSinceMoveDeclared = 0;
}

void throw_ball_to_base(MatchSession* match, BaseID base, IntentChannel* channel)
{
    // Don't start a throw if any catching action is already in progress. We read the real
    // execution-side mutex (current_catching_action) directly — the AI no longer keeps a duplicate
    // throwStage/AI_THROW_LOCK state machine that had to mirror (and drifted from) it.
    if (match->pendingActionState.current_catching_action != CATCHING_ACTION_NONE) {
        return;
    }

    int catcherIndex = match->pII.catcherOnBaseIndex[base];
    int catcherNearHome = 0;
    if (catcherIndex != -1) {
        catcherNearHome = match->playerInfo[catcherIndex].cTPI.isNearHomeLocation;
    }

    int replacerIndex = match->pII.catcherReplacerOnBaseIndex[base];
    ReplacementState replacerStage = REPLACEMENT_IDLE;
    int replacerBase = -1;
    int replacerMoving = 0;
    if (replacerIndex != -1) {
        replacerStage = match->playerInfo[replacerIndex].cTPI.replacingStage;
        replacerBase = match->playerInfo[replacerIndex].cTPI.replacingBase;
        replacerMoving = match->playerInfo[replacerIndex].cPI.moving;
    }

    int shouldThrow = should_ai_throw(
        &(match->pII), catcherNearHome, replacerIndex, replacerStage, replacerBase, replacerMoving, base
    );

    if (shouldThrow == 1) {
        // Declare the throw COMMITTED — target + power at once. The engine sizes the windup to the power
        // and actualizes the release (execute_actions / throwing_system); the AI plays no minigame, counts
        // no frames. (The human reaches the same COMMITTED in two frames — INITIATED{target} then
        // COMMITTED{power} from its charge widget; the engine owns the release instant for both.)
        ThrowDeclaration declared = {.phase = THROW_DECL_COMMITTED, .target = base, .power = THROW_POWER_DEFAULT};
        intent_push(channel, (IntentMessage){.kind = INTENT_THROW, .as.throw = declared});
    }
}

void update_catching_ai(
    MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions,
    AIControllerState* aiController, IntentChannel* channel
)
{
    // Update AI pitching
    update_ai_pitching(match, &rules->halfInningState, aiController, channel);

    // (The throw no longer needs an AI-side "finish throwing" step: the engine owns the windup and
    // releases the ball when it completes. throwStage / AI_THROW_LOCK / the meter-watch / the
    // timeout-abort fallback all deleted with it.)

    // if noone has ball and someone is controlled, ai will try to move towards the target point calculated
    // in game_manipulation.
    if (match->pII.hasBallIndex == -1 && match->pII.controlIndex != -1) {
        // gate on the real execution-side mutex (no catching action in progress) — the AI's duplicate
        // aiActionEventLock/aiLockUpdate is gone.
        if (match->pendingActionState.current_catching_action == CATCHING_ACTION_NONE) {
            if (match->pRAI.throw_going_to_base == -1 || match->ballInfo.currentFlightHasHitGround == 1) {
                Vector3D target = chase_point(
                    &match->playerInfo[match->pII.controlIndex].tPI.location, &(match->cameraState.targetPoint)
                );
                steer_controlled_fielder(match, aiController, &target, channel);
            }
        }
    }
    // if someone has ball
    if (match->pII.hasBallIndex != -1) {
        int index3 = get_base_controller(match, &rules->referee, (BaseID)3);
        int index2 = get_base_controller(match, &rules->referee, (BaseID)2);

        BaseID r3BaseAtPitchStart = BASE_NONE;
        int r3IsOnBase = 0;
        if (index3 != -1) {
            r3BaseAtPitchStart = rules->referee.battingPlayers[index3].baseAtPitchStart;
            r3IsOnBase = (match->playerInfo[index3].bTPI.state == PLAYER_STATE_ON_BASE);
        }

        BaseID r2BaseAtPitchStart = BASE_NONE;
        int r2IsOnBase = 0;
        if (index2 != -1) {
            r2BaseAtPitchStart = rules->referee.battingPlayers[index2].baseAtPitchStart;
            r2IsOnBase = (match->playerInfo[index2].bTPI.state == PLAYER_STATE_ON_BASE);
        }

        int catcherHomeIndex = match->pII.catcherOnBaseIndex[0];
        int hasBallIndex = match->pII.hasBallIndex;

        if (should_ai_drop_ball(
                &(rules->referee), &(rules->betweenPitchState), r3BaseAtPitchStart, r3IsOnBase, r2BaseAtPitchStart,
                r2IsOnBase, catcherHomeIndex, hasBallIndex
            )) {
            // Declare the drop and stop there. Whether it is allowed — this catcher still holding
            // the ball, no throw or pitch with a claim on it — is the gate's decision, made once for
            // every producer, so the old AI_DROP_LOCK / dropStage wrapper has nothing left to guard
            // against and no execution-side state left to mirror. `hasBallIndex` flips to -1 the
            // frame the drop is actualized, which stops any re-issue on its own.
            intent_push(channel, (IntentMessage){.kind = INTENT_DROP_BALL});
        }
        // otherwise we throw or move towards a base where lead_from_base player is going. if lead_from_base player is
        // going nowhere we take ball to home base.
        else {
            BaseID leadBase = BASE_NONE;
            int throwBase = 0;

            CatchingRunnerInfo runners[PLAYERS_IN_TEAM + JOKER_COUNT];
            int runnerCount = 0;
            int i;

            for (i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
                PlayerUnitState s = match->playerInfo[i].bTPI.state;
                BaseID bid = match->playerInfo[i].bTPI.baseId;

                if (bid != BASE_NONE) {
                    runners[runnerCount].isOnBase = (s == PLAYER_STATE_ON_BASE || s == PLAYER_STATE_AT_BAT);
                    runners[runnerCount].takingFreeWalk = (s == PLAYER_STATE_ADVANCING_FREELY);
                    runners[runnerCount].base = bid;
                    runners[runnerCount].leading = (s == PLAYER_STATE_LEADING);
                    runnerCount++;
                }
            }

            int randomVal = seeded_rand(&aiController->rngSeed, 500);
            leadBase = determine_lead_base(runners, runnerCount, randomVal);

            if (leadBase != BASE_NONE && base_cmp(leadBase, BASE_THIRD) < 0)
                throwBase = (int)base_get_next(leadBase);
            else
                throwBase = 0;

            if (match->pendingActionState.current_catching_action == CATCHING_ACTION_NONE) {
                // Two different jobs share this move, and telling them apart is what keeps the
                // game alive.
                //
                // If the player we are steering IS the catcher of that base, this is "go back to
                // your post" — and for the pitcher it is load-bearing: the AI refuses to pitch
                // until every fielder is home, so a pitcher that caught a throw off its spot has
                // to walk back or the half-inning never resumes.
                //
                // If it is anybody else, this is "bring the ball in so you can throw it" — and
                // then it must stop SHORT. Walk a carrier inside the engine's too-close-to-throw
                // radius and it can neither throw the ball nor hand it over: a deadlock, measured
                // on six of 264 sweep seeds before the stand-off went in.
                int baseman = match->pII.catcherOnBaseIndex[throwBase];
                if (match->pII.controlIndex != -1 && baseman != -1) {
                    Vector3D target = match->playerInfo[baseman].tPI.homeLocation;
                    if (match->pII.controlIndex != baseman) {
                        Vector3D baseTarget = throw_target_point(fieldPositions, (BaseID)throwBase);
                        target =
                            carry_to_throw_point(&match->playerInfo[match->pII.controlIndex].tPI.location, &baseTarget);
                    }
                    steer_controlled_fielder(match, aiController, &target, channel);
                }
            }
            throw_ball_to_base(match, (BaseID)throwBase, channel);
        }
    }
}
