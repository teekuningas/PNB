#include "catching_ai_strategy.h"
#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

MovementKeys calculate_movement_keys(float dx, float dz) {
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

int should_ai_throw(int hasBallIndex, int catcherIndex, int catcherNearHome,
                    int replacerIndex, int replacerStage, int replacerBase, int replacerMoving,
                    int targetBase) {
    int shouldThrow = 0;

    // Check normal catcher
    if (hasBallIndex != catcherIndex) {
        if (catcherNearHome == 1) {
            shouldThrow = 1;
        }
    }

    // Check replacer
    if (hasBallIndex != replacerIndex) {
        if (replacerStage == 1 &&
            replacerBase == targetBase &&
            replacerMoving == 0) {
            shouldThrow = 1;
        }
    }

    return shouldThrow;
}

int should_ai_drop_ball(int woundingCatch, int batterStartedRunning,
                        int runner3OriginalBase, int runner3IsOnBase,
                        int runner2OriginalBase, int runner2IsOnBase,
                        int catcherHomeIndex, int hasBallIndex) {
    if (woundingCatch == 1 && batterStartedRunning == 1 &&
        runner3OriginalBase == 3 && runner3IsOnBase == 1 &&
        runner2OriginalBase == 2 && runner2IsOnBase == 1 &&
        catcherHomeIndex != hasBallIndex) {
        return 1;
    }
    return 0;
}

int determine_lead_base(const CatchingRunnerInfo* runners, int runnerCount, int randomValue) {
    int leadBase = -1;
    int i;
    for (i = 0; i < runnerCount; i++) {
        if (runners[i].isOnBase == 0 && runners[i].takingFreeWalk == 0) {
            if (runners[i].base > leadBase) {
                if (runners[i].leading == 0) {
                    leadBase = runners[i].base;
                } else {
                    if (randomValue == 0) {
                        leadBase = runners[i].base - 1;
                    }
                }
            }
        }
    }
    return leadBase;
}
