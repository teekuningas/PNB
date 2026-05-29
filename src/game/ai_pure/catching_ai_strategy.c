#include "catching_ai_strategy.h"
#include "base_logic.h"
#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

MovementKeys calculate_movement_keys(float dx, float dz)
{
    MovementKeys keys = {0, 0, 0, 0};

    // Original logic: angle = atan2(-(tz-pz), (tx-px))
    // dx = tx - px
    // dz = tz - pz
    float angle = (float)atan2(-dz, dx);

    if (angle > 7 * PI / 8 || angle <= -7 * PI / 8) {
        keys.left = 1;
    } else if (angle <= 7 * PI / 8 && angle > 5 * PI / 8) {
        keys.left = 1;
        keys.up = 1;
    } else if (angle <= 5 * PI / 8 && angle > 3 * PI / 8) {
        keys.up = 1;
    } else if (angle <= 3 * PI / 8 && angle > PI / 8) {
        keys.up = 1;
        keys.right = 1;
    } else if (angle <= PI / 8 && angle > -PI / 8) {
        keys.right = 1;
    } else if (angle <= -PI / 8 && angle > -3 * PI / 8) {
        keys.right = 1;
        keys.down = 1;
    } else if (angle <= -3 * PI / 8 && angle > -5 * PI / 8) {
        keys.down = 1;
    } else if (angle <= -5 * PI / 8 && angle > -7 * PI / 8) {
        keys.down = 1;
        keys.left = 1;
    }

    return keys;
}

int should_ai_throw(
    const PlayerIndexInfo* playerIndices, int catcherNearHome, int replacerIndex, int replacerStage, int replacerBase,
    int replacerMoving, int targetBase
)
{
    int hasBallIndex = playerIndices->hasBallIndex;
    int catcherIndex = playerIndices->catcherOnBaseIndex[targetBase];

    int shouldThrow = 0;
    if (hasBallIndex != -1 && catcherIndex != -1) {
        // Only consider throwing if I am NOT the baseman for this base
        if (hasBallIndex != catcherIndex) {
            // Check normal catcher
            if (catcherNearHome == 1) {
                shouldThrow = 1;
            }

            // Check replacer
            if (hasBallIndex != replacerIndex) {
                if (replacerStage == 1 && replacerBase == targetBase && replacerMoving == 0) {
                    shouldThrow = 1;
                }
            }
        }
    }

    return shouldThrow;
}

int should_ai_drop_ball(
    const RefereeState* ref, const BetweenPitchState* betweenPitchState, BaseID runner3BaseAtPitchStart,
    int runner3IsOnBase, BaseID runner2BaseAtPitchStart, int runner2IsOnBase, int catcherHomeIndex, int hasBallIndex
)
{
    // Tactical drop for bases loaded (§30 taktinen pudotus):
    // Dropping the ball in a fly-ball situation can allow for a force play
    // when we want to get an OUT at home base or create a double play.
    // We only do this when 2nd and 3rd bases are occupied.
    if (ref->woundingEvaluationActive == 1) {
        if (runner3BaseAtPitchStart == BASE_THIRD && runner3IsOnBase == 1 && runner2BaseAtPitchStart == BASE_SECOND &&
            runner2IsOnBase == 1 && catcherHomeIndex == hasBallIndex) {
            return 1;
        }
    }
    return 0;
}
BaseID determine_lead_base(const CatchingRunnerInfo* runners, int runnerCount, int randomValue)
{
    BaseID leadBase = BASE_NONE;
    int i;
    for (i = 0; i < runnerCount; i++) {
        if (runners[i].isOnBase == 0 && runners[i].takingFreeWalk == 0) {
            if (base_cmp(runners[i].base, leadBase) > 0) {
                if (runners[i].leading == 0) {
                    leadBase = runners[i].base;
                } else {
                    if (randomValue == 0) {
                        leadBase = base_get_prev(runners[i].base);
                    }
                }
            }
        }
    }
    return leadBase;
}
