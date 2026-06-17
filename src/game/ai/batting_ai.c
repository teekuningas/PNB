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

// Macros moved from execute_actions.c

#define CLICK_BREAK_CONSTANT 3

void init_batting_ai(AIState* aiState)
{
    int i;
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
    for (i = 0; i < BASE_COUNT; i++) {
        aiState->baseRunnerKeyDown[i] = 0;
        aiState->lastSafeOnBaseIndex[i] = -1;
        aiState->baseRunnerDecisionMade[i] = 0;
        aiState->amountOfClicks[i] = 0;
        aiState->baseRunnerLock[i] = AI_NO_LOCK;
        aiState->clickBreak[i] = 0;
    }
}

void update_batting_ai(
    MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions, unsigned int* rng_seed
)
{
    int i;
    int isDoubleClickingOk = 0;

    // Cleanup dangling locks if state changed externally
    if (match->flowControl.waitingForBatterDecision == 0) {
        if (match->aiState.actionKeyLock == AI_WAITING_BATTER_LOCK || match->aiState.actionKeyLock == AI_CHANGE_LOCK) {
            match->aiState.actionKeyLock = AI_NO_LOCK;
            match->aiState.battingKeyDown = 0;
            match->aiState.changingKeyDown = 0;
        }
    }
    if (match->flowControl.waitingForFreeWalkDecision == 0) {
        if (match->aiState.actionKeyLock == AI_WAITING_WALK_LOCK) {
            match->aiState.actionKeyLock = AI_NO_LOCK;
            match->aiState.battingKeyDown = 0;
        }
    }

    // update some flags
    for (i = 0; i < BASE_COUNT; i++) {
        match->aiState.clickBreak[i]++;
        if (match->aiState.clickBreak[i] > 1000) match->aiState.clickBreak[i] = 0;
        if (match->aiState.baseRunnerDecisionMade[i] == 1) {
            if (get_base_controller(match, &rules->referee, (BaseID)i) == -1) {
                match->aiState.baseRunnerDecisionMade[i] = 0;
            }
            if (match->aiState.lastSafeOnBaseIndex[i] != get_base_controller(match, &rules->referee, (BaseID)i)) {
                match->aiState.baseRunnerDecisionMade[i] = 0;
            }
        }
        match->aiState.lastSafeOnBaseIndex[i] = get_base_controller(match, &rules->referee, (BaseID)i);
    }
    if (match->pRAI.batter_ready == 0 && match->aiState.planCalculated == 1) {
        match->aiState.planCalculated = 0;
        // Reset all base-runner decision state for a fresh pitch cycle.
        // Without this, baseRunnerDecisionMade (especially for base 3, which has no
        // "come back from leading" path) stays stale across pitches, preventing the AI
        // from issuing new run commands to runners who stayed on their base.
        for (i = 0; i < BASE_COUNT; i++) {
            match->aiState.baseRunnerDecisionMade[i] = 0;
            match->aiState.baseRunnerKeyDown[i] = 0;
            match->aiState.baseRunnerLock[i] = AI_NO_LOCK;
            match->aiState.clickBreak[i] = 0;
        }
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
        } else if (match->aiState.changeHasHappened == 1) {
            if (match->aiState.firstIndex == index) {
                match->aiState.change = 0;
            }
        }

        // change player
        if (match->aiState.change == 1 && match->aiState.changingKeyDown == 0 &&
            match->aiState.actionKeyLock == AI_NO_LOCK) {
            match->aF.bTAF.choose_batter = CHOOSE_BATTER_NEXT;
            match->aiState.changingKeyDown = 1;
            match->aiState.actionKeyLock = AI_CHANGE_LOCK;
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
        // if we decide that batter should run, we click down once.
        if (match->aiState.runningBatter == 1) {
            if (match->aiState.baseRunnerDecisionMade[0] == 0 && match->aiState.baseRunnerKeyDown[0] == 0 &&
                match->aiState.baseRunnerLock[0] == AI_NO_LOCK && match->aiState.clickBreak[0] > CLICK_BREAK_CONSTANT) {
                match->aiState.baseRunnerKeyDown[0] = 1;
                match->aiState.baseRunnerLock[0] = AI_CLICK_LOCK;
                match->aF.bTAF.base_run[0] = ACTION_TRIGGER_START;
            } else if (match->aiState.baseRunnerKeyDown[0] == 1 && match->aiState.baseRunnerLock[0] == AI_CLICK_LOCK) {
                match->aiState.baseRunnerKeyDown[0] = 0;
                match->aiState.baseRunnerDecisionMade[0] = 1;
                match->aiState.clickBreak[0] = 0;
                match->aiState.baseRunnerLock[0] = AI_NO_LOCK;
            }
        }
        // if decide that baserunners should run, we click their keys.
        if (match->aiState.runningBaseRunners == 1) {
            int i;
            for (i = 1; i < BASE_COUNT; i++) {
                if (match->aiState.baseRunnerDecisionMade[i] == 0 &&
                    get_base_controller(match, &rules->referee, (BaseID)i) != -1 &&
                    match->playerInfo[get_base_controller(match, &rules->referee, (BaseID)i)].bTPI.state ==
                        PLAYER_STATE_ON_BASE &&
                    match->aiState.baseRunnerKeyDown[i] == 0 && match->aiState.baseRunnerLock[i] == AI_NO_LOCK &&
                    match->aiState.clickBreak[i] > CLICK_BREAK_CONSTANT) {
                    match->aiState.baseRunnerKeyDown[i] = 1;
                    match->aiState.baseRunnerLock[i] = AI_CLICK_LOCK;
                    match->aF.bTAF.base_run[i] = ACTION_TRIGGER_START;
                } else if (match->aiState.baseRunnerKeyDown[i] == 1 &&
                           match->aiState.baseRunnerLock[i] == AI_CLICK_LOCK) {
                    match->aiState.baseRunnerKeyDown[i] = 0;
                    match->aiState.baseRunnerLock[i] = AI_NO_LOCK;
                    match->aiState.baseRunnerDecisionMade[i] = 1;
                    match->aiState.clickBreak[i] = 0;
                }
            }
        }
    }
    // if ball is not home, we return players from first and second base to their bases
    else if (match->pRAI.batter_ready == 1 && match->pRAI.pitch_state != PITCH_STAGE_AIRBORNE &&
             match->gameFlowState.ballHome == 0) {
        if (match->aiState.runningBaseRunners == 1) {
            int i;
            for (i = 1; i < 3; i++) {
                if (get_base_controller(match, &rules->referee, (BaseID)i) != -1 &&
                    match->playerInfo[get_base_controller(match, &rules->referee, (BaseID)i)].bTPI.state ==
                        PLAYER_STATE_LEADING &&
                    match->aiState.baseRunnerKeyDown[i] == 0 && match->aiState.baseRunnerLock[i] == AI_NO_LOCK &&
                    match->aiState.clickBreak[i] > CLICK_BREAK_CONSTANT) {
                    match->aiState.baseRunnerKeyDown[i] = 1;
                    match->aiState.baseRunnerLock[i] = AI_COME_BACK_LOCK;
                    match->aF.bTAF.base_run[i] = ACTION_TRIGGER_START;
                } else if (match->aiState.baseRunnerKeyDown[i] == 1 &&
                           match->aiState.baseRunnerLock[i] == AI_COME_BACK_LOCK) {
                    match->aiState.baseRunnerKeyDown[i] = 0;
                    match->aiState.baseRunnerDecisionMade[i] = 0;
                    match->aiState.baseRunnerLock[i] = AI_NO_LOCK;
                    match->aiState.clickBreak[i] = 0;
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
            // batter isnt handled here
            // this code will make baserunners come back if wrong pitch is pitched
            for (i = 1; i < BASE_COUNT; i++) {
                int index = get_base_controller(match, &rules->referee, (BaseID)i);
                if (index != -1 && match->playerRuntime[index].goingForward == 1 &&
                    match->aiState.baseRunnerKeyDown[i] == 0 && match->aiState.baseRunnerLock[i] == AI_NO_LOCK &&
                    match->aiState.clickBreak[i] > CLICK_BREAK_CONSTANT) {
                    match->aiState.baseRunnerKeyDown[i] = 1;
                    match->aiState.baseRunnerLock[i] = AI_COME_BACK_WRONG_PITCH_LOCK;
                    match->aF.bTAF.base_run[i] = ACTION_TRIGGER_START;
                } else if (match->aiState.baseRunnerKeyDown[i] == 1 &&
                           match->aiState.baseRunnerLock[i] == AI_COME_BACK_WRONG_PITCH_LOCK) {
                    match->aiState.baseRunnerKeyDown[i] = 0;
                    match->aiState.baseRunnerDecisionMade[i] = 0;
                    match->aiState.baseRunnerLock[i] = AI_NO_LOCK;
                    match->aiState.clickBreak[i] = 0;
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
        checkIfBallIsOutOfBounds(&match->ballInfo, fieldPositions)) {
        isDoubleClickingOk = 1;
    }
    // we will run with everyone so we need to simulate double click here.
    for (i = 0; i < BASE_COUNT; i++) {
        int j;
        int index = get_base_controller(match, &rules->referee, (BaseID)i);
        int shouldRun = 1;
        if (i == 0 && match->pRAI.batter_can_advance == 0) continue;
        // here we check that there is no one running this same base interval.
        for (j = 0; j < PLAYERS_IN_TEAM + JOKER_COUNT; j++) {
            BaseID bid = match->playerInfo[j].bTPI.baseId;
            if (bid != BASE_NONE) {
                int baseInt = base_to_int_index(bid);

                if (baseInt == i) {
                    if (j != index) {
                        shouldRun = 0;
                    }
                }
            }
        }
        // if everything ok, initiate running.
        if (shouldRun == 1 && isDoubleClickingOk == 1 && match->aiState.baseRunnerLock[i] == AI_NO_LOCK &&
            match->aiState.baseRunnerKeyDown[i] == 0 && index != -1 && match->playerRuntime[index].goingForward != 1 &&
            match->aiState.amountOfClicks[i] == 0 && match->aiState.clickBreak[i] > CLICK_BREAK_CONSTANT) {
            match->aiState.baseRunnerKeyDown[i] = 1;
            match->aiState.baseRunnerLock[i] = AI_DOUBLE_CLICK_LOCK;
            match->aF.bTAF.base_run[i] = ACTION_TRIGGER_START;

        } else if (match->aiState.baseRunnerKeyDown[i] == 0 &&
                   match->aiState.baseRunnerLock[i] == AI_DOUBLE_CLICK_LOCK &&
                   match->aiState.clickBreak[i] > CLICK_BREAK_CONSTANT) {
            match->aiState.baseRunnerKeyDown[i] = 1;
            match->aF.bTAF.base_run[i] = ACTION_TRIGGER_START;
        } else if (match->aiState.baseRunnerKeyDown[i] == 1 &&
                   match->aiState.baseRunnerLock[i] == AI_DOUBLE_CLICK_LOCK) {
            match->aiState.baseRunnerKeyDown[i] = 0;
            if (match->aiState.amountOfClicks[i] == 1) {
                match->aiState.baseRunnerLock[i] = AI_NO_LOCK;
                match->aiState.amountOfClicks[i] = 0;
            } else {
                match->aiState.amountOfClicks[i]++;
            }
            match->aiState.clickBreak[i] = 0;
        }
    }
}
