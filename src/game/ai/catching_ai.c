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

static void update_ai_pitching(StateInfo* stateInfo, unsigned int* rng_seed)
{
    int pitcherIndex = stateInfo->match->pII.catcherOnBaseIndex[0];
    // here we finish pitching if started.
    // here i use these weird lock timeouts. im not sure if they are necessary
    // but they could be. not gonna try anymore.
    if (stateInfo->match->aiState.pitchStage == 1) {
        if (stateInfo->match->pendingActionState.aiLockTimeoutCounter == -1) {
            stateInfo->match->pendingActionState.aiLockTimeoutCounter = 0;
        }
        if (stateInfo->match->pendingActionState.meter_counter > stateInfo->match->aiState.pitchFirstLimit) {
            stateInfo->match->aiState.pitchStage = 2;
            stateInfo->match->aF.cTAF.pitch = PITCH_ACTION_POWER_SET;
            stateInfo->match->pendingActionState.aiLockTimeoutCounter = -1;
        } else {
            stateInfo->match->pendingActionState.aiLockTimeoutCounter++;
            if (stateInfo->match->pendingActionState.aiLockTimeoutCounter > TIMEOUT_CONSTANT) {
                stateInfo->match->aiState.pitchStage = 0;
                stateInfo->match->pendingActionState.aiActionEventLock = AI_NO_LOCK;
                stateInfo->match->pendingActionState.aiLockUpdate = 1;
                stateInfo->match->pendingActionState.aiLockTimeoutCounter = -1;
            }
        }
    } else if (stateInfo->match->aiState.pitchStage == 2) {
        if (stateInfo->match->pendingActionState.aiLockTimeoutCounter == -1) {
            stateInfo->match->pendingActionState.aiLockTimeoutCounter = 0;
        }
        if (stateInfo->match->pendingActionState.meter_counter > stateInfo->match->aiState.pitchSecondLimit) {
            stateInfo->match->aiState.pitchStage = 3;
            stateInfo->match->aF.cTAF.pitch = PITCH_ACTION_ANGLE_SET;
            stateInfo->match->pendingActionState.aiLockTimeoutCounter = -1;
        } else {
            stateInfo->match->pendingActionState.aiLockTimeoutCounter++;
            if (stateInfo->match->pendingActionState.aiLockTimeoutCounter > TIMEOUT_CONSTANT) {
                stateInfo->match->aiState.pitchStage = 0;
                stateInfo->match->pendingActionState.aiActionEventLock = AI_NO_LOCK;
                stateInfo->match->pendingActionState.aiLockUpdate = 1;
                stateInfo->match->pendingActionState.aiLockTimeoutCounter = -1;
            }
        }
    } else if (stateInfo->match->aiState.pitchStage == 3) {
        stateInfo->match->aiState.pitchStage = 0;
        stateInfo->match->pendingActionState.aiActionEventLock = AI_NO_LOCK;
        stateInfo->match->pendingActionState.aiLockUpdate = 1;
    }

    // if pitcher has the ball and he is in correct position
    if (stateInfo->match->pII.hasBallIndex == pitcherIndex &&
        stateInfo->match->playerInfo[pitcherIndex].cTPI.isNearHomeLocation == 1) {
        // and lets give player some time to prepare
        if (stateInfo->match->aiState.batterReadyTimer > 70) {
            // try pitching.
            if (stateInfo->match->aiState.pitchStage == 0) {
                int i;
                int homeLocationFlag = 1;
                int pitchFlag = 0;

                stateInfo->match->aiState.pitchTime++;
                if (stateInfo->match->aiState.pitchTime >= 100) {
                    pitchFlag = 1;
                }
                for (i = PLAYERS_IN_TEAM + JOKER_COUNT; i < 2 * PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
                    if (stateInfo->match->playerInfo[i].cTPI.isNearHomeLocation == 0) {
                        homeLocationFlag = 0;
                    }
                }
                if (stateInfo->match->pendingActionState.aiActionEventLock == AI_NO_LOCK &&
                    stateInfo->match->pendingActionState.aiLockUpdate == 0) {
                    if (homeLocationFlag == 1 && pitchFlag == 1) {
                        int rand1 = seeded_rand(rng_seed, 15);
                        int rand2 = seeded_rand(rng_seed, 3);
                        int rand3 = seeded_rand(rng_seed, 10);

                        stateInfo->match->pendingActionState.aiActionEventLock = AI_PITCH_LOCK;
                        stateInfo->match->pendingActionState.aiLockUpdate = 1;
                        stateInfo->match->aiState.pitchStage = 1;
                        stateInfo->match->aF.cTAF.pitch = PITCH_ACTION_START;

                        int onFieldCount = count_active_batting_players(stateInfo->match->playerInfo);
                        calculate_ai_pitch_targets(
                            rand1, rand2, rand3, onFieldCount, &(stateInfo->rules->halfInningState),
                            ANIMATION_FREQUENCY, &(stateInfo->match->aiState.pitchFirstLimit),
                            &(stateInfo->match->aiState.pitchSecondLimit)
                        );
                    }
                }
            }
        }
    }

    if (stateInfo->match->aiState.pitchPreviousTime == stateInfo->match->aiState.pitchTime) {
        stateInfo->match->aiState.pitchTime = 0;
    }
    stateInfo->match->aiState.pitchPreviousTime = stateInfo->match->aiState.pitchTime;
    // this batterReadyTimer is used to give human player a bit more time before AI pitches.
    if (stateInfo->match->pRAI.batter_ready == 1 &&
        stateInfo->match->pII.catcherOnBaseIndex[0] == stateInfo->match->pII.hasBallIndex &&
        stateInfo->match->aiState.batterReadyTimer == -1) {
        stateInfo->match->aiState.batterReadyTimer = 0;
    } else if (stateInfo->match->pRAI.batter_ready == 0) {
        stateInfo->match->aiState.batterReadyTimer = -1;
    }
    if (stateInfo->match->aiState.batterReadyTimer != -1) {
        stateInfo->match->aiState.batterReadyTimer++;
    }
}

void init_catching_ai(AIState* aiState)
{
    aiState->dropStage = 0;
    aiState->throwStage = 0;
    aiState->moveCounter = 0;
}

// we move towards the target position by simulating movement intent directly.
void move_controlled_player_to_location(StateInfo* stateInfo, Vector3D* target)
{
    float px = stateInfo->match->playerInfo[stateInfo->match->pII.controlIndex].tPI.location.x;
    float pz = stateInfo->match->playerInfo[stateInfo->match->pII.controlIndex].tPI.location.z;
    float tx = target->x;
    float tz = target->z;
    float dx = tx - px;
    float dz = tz - pz;

    if (!is_vector_small_enough_circle_xz(dx, dz, 1.0f) && stateInfo->match->pendingActionState.throw_going_on == 0) {
        if (stateInfo->match->aiState.moveCounter >= 10) {
            MovementKeys keys = calculate_movement_keys(dx, dz);
            // Set triggers only if NOT already moving in that direction to avoid unnecessary resets
            if (keys.up) {
                if (stateInfo->match->aF.cTAF.move[0] == ACTION_IDLE)
                    stateInfo->match->aF.cTAF.move[0] = ACTION_TRIGGER_START;
            } else {
                if (stateInfo->match->aF.cTAF.move[0] != ACTION_IDLE)
                    stateInfo->match->aF.cTAF.move[0] = ACTION_TRIGGER_STOP;
            }

            if (keys.right) {
                if (stateInfo->match->aF.cTAF.move[1] == ACTION_IDLE)
                    stateInfo->match->aF.cTAF.move[1] = ACTION_TRIGGER_START;
            } else {
                if (stateInfo->match->aF.cTAF.move[1] != ACTION_IDLE)
                    stateInfo->match->aF.cTAF.move[1] = ACTION_TRIGGER_STOP;
            }

            if (keys.down) {
                if (stateInfo->match->aF.cTAF.move[2] == ACTION_IDLE)
                    stateInfo->match->aF.cTAF.move[2] = ACTION_TRIGGER_START;
            } else {
                if (stateInfo->match->aF.cTAF.move[2] != ACTION_IDLE)
                    stateInfo->match->aF.cTAF.move[2] = ACTION_TRIGGER_STOP;
            }

            if (keys.left) {
                if (stateInfo->match->aF.cTAF.move[3] == ACTION_IDLE)
                    stateInfo->match->aF.cTAF.move[3] = ACTION_TRIGGER_START;
            } else {
                if (stateInfo->match->aF.cTAF.move[3] != ACTION_IDLE)
                    stateInfo->match->aF.cTAF.move[3] = ACTION_TRIGGER_STOP;
            }

            stateInfo->match->aiState.moveCounter = 0;
        }
    } else {
        int i;
        for (i = 0; i < 4; i++) {
            if (stateInfo->match->aF.cTAF.move[i] != ACTION_IDLE) {
                stateInfo->match->aF.cTAF.move[i] = ACTION_TRIGGER_STOP;
            }
        }
        stateInfo->match->aiState.moveCounter = 0;
    }
    stateInfo->match->aiState.moveCounter++;
}

void throw_ball_to_base(StateInfo* stateInfo, BaseID base)
{
    if (stateInfo->match->aiState.throwStage == 0) {
        if (stateInfo->match->pendingActionState.aiActionEventLock == AI_NO_LOCK &&
            stateInfo->match->pendingActionState.aiLockUpdate == 0) {
            int catcherIndex = stateInfo->match->pII.catcherOnBaseIndex[base];
            int catcherNearHome = 0;
            if (catcherIndex != -1) {
                catcherNearHome = stateInfo->match->playerInfo[catcherIndex].cTPI.isNearHomeLocation;
            }

            int replacerIndex = stateInfo->match->pII.catcherReplacerOnBaseIndex[base];
            ReplacementState replacerStage = REPLACEMENT_IDLE;
            int replacerBase = -1;
            int replacerMoving = 0;
            if (replacerIndex != -1) {
                replacerStage = stateInfo->match->playerInfo[replacerIndex].cTPI.replacingStage;
                replacerBase = stateInfo->match->playerInfo[replacerIndex].cTPI.replacingBase;
                replacerMoving = stateInfo->match->playerInfo[replacerIndex].cPI.moving;
            }

            int shouldThrow = should_ai_throw(
                &(stateInfo->match->pII), catcherNearHome, replacerIndex, replacerStage, replacerBase, replacerMoving,
                base
            );

            if (shouldThrow == 1) {
                stateInfo->match->aiState.throwStage = 1;
                stateInfo->match->pendingActionState.aiLockUpdate = 1;
                stateInfo->match->pendingActionState.aiActionEventLock = AI_THROW_LOCK;

                // AI sets trigger directly
                stateInfo->match->aF.cTAF.throw_to_base[base] = ACTION_TRIGGER_START;
            }
        }
    }
}

void update_catching_ai(StateInfo* stateInfo, unsigned int* rng_seed)
{
    // Update AI pitching
    update_ai_pitching(stateInfo, rng_seed);

    // finish dropping
    if (stateInfo->match->aiState.dropStage == 1) {
        stateInfo->match->pendingActionState.aiActionEventLock = AI_NO_LOCK;
        stateInfo->match->aiState.dropStage = 0;
        stateInfo->match->pendingActionState.aiLockUpdate = 1;
    }
    // finish throwing
    if (stateInfo->match->aiState.throwStage == 1) {
        if (stateInfo->match->pendingActionState.aiLockTimeoutCounter == -1) {
            stateInfo->match->pendingActionState.aiLockTimeoutCounter = 0;
        }
        if (stateInfo->match->pendingActionState.meter_counter > THROW_MAX * (3.0f / 4)) {
            stateInfo->match->aiState.throwStage = 0;
            stateInfo->match->pendingActionState.aiActionEventLock = AI_NO_LOCK;
            stateInfo->match->pendingActionState.aiLockUpdate = 1;
            stateInfo->match->pendingActionState.aiLockTimeoutCounter = -1;

            // AI signals throw release via throw_going_to_base (set by prepare_throw)
            if (stateInfo->match->pRAI.throw_going_to_base >= 0 && stateInfo->match->pRAI.throw_going_to_base < BASE_COUNT) {
                stateInfo->match->aF.cTAF.throw_to_base[stateInfo->match->pRAI.throw_going_to_base] = ACTION_TRIGGER_STOP;
            }
        } else {
            stateInfo->match->pendingActionState.aiLockTimeoutCounter++;
            if (stateInfo->match->pendingActionState.aiLockTimeoutCounter > TIMEOUT_CONSTANT) {
                stateInfo->match->aiState.throwStage = 0;
                stateInfo->match->pendingActionState.aiActionEventLock = AI_NO_LOCK;
                stateInfo->match->pendingActionState.aiLockUpdate = 1;
                stateInfo->match->pendingActionState.aiLockTimeoutCounter = -1;

                // Force-abort the throw: clear execution state so auto-clear fires
                stateInfo->match->pendingActionState.throw_going_on = 0;
                stateInfo->match->pRAI.throw_going_to_base = -1;
            }
        }
    }
    // if noone has ball and someone is controlled, ai will try to move towards the target point calculated
    // in game_manipulation.
    if (stateInfo->match->pII.hasBallIndex == -1 && stateInfo->match->pII.controlIndex != -1) {
        if (stateInfo->match->pendingActionState.aiActionEventLock == AI_NO_LOCK &&
            stateInfo->match->pendingActionState.aiLockUpdate == 0) {
            if (stateInfo->match->pRAI.throw_going_to_base == -1 ||
                stateInfo->match->ballInfo.currentFlightHasHitGround == 1) {
                move_controlled_player_to_location(stateInfo, &(stateInfo->match->cameraState.targetPoint));
            }
        }
    }
    // if someone has ball
    if (stateInfo->match->pII.hasBallIndex != -1) {
        int index3 = get_base_controller(stateInfo->match, &stateInfo->rules->referee, (BaseID)3);
        int index2 = get_base_controller(stateInfo->match, &stateInfo->rules->referee, (BaseID)2);

        BaseID r3BaseAtPitchStart = BASE_NONE;
        int r3IsOnBase = 0;
        if (index3 != -1) {
            r3BaseAtPitchStart = stateInfo->rules->referee.battingPlayers[index3].baseAtPitchStart;
            r3IsOnBase = (stateInfo->match->playerInfo[index3].bTPI.state == PLAYER_STATE_ON_BASE);
        }

        BaseID r2BaseAtPitchStart = BASE_NONE;
        int r2IsOnBase = 0;
        if (index2 != -1) {
            r2BaseAtPitchStart = stateInfo->rules->referee.battingPlayers[index2].baseAtPitchStart;
            r2IsOnBase = (stateInfo->match->playerInfo[index2].bTPI.state == PLAYER_STATE_ON_BASE);
        }

        int catcherHomeIndex = stateInfo->match->pII.catcherOnBaseIndex[0];
        int hasBallIndex = stateInfo->match->pII.hasBallIndex;

        if (should_ai_drop_ball(
                &(stateInfo->rules->referee), &(stateInfo->rules->betweenPitchState), r3BaseAtPitchStart, r3IsOnBase,
                r2BaseAtPitchStart, r2IsOnBase, catcherHomeIndex, hasBallIndex
            )) {
            if (stateInfo->match->pendingActionState.aiActionEventLock == AI_NO_LOCK &&
                stateInfo->match->pendingActionState.aiLockUpdate == 0) {
                stateInfo->match->aiState.dropStage = 1;
                stateInfo->match->pendingActionState.aiLockUpdate = 1;
                stateInfo->match->pendingActionState.aiActionEventLock = AI_DROP_LOCK;
                stateInfo->match->aF.cTAF.drop_ball = ACTION_TRIGGER_START;
            }
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
                PlayerUnitState s = stateInfo->match->playerInfo[i].bTPI.state;
                BaseID bid = stateInfo->match->playerInfo[i].bTPI.baseId;

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

            if (stateInfo->match->pendingActionState.aiActionEventLock == AI_NO_LOCK &&
                stateInfo->match->pendingActionState.aiLockUpdate == 0) {
                Vector3D target;
                target.x = stateInfo->match->playerInfo[stateInfo->match->pII.catcherOnBaseIndex[throwBase]]
                               .tPI.homeLocation.x;
                target.z = stateInfo->match->playerInfo[stateInfo->match->pII.catcherOnBaseIndex[throwBase]]
                               .tPI.homeLocation.z;
                move_controlled_player_to_location(stateInfo, &target);
            }
            throw_ball_to_base(stateInfo, (BaseID)throwBase);
        }
    }
    if (stateInfo->match->pendingActionState.aiLockUpdate == 1) {
        stateInfo->match->pendingActionState.aiLockUpdate = 0;
    }
}
