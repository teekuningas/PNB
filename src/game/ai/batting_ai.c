#include <math.h>
#include <stdlib.h>

#include "globals.h"
#include "ai/batting_ai.h"
#include "batting_ai_strategy.h"
#include "execute_actions.h"
#include "actions/batting_system.h"
#include "game_manipulation.h"
#include "rng.h"
#include "base_logic.h"
#include "base_control.h"
#include "rules_pure/player_utils.h"

void init_batting_ai(AIState* aiState)
{
    aiState->battingKeyDown = 0;
    aiState->changingKeyDown = 0;
    aiState->actionKeyLock = AI_NO_LOCK;
    aiState->battingStyle = 0;
    aiState->runningBatter = 0;
    aiState->runningBaseRunners = 0;

    aiState->increaseKeyDown = 0;
    aiState->decreaseKeyDown = 0;
    aiState->angleDecided = 0;
    aiState->decidedAngle = 0.0f;
    aiState->decidedSwingTrigger = BAT_SWING_MAX - 10;
    aiState->aiWrongPitch = 0;
    aiState->planCalculated = 0;
    aiState->firstIndex = -1;
    aiState->firstIndexSelected = 0;
    aiState->change = 0;
    aiState->changeHasHappened = 0;
    aiState->batterChangeCount = 0;
}

void update_batting_ai(
    MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions, unsigned int* rng_seed
)
{
    int i;
    int okToAdvanceAfterHit = 0;

    // Cleanup dangling locks if state changed externally
    if (match->flowControl.waitingForBatterDecision == 0) {
        if (match->aiState.actionKeyLock == AI_WAITING_BATTER_LOCK || match->aiState.actionKeyLock == AI_CHANGE_LOCK) {
            match->aiState.actionKeyLock = AI_NO_LOCK;
            match->aiState.battingKeyDown = 0;
            match->aiState.changingKeyDown = 0;
        }
    } else if (match->aiState.actionKeyLock == AI_BATTING_LOCK) {
        // A swing lock is only valid while a ready batter is mid-swing. Once a new batter
        // decision has begun, a lingering AI_BATTING_LOCK is orphaned: the at-bat ended (e.g.
        // a 3rd-strike forced run) before the swing's release path — which lives inside the
        // batter_ready block below — could clear it. The selection branch requires AI_NO_LOCK,
        // so without this self-heal the AI batting team deadlocks the entire game.
        match->aiState.actionKeyLock = AI_NO_LOCK;
        match->aiState.battingKeyDown = 0;
    }
    if (match->flowControl.waitingForFreeWalkDecision == 0) {
        if (match->aiState.actionKeyLock == AI_WAITING_WALK_LOCK) {
            match->aiState.actionKeyLock = AI_NO_LOCK;
            match->aiState.battingKeyDown = 0;
        }
    }

    // A fresh pitch cycle forces the swing plan to be recomputed. The per-base run decisions
    // need no bookkeeping reset: the AI re-derives them every frame from live game state
    // (will_start_running + each runner's PlayerUnitState), so they cannot go stale.
    if (match->pRAI.batter_ready == 0 && match->aiState.planCalculated == 1) {
        match->aiState.planCalculated = 0;
    }
    // make free walk decision == accept
    if (match->flowControl.waitingForFreeWalkDecision == 1) {
        if (match->aiState.battingKeyDown == 0) {
            if (match->aiState.actionKeyLock == AI_NO_LOCK) {
                match->aF.bTAF.take_free_walk = FREE_WALK_ACCEPT;
                match->aiState.battingKeyDown = 1;
                match->aiState.actionKeyLock = AI_WAITING_WALK_LOCK;
            }
        } else {
            match->aiState.actionKeyLock = AI_NO_LOCK;
            match->aiState.battingKeyDown = 0;
        }
    }
    // we decide batter only after ball is at home so that in normal situation ai will have more information
    // to make its strategy decisions
    if (match->flowControl.waitingForBatterDecision == 1 && match->gameFlowState.ballHome == 1) {
        // we do this by brute force, we change player until we find a fit one or we are back to non joker.
        // plan is that if there is a man on first base and current batter would not have a great power,
        // we would try to find a joker that has power instead.
        // and if field is empty we would change a joker with speed instead.
        int firstBaseIndex = get_base_controller(match, &rules->referee, (BaseID)1);
        int secondBaseIndex = get_base_controller(match, &rules->referee, (BaseID)2);
        int thirdBaseIndex = get_base_controller(match, &rules->referee, (BaseID)3);
        int fieldStatus;
        int index = match->pII.batterSelectionIndex;

        if (firstBaseIndex != -1)
            fieldStatus = 2;
        else if (secondBaseIndex != -1 || thirdBaseIndex != -1)
            fieldStatus = 1;
        else
            fieldStatus = 0;

        match->aiState.change =
            should_change_batter(fieldStatus, match->playerInfo[index].bTPI.power, match->playerInfo[index].bTPI.speed);

        if (match->aiState.firstIndexSelected == 0) {
            match->aiState.firstIndex = index;
            match->aiState.firstIndexSelected = 1;
            match->aiState.batterChangeCount = 0;
        } else if (match->aiState.changeHasHappened == 1) {
            if (match->aiState.firstIndex == index) {
                match->aiState.change = 0;
            }
        }
        // BAND-AID (§3.1, bug #5 — dies with the swing slice): the firstIndex==index breaker above can be
        // skipped when change_batter steps over a JOKER_USED slot, looping forever. There are only
        // JOKER_COUNT+1 distinct selectable slots, so once we've issued more changes than that we have
        // certainly seen them all — force a select instead of cycling on.
        if (match->aiState.batterChangeCount > JOKER_COUNT) {
            match->aiState.change = 0;
        }

        // change player
        if (match->aiState.change == 1 && match->aiState.changingKeyDown == 0 &&
            match->aiState.actionKeyLock == AI_NO_LOCK) {
            match->aF.bTAF.choose_batter = CHOOSE_BATTER_NEXT;
            match->aiState.changingKeyDown = 1;
            match->aiState.actionKeyLock = AI_CHANGE_LOCK;
            match->aiState.batterChangeCount++;
        } else if (match->aiState.changingKeyDown == 1 && match->aiState.actionKeyLock == AI_CHANGE_LOCK) {
            match->aiState.actionKeyLock = AI_NO_LOCK;
            match->aiState.changingKeyDown = 0;
            match->aiState.changeHasHappened = 1;
        }
        // select best batter.
        if (match->aiState.change == 0 && match->aiState.battingKeyDown == 0 &&
            match->aiState.actionKeyLock == AI_NO_LOCK) {
            match->aF.bTAF.choose_batter = CHOOSE_BATTER_SELECT;
            match->aiState.battingKeyDown = 1;
            match->aiState.actionKeyLock = AI_WAITING_BATTER_LOCK;
        } else if (match->aiState.battingKeyDown == 1 && match->aiState.actionKeyLock == AI_WAITING_BATTER_LOCK) {
            match->aiState.actionKeyLock = AI_NO_LOCK;
            match->aiState.battingKeyDown = 0;
            match->aiState.firstIndex = -1;
            match->aiState.firstIndexSelected = 0;
            match->aiState.changeHasHappened = 0;
        }

    } else if (match->pRAI.batter_ready == 1 && match->pRAI.pitch_state != PITCH_STAGE_AIRBORNE &&
               match->gameFlowState.ballHome == 1) {
        // decision tree.. contents can be read within
        if (match->aiState.planCalculated == 0) {
            int batterIndex = get_active_batter_index(match);
            int firstBaseIndex = get_base_controller(match, &rules->referee, (BaseID)1);
            int secondBaseIndex = get_base_controller(match, &rules->referee, (BaseID)2);
            int thirdBaseIndex = get_base_controller(match, &rules->referee, (BaseID)3);
            int power = match->playerInfo[batterIndex].bTPI.power;
            int speed = match->playerInfo[batterIndex].bTPI.speed;
            int fieldStatus;

            if (firstBaseIndex != -1)
                fieldStatus = 2;
            else if (secondBaseIndex != -1 || thirdBaseIndex != -1)
                fieldStatus = 1;
            else
                fieldStatus = 0;

            BattingStrategy strategy = calculate_batting_strategy(
                &(rules->halfInningState), fieldStatus, power, speed, rules->scoreboard.period
            );

            match->aiState.battingStyle = strategy.style;
            match->aiState.runningBaseRunners = strategy.runBaseRunners;
            match->aiState.runningBatter = strategy.runBatter;

            match->aiState.planCalculated = 1;
        }
        // Arm the batter to advance — the run is committed on contact. Declared once: the
        // command's actualization sets will_start_running[BASE_HOME], which makes the guard
        // below false next frame (no click bookkeeping needed).
        if (match->aiState.runningBatter == 1 && match->pRAI.will_start_running[BASE_HOME] == 0) {
            match->aF.bTAF.base_run[BASE_HOME] = RUN_FORWARD;
        }
        // Arm on-base runners to advance on the pitch. Same self-limiting: once a runner is
        // armed (and, on 1st/2nd, leads off) it is no longer ON_BASE-and-unarmed, so it is not
        // re-commanded.
        if (match->aiState.runningBaseRunners == 1) {
            for (i = 1; i < BASE_COUNT; i++) {
                int index = get_base_controller(match, &rules->referee, (BaseID)i);
                if (index != -1 && match->playerInfo[index].bTPI.state == PLAYER_STATE_ON_BASE &&
                    match->pRAI.will_start_running[i] == 0) {
                    match->aF.bTAF.base_run[i] = RUN_FORWARD;
                }
            }
        }
    }
    // if ball is not home, we return players from first and second base to their bases
    else if (match->pRAI.batter_ready == 1 && match->pRAI.pitch_state != PITCH_STAGE_AIRBORNE &&
             match->gameFlowState.ballHome == 0) {
        // The ball came back home with no pitch in the air: pull any leading 1st/2nd runners
        // back onto their base. RUN_BACK on a leading runner makes it run back; once it is no
        // longer LEADING the command stops being issued.
        if (match->aiState.runningBaseRunners == 1) {
            for (i = 1; i < 3; i++) {
                int index = get_base_controller(match, &rules->referee, (BaseID)i);
                if (index != -1 && match->playerInfo[index].bTPI.state == PLAYER_STATE_LEADING) {
                    match->aF.bTAF.base_run[i] = RUN_BACK;
                }
            }
        }
    }
    // and here we bat
    else if (match->pRAI.pitch_state == PITCH_STAGE_AIRBORNE) {
        int i;
        // predict if pitch is going to be ball
        if (match->aiState.aiWrongPitch == 0 &&
            is_wrong_pitch(match->ballInfo.velocity.x, match->ballInfo.velocity.y, GRAVITY, PLATE_WIDTH)) {
            match->aiState.aiWrongPitch = 1;
        }
        if (match->aiState.aiWrongPitch == 1) {
            // The batter is handled below; here we abort any runner who already broke for the
            // next base, since the pitch is going to be a ball. RUN_BACK turns a forward run
            // into a return; once it is no longer going forward the command stops.
            for (i = 1; i < BASE_COUNT; i++) {
                int index = get_base_controller(match, &rules->referee, (BaseID)i);
                if (index != -1 && match->playerRuntime[index].goingForward == 1) {
                    match->aF.bTAF.base_run[i] = RUN_BACK;
                }
            }
        }
        // a bunt
        if (match->aiState.battingStyle == 0) {
            if (match->aiState.angleDecided == 0) {
                match->aiState.decidedAngle = calculate_ai_batting_angle(0, seeded_rand(rng_seed, RAND_MAX));
                match->aiState.angleDecided = 1;
            }
            if (match->pendingActionState.meter_counter > BAT_SWING_MAX - 23 && match->aiState.battingKeyDown == 0 &&
                match->aiState.actionKeyLock == AI_NO_LOCK && match->aiState.aiWrongPitch == 0) {
                match->aF.bTAF.swing = BAT_ACTION_POWER_SET;
                match->aiState.battingKeyDown = 1;
                match->aiState.actionKeyLock = AI_BATTING_LOCK;
            } else if (match->aiState.battingKeyDown == 1 && match->aiState.actionKeyLock == AI_BATTING_LOCK) {
                if (match->pendingActionState.meter_counter > BAT_LOAD_MAX - 9) {
                    match->aF.bTAF.swing = BAT_ACTION_ANGLE_SET;
                    match->aiState.battingKeyDown = 0;
                    match->aiState.actionKeyLock = AI_NO_LOCK;
                }
            }
        }
        // a normal swing
        else if (match->aiState.battingStyle == 1) {
            if (match->aiState.angleDecided == 0) {
                // Direction: randomized across the field, independent of base runners.
                match->aiState.decidedAngle = calculate_ai_batting_angle(1, seeded_rand(rng_seed, RAND_MAX));
                // Power: release the swing at a random meter level so power varies between
                // at-bats (kept in a competent mid-to-strong band — no bunts, no overflow).
                match->aiState.decidedSwingTrigger =
                    BAT_SWING_MAX - 4 - seeded_rand(rng_seed, 19); // ~[BAT_SWING_MAX-22 .. BAT_SWING_MAX-4]
                match->aiState.angleDecided = 1;
            }
            if (match->pendingActionState.meter_counter > match->aiState.decidedSwingTrigger &&
                match->aiState.battingKeyDown == 0 && match->aiState.actionKeyLock == AI_NO_LOCK &&
                match->aiState.aiWrongPitch == 0) {
                match->aF.bTAF.swing = BAT_ACTION_POWER_SET;
                match->aiState.battingKeyDown = 1;
                match->aiState.actionKeyLock = AI_BATTING_LOCK;
            } else if (match->aiState.battingKeyDown == 1 && match->aiState.actionKeyLock == AI_BATTING_LOCK) {
                if (match->pendingActionState.meter_counter > BAT_LOAD_MAX - 6) {
                    match->aF.bTAF.swing = BAT_ACTION_ANGLE_SET;
                    match->aiState.battingKeyDown = 0;
                    match->aiState.actionKeyLock = AI_NO_LOCK;
                }
            }
        }
        // swing that tries to get oneself wounded
        else if (match->aiState.battingStyle == 2) {
            if (match->aiState.angleDecided == 0) {
                match->aiState.decidedAngle = calculate_ai_batting_angle(2, seeded_rand(rng_seed, RAND_MAX));
                match->aiState.angleDecided = 1;
            }
            if (match->pendingActionState.meter_counter > BAT_SWING_MAX - 11 && match->aiState.battingKeyDown == 0 &&
                match->aiState.actionKeyLock == AI_NO_LOCK && match->aiState.aiWrongPitch == 0) {
                match->aF.bTAF.swing = BAT_ACTION_POWER_SET;
                match->aiState.battingKeyDown = 1;
                match->aiState.actionKeyLock = AI_BATTING_LOCK;
            } else if (match->aiState.battingKeyDown == 1 && match->aiState.actionKeyLock == AI_BATTING_LOCK) {
                if (match->pendingActionState.meter_counter > BAT_LOAD_MAX - 8) {
                    match->aF.bTAF.swing = BAT_ACTION_ANGLE_SET;
                    match->aiState.battingKeyDown = 0;
                    match->aiState.actionKeyLock = AI_NO_LOCK;
                }
            }
        }
        if (match->aiState.decidedAngle >= 0 && match->pendingActionState.batter_angle < match->aiState.decidedAngle &&
            match->aiState.increaseKeyDown == 0) {
            match->aF.bTAF.increase_batter_angle = ACTION_TRIGGER_START;
            match->aiState.increaseKeyDown = 1;
        } else if (match->pendingActionState.batter_angle >= match->aiState.decidedAngle &&
                   match->aiState.increaseKeyDown == 1) {
            match->aF.bTAF.increase_batter_angle = ACTION_TRIGGER_STOP;
            match->aiState.increaseKeyDown = 0;
        }

        if (match->aiState.decidedAngle < 0 && match->pendingActionState.batter_angle > match->aiState.decidedAngle &&
            match->aiState.decreaseKeyDown == 0) {
            match->aF.bTAF.decrease_batter_angle = ACTION_TRIGGER_START;
            match->aiState.decreaseKeyDown = 1;
        } else if (match->pendingActionState.batter_angle <= match->aiState.decidedAngle &&
                   match->aiState.decreaseKeyDown == 1) {
            match->aF.bTAF.decrease_batter_angle = ACTION_TRIGGER_STOP;
            match->aiState.decreaseKeyDown = 0;
        }
    }
    if (match->pRAI.pitch_state != PITCH_STAGE_AIRBORNE && match->aiState.angleDecided == 1) {
        match->aiState.angleDecided = 0;
    }
    // AI: Check if it's safe to advance runners
    // Ball was hit, not caught, no one has it, no throw in progress, and ball is physically outside field
    // AND ball has traveled far enough from home (to avoid triggering when ball is still at home plate)
    // AND ball has already hit the ground (if still airborne and out of bounds, it's a foul/läpilyönti)
    if (rules->betweenPitchState.batOutcome == BAT_OUTCOME_HIT && rules->betweenPitchState.catchHasBeenMade == 0 &&
        match->pRAI.throw_going_to_base == -1 && match->pII.hasBallIndex == -1 && match->ballInfo.moving == 1 &&
        rules->betweenPitchState.hasBallHitGround == 1 && match->ballInfo.location.z < -10.0f &&
        is_ball_out_of_bounds(&match->ballInfo, fieldPositions)) {
        okToAdvanceAfterHit = 1;
    }
    // Once the hit is safely loose in the field, send every eligible runner to the next base.
    // This is a deliberate "run now", so we declare RUN_COMMIT (the AI's equivalent of a human
    // double-press) — NOT RUN_FORWARD, which only arms. Once a runner is goingForward we stop
    // re-issuing, so the old double-click simulation (amountOfClicks / clickBreak / locks) is gone.
    if (okToAdvanceAfterHit) {
        for (i = 0; i < BASE_COUNT; i++) {
            int j;
            int index = get_base_controller(match, &rules->referee, (BaseID)i);
            int shouldRun = 1;
            if (i == 0 && match->pRAI.batter_can_advance == 0) continue;
            if (index == -1 || match->playerRuntime[index].goingForward == 1) continue;
            // don't send a runner into a base interval someone else already occupies.
            for (j = 0; j < PLAYERS_IN_TEAM + JOKER_COUNT; j++) {
                BaseID bid = match->playerInfo[j].bTPI.baseId;
                if (bid != BASE_NONE && base_to_int_index(bid) == i && j != index) {
                    shouldRun = 0;
                }
            }
            if (shouldRun) {
                match->aF.bTAF.base_run[i] = RUN_COMMIT;
            }
        }
    }
}
