#include "ai/catching_ai.h"
#include "intent_channel.h"
#include "actions/throwing_system.h"
#include "actions/batting_system.h"
#include "catching_ai_strategy.h"
#include "pitching_ai_strategy.h"
#include "rng.h"
#include "base_logic.h"
#include "base_control.h"

// How long (frames) after the batter is ready the AI waits before pitching — a deliberate pause so a
// human batter can settle, and so the fielders can get home. (Was the legacy batterReadyTimer>70 gate +
// the pitchTime>=100 ramp, collapsed into one clock.)
#define AI_PITCH_DELAY 150

static void update_ai_pitching(
    const MatchSession* match, const HalfInningState* halfInningState, AIControllerState* aiController,
    IntentChannel* channel
)
{
    int pitcherIndex = match->fielding_posts.baseman[0];

    // Try to pitch when: the pitcher holds the ball at home, no catching action is in progress, no pitch
    // is already going, the batter has had a moment to settle, and the fielders are home. The AI then
    // DECLARES the pitch's two values directly — no meter to puppeteer, no lock machine to mirror. The
    // engine owns the windup + release.
    if (match->ballInfo.holder == pitcherIndex && match->playerInfo[pitcherIndex].cTPI.isNearHomeLocation == 1 &&
        match->catching_action.kind == CATCHING_ACTION_NONE && !match->ballInfo.pitch_in_flight &&
        aiController->batterReadyTimer > AI_PITCH_DELAY) {

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
            // Both values at once, in the same tick: the power begins the windup and the aim is held on
            // it. The staggered reveal the batter reacts to is real — the windup's length scales with the
            // toss, so a batter loading during the crouch is reading this decision physically.
            intent_push(channel, (IntentMessage){.kind = INTENT_PITCH_POWER, .as.pitch_power = {.power = aim.power}});
            intent_push(
                channel, (IntentMessage){.kind = INTENT_PITCH_AIM, .as.pitch_aim = {.direction = aim.direction}}
            );
        }
    }

    // batterReadyTimer: the AI's pre-pitch pause clock (counts up while the pitcher holds the ball with a
    // ready batter; reset whenever the batter is not ready).
    if (batter_stands_ready(&match->at_bat) && match->fielding_posts.baseman[0] == match->ballInfo.holder &&
        aiController->batterReadyTimer == -1) {
        aiController->batterReadyTimer = 0;
    } else if (!batter_stands_ready(&match->at_bat)) {
        aiController->batterReadyTimer = -1;
    }
    if (aiController->batterReadyTimer != -1) {
        aiController->batterReadyTimer++;
    }
}

// Steer the controlled fielder by saying where it should be, and say it only when saying it means
// something. The controller decides WHAT is worth declaring — the dead zone, the drift threshold,
// the heartbeat — and the engine decides how the walk happens. Nothing here simulates a keypress,
// counts a throttle, or reads back what it said last time.
static void steer_controlled_fielder(
    const MatchSession* match, AIControllerState* ai, const Vector3D* desired, IntentChannel* channel
)
{
    int index = match->catchingState.controlled_fielder;
    if (index == -1) return;

    // A different fielder is a different conversation: whatever was said about the last one says
    // nothing about this one. The controlled fielder is durable world state, so noticing this is observation,
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

static void throw_ball_to_base(const MatchSession* match, BaseID base, IntentChannel* channel)
{
    // Don't start a throw if any catching action is already in progress. We read the real
    // catching action in progress directly — the AI no longer keeps a duplicate
    // throwStage/AI_THROW_LOCK state machine that had to mirror (and drifted from) it.
    if (match->catching_action.kind != CATCHING_ACTION_NONE) {
        return;
    }

    int catcherIndex = match->fielding_posts.baseman[base];
    int catcherNearHome = 0;
    if (catcherIndex != -1) {
        catcherNearHome = match->playerInfo[catcherIndex].cTPI.isNearHomeLocation;
    }

    int replacerIndex = match->fielding_posts.cover[base];
    ReplacementState replacerStage = REPLACEMENT_IDLE;
    int replacerBase = -1;
    int replacerMoving = 0;
    if (replacerIndex != -1) {
        replacerStage = match->playerInfo[replacerIndex].cTPI.replacingStage;
        replacerBase = match->playerInfo[replacerIndex].cTPI.replacingBase;
        replacerMoving = match->playerInfo[replacerIndex].cPI.moving;
    }

    int shouldThrow = should_ai_throw(
        match->ballInfo.holder, match->fielding_posts.baseman[base], catcherNearHome, replacerIndex, replacerStage,
        replacerBase, replacerMoving, base
    );

    if (shouldThrow == 1) {
        // Target and power at once. The engine sizes the windup to the power and actualizes the release;
        // the AI plays no minigame and counts no frames. (A human says the same two values a moment
        // apart — the target when it starts holding, the power when it lets go; the engine owns the
        // release instant for both.)
        intent_push(channel, (IntentMessage){.kind = INTENT_THROW_TARGET, .as.throw_target = {.base = base}});
        intent_push(
            channel, (IntentMessage){.kind = INTENT_THROW_POWER, .as.throw_power = {.power = THROW_POWER_DEFAULT}}
        );
    }
}

void update_catching_ai(
    const MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions,
    AIControllerState* aiController, IntentChannel* channel
)
{
    // Update AI pitching
    update_ai_pitching(match, &rules->halfInningState, aiController, channel);

    // (The throw no longer needs an AI-side "finish throwing" step: the engine owns the windup and
    // releases the ball when it completes. throwStage / AI_THROW_LOCK / the meter-watch / the
    // timeout-abort fallback all deleted with it.)

    // if noone has ball and someone is controlled, ai will try to move towards the target point the
    // fielders' chase ranking predicted (fielder_behaviours.c).
    if (match->ballInfo.holder == -1 && match->catchingState.controlled_fielder != -1) {
        // gate on the real execution-side mutex (no catching action in progress) — the AI's duplicate
        // aiActionEventLock/aiLockUpdate is gone.
        if (match->catching_action.kind == CATCHING_ACTION_NONE) {
            if (match->ballInfo.thrown_to == BASE_NONE || match->ballInfo.currentFlightHasHitGround == 1) {
                Vector3D target = chase_point(
                    &match->playerInfo[match->catchingState.controlled_fielder].tPI.location,
                    &(match->catchingState.ballTargetPoint)
                );
                steer_controlled_fielder(match, aiController, &target, channel);
            }
        }
    }
    // if someone has ball
    if (match->ballInfo.holder != -1) {
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

        int catcherHomeIndex = match->fielding_posts.baseman[0];
        int holder = match->ballInfo.holder;

        if (should_ai_drop_ball(
                &(rules->referee), &(rules->betweenPitchState), r3BaseAtPitchStart, r3IsOnBase, r2BaseAtPitchStart,
                r2IsOnBase, catcherHomeIndex, holder
            )) {
            // Declare the drop and stop there. Whether it is allowed — this catcher still holding
            // the ball, no throw or pitch with a claim on it — is the gate's decision, made once for
            // every producer, so the old AI_DROP_LOCK / dropStage wrapper has nothing left to guard
            // against and no execution-side state left to mirror. `ballInfo.holder` flips to -1 the
            // frame the drop is actualized, which stops any re-issue on its own.
            intent_push(channel, (IntentMessage){.kind = INTENT_DROP_BALL});
        }
        // otherwise we throw or move towards a base where the lead player is going. if the lead player is
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

            if (match->catching_action.kind == CATCHING_ACTION_NONE) {
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
                int baseman = match->fielding_posts.baseman[throwBase];
                if (match->catchingState.controlled_fielder != -1 && baseman != -1) {
                    Vector3D target = match->playerInfo[baseman].tPI.homeLocation;
                    if (match->catchingState.controlled_fielder != baseman) {
                        Vector3D baseTarget = throw_target_point(fieldPositions, (BaseID)throwBase);
                        target = carry_to_throw_point(
                            &match->playerInfo[match->catchingState.controlled_fielder].tPI.location, &baseTarget
                        );
                    }
                    steer_controlled_fielder(match, aiController, &target, channel);
                }
            }
            throw_ball_to_base(match, (BaseID)throwBase, channel);
        }
    }
}
