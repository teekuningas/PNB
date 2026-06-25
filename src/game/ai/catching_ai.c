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

static void update_ai_pitching(MatchSession* match, const HalfInningState* halfInningState, unsigned int* rng_seed)
{
    int pitcherIndex = match->pII.catcherOnBaseIndex[0];
    // here we finish pitching if started.
    // here i use these weird lock timeouts. im not sure if they are necessary
    // but they could be. not gonna try anymore.
    if (match->aiState.pitchStage == 1) {
        if (match->pendingActionState.aiLockTimeoutCounter == -1) {
            match->pendingActionState.aiLockTimeoutCounter = 0;
        }
        if (match->pendingActionState.meter_counter > match->aiState.pitchFirstLimit) {
            match->aiState.pitchStage = 2;
            match->aF.cTAF.pitch = PITCH_ACTION_POWER_SET;
            match->pendingActionState.aiLockTimeoutCounter = -1;
        } else {
            match->pendingActionState.aiLockTimeoutCounter++;
            if (match->pendingActionState.aiLockTimeoutCounter > TIMEOUT_CONSTANT) {
                match->aiState.pitchStage = 0;
                match->pendingActionState.aiActionEventLock = AI_NO_LOCK;
                match->pendingActionState.aiLockUpdate = 1;
                match->pendingActionState.aiLockTimeoutCounter = -1;
            }
        }
    } else if (match->aiState.pitchStage == 2) {
        if (match->pendingActionState.aiLockTimeoutCounter == -1) {
            match->pendingActionState.aiLockTimeoutCounter = 0;
        }
        if (match->pendingActionState.meter_counter > match->aiState.pitchSecondLimit) {
            match->aiState.pitchStage = 3;
            match->aF.cTAF.pitch = PITCH_ACTION_ANGLE_SET;
            match->pendingActionState.aiLockTimeoutCounter = -1;
        } else {
            match->pendingActionState.aiLockTimeoutCounter++;
            if (match->pendingActionState.aiLockTimeoutCounter > TIMEOUT_CONSTANT) {
                match->aiState.pitchStage = 0;
                match->pendingActionState.aiActionEventLock = AI_NO_LOCK;
                match->pendingActionState.aiLockUpdate = 1;
                match->pendingActionState.aiLockTimeoutCounter = -1;
            }
        }
    } else if (match->aiState.pitchStage == 3) {
        match->aiState.pitchStage = 0;
        match->pendingActionState.aiActionEventLock = AI_NO_LOCK;
        match->pendingActionState.aiLockUpdate = 1;
    }

    // if pitcher has the ball and he is in correct position
    if (match->pII.hasBallIndex == pitcherIndex && match->playerInfo[pitcherIndex].cTPI.isNearHomeLocation == 1) {
        // and lets give player some time to prepare
        if (match->aiState.batterReadyTimer > 70) {
            // try pitching.
            if (match->aiState.pitchStage == 0) {
                int i;
                int homeLocationFlag = 1;
                int pitchFlag = 0;

                match->aiState.pitchTime++;
                if (match->aiState.pitchTime >= 100) {
                    pitchFlag = 1;
                }
                for (i = PLAYERS_IN_TEAM + JOKER_COUNT; i < 2 * PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
                    if (match->playerInfo[i].cTPI.isNearHomeLocation == 0) {
                        homeLocationFlag = 0;
                    }
                }
                if (match->pendingActionState.aiActionEventLock == AI_NO_LOCK &&
                    match->pendingActionState.aiLockUpdate == 0) {
                    if (homeLocationFlag == 1 && pitchFlag == 1) {
                        int rand1 = seeded_rand(rng_seed, 15);
                        int rand2 = seeded_rand(rng_seed, 3);
                        int rand3 = seeded_rand(rng_seed, 10);

                        match->pendingActionState.aiActionEventLock = AI_PITCH_LOCK;
                        match->pendingActionState.aiLockUpdate = 1;
                        match->aiState.pitchStage = 1;
                        match->aF.cTAF.pitch = PITCH_ACTION_START;

                        int onFieldCount = count_active_batting_players(match->playerInfo);
                        calculate_ai_pitch_targets(
                            rand1, rand2, rand3, onFieldCount, halfInningState, ANIMATION_FREQUENCY,
                            &(match->aiState.pitchFirstLimit), &(match->aiState.pitchSecondLimit)
                        );
                    }
                }
            }
        }
    }

    if (match->aiState.pitchPreviousTime == match->aiState.pitchTime) {
        match->aiState.pitchTime = 0;
    }
    match->aiState.pitchPreviousTime = match->aiState.pitchTime;
    // this batterReadyTimer is used to give human player a bit more time before AI pitches.
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

void init_catching_ai(AIState* aiState)
{
    aiState->moveCounter = 0;
}

// we move towards the target position by simulating movement intent directly.
void move_controlled_player_to_location(MatchSession* match, Vector3D* target)
{
    float px = match->playerInfo[match->pII.controlIndex].tPI.location.x;
    float pz = match->playerInfo[match->pII.controlIndex].tPI.location.z;
    float tx = target->x;
    float tz = target->z;
    float dx = tx - px;
    float dz = tz - pz;

    if (!is_vector_small_enough_circle_xz(dx, dz, 1.0f) && match->pendingActionState.throw_going_on == 0) {
        if (match->aiState.moveCounter >= 10) {
            MovementKeys keys = calculate_movement_keys(dx, dz);
            // Set triggers only if NOT already moving in that direction to avoid unnecessary resets
            if (keys.up) {
                if (match->aF.cTAF.move[0] == ACTION_IDLE) match->aF.cTAF.move[0] = ACTION_TRIGGER_START;
            } else {
                if (match->aF.cTAF.move[0] != ACTION_IDLE) match->aF.cTAF.move[0] = ACTION_TRIGGER_STOP;
            }

            if (keys.right) {
                if (match->aF.cTAF.move[1] == ACTION_IDLE) match->aF.cTAF.move[1] = ACTION_TRIGGER_START;
            } else {
                if (match->aF.cTAF.move[1] != ACTION_IDLE) match->aF.cTAF.move[1] = ACTION_TRIGGER_STOP;
            }

            if (keys.down) {
                if (match->aF.cTAF.move[2] == ACTION_IDLE) match->aF.cTAF.move[2] = ACTION_TRIGGER_START;
            } else {
                if (match->aF.cTAF.move[2] != ACTION_IDLE) match->aF.cTAF.move[2] = ACTION_TRIGGER_STOP;
            }

            if (keys.left) {
                if (match->aF.cTAF.move[3] == ACTION_IDLE) match->aF.cTAF.move[3] = ACTION_TRIGGER_START;
            } else {
                if (match->aF.cTAF.move[3] != ACTION_IDLE) match->aF.cTAF.move[3] = ACTION_TRIGGER_STOP;
            }

            match->aiState.moveCounter = 0;
        }
    } else {
        int i;
        for (i = 0; i < 4; i++) {
            if (match->aF.cTAF.move[i] != ACTION_IDLE) {
                match->aF.cTAF.move[i] = ACTION_TRIGGER_STOP;
            }
        }
        match->aiState.moveCounter = 0;
    }
    match->aiState.moveCounter++;
}

void throw_ball_to_base(MatchSession* match, BaseID base)
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
        // Declare the throw command directly: which base + how hard. The engine actualizes it through
        // the windup dance (execute_actions / throwing_system); the AI does not puppeteer a meter.
        match->aF.cTAF.throw.target = base;
        match->aF.cTAF.throw.power = THROW_POWER_DEFAULT;
    }
}

void update_catching_ai(MatchSession* match, const GameRulesState* rules, unsigned int* rng_seed)
{
    // Update AI pitching
    update_ai_pitching(match, &rules->halfInningState, rng_seed);

    // (The throw no longer needs an AI-side "finish throwing" step: the engine owns the windup and
    // releases the ball when it completes — PLAN §4.12 sub-step 2. throwStage / AI_THROW_LOCK / the
    // meter-watch / the timeout-abort fallback all deleted with it.)

    // if noone has ball and someone is controlled, ai will try to move towards the target point calculated
    // in game_manipulation.
    if (match->pII.hasBallIndex == -1 && match->pII.controlIndex != -1) {
        if (match->pendingActionState.aiActionEventLock == AI_NO_LOCK && match->pendingActionState.aiLockUpdate == 0) {
            if (match->pRAI.throw_going_to_base == -1 || match->ballInfo.currentFlightHasHitGround == 1) {
                move_controlled_player_to_location(match, &(match->cameraState.targetPoint));
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
            // Declare the drop command directly (intent). drop_ball() in throwing_system.c is
            // instantaneous and self-guarded — it no-ops unless this catcher still holds the ball and
            // no throw/pitch is in flight — so the old AI_DROP_LOCK / dropStage wrapper around it was
            // pure redundancy of the execution-side state (the §8.1 duplicate-state-machine smell). The
            // execution-side `hasBallIndex` flips to -1 the frame the drop is consumed, preventing any
            // re-issue on its own.
            match->aF.cTAF.drop_ball = ACTION_TRIGGER_START;
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

            int randomVal = seeded_rand(rng_seed, 500);
            leadBase = determine_lead_base(runners, runnerCount, randomVal);

            if (leadBase != BASE_NONE && base_cmp(leadBase, BASE_THIRD) < 0)
                throwBase = (int)base_get_next(leadBase);
            else
                throwBase = 0;

            if (match->pendingActionState.aiActionEventLock == AI_NO_LOCK &&
                match->pendingActionState.aiLockUpdate == 0) {
                Vector3D target;
                target.x = match->playerInfo[match->pII.catcherOnBaseIndex[throwBase]].tPI.homeLocation.x;
                target.z = match->playerInfo[match->pII.catcherOnBaseIndex[throwBase]].tPI.homeLocation.z;
                move_controlled_player_to_location(match, &target);
            }
            throw_ball_to_base(match, (BaseID)throwBase);
        }
    }
    if (match->pendingActionState.aiLockUpdate == 1) {
        match->pendingActionState.aiLockUpdate = 0;
    }
}
