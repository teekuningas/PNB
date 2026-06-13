#include "batting_ai_strategy.h"
#include <math.h>
#include <stdlib.h>

// Largest bat angle (each side) the AI aims for. Kept just inside the batting system's
// own clamp (PI/7 ≈ 0.449), reachable in the 0.02-per-frame steps the swing uses, so the
// randomized direction spreads evenly instead of piling up at a clamped extreme.
#define AI_MAX_BATTING_ANGLE 0.44f

BattingStrategy
calculate_batting_strategy(const HalfInningState* halfInningState, int fieldStatus, int power, int speed, int period)
{
    BattingStrategy strategy = {1, 0, 0}; // Default: normal style, no running

    // §26 Syötön tuomitseminen
    // If 2 strikes, be more conservative
    if (halfInningState->strikes >= 2) {
        strategy.style = 1; // Normal swing to ensure contact
    }
    int hasPower = (power > 2) ? 1 : 0;
    int isFast = (speed > 2) ? 1 : 0;

    if (period < 4) {
        if (halfInningState->strikes == 0) {
            strategy.style = 1;
            strategy.runBaseRunners = 0;
            strategy.runBatter = 0;
        } else if (halfInningState->strikes == 1) {
            if (fieldStatus == 0) {
                if (isFast == 0) {
                    strategy.style = 2;
                    strategy.runBaseRunners = 0;
                    strategy.runBatter = 1;
                } else {
                    strategy.style = 0;
                    strategy.runBaseRunners = 0;
                    strategy.runBatter = 1;
                }
            } else if (fieldStatus == 1) {
                if (hasPower == 1) {
                    if (isFast == 0) {
                        strategy.style = 1;
                        strategy.runBaseRunners = 1;
                        strategy.runBatter = 0;
                    } else {
                        strategy.style = 1;
                        strategy.runBaseRunners = 1;
                        strategy.runBatter = 1;
                    }
                } else {
                    if (isFast == 0) {
                        strategy.style = 2;
                        strategy.runBaseRunners = 0;
                        strategy.runBatter = 1;
                    } else {
                        strategy.style = 0;
                        strategy.runBaseRunners = 0;
                        strategy.runBatter = 1;
                    }
                }
            } else if (fieldStatus == 2) {
                if (hasPower == 1) {
                    if (isFast == 0) {
                        strategy.style = 1;
                        strategy.runBaseRunners = 1;
                        strategy.runBatter = 0;
                    } else {
                        strategy.style = 1;
                        strategy.runBaseRunners = 1;
                        strategy.runBatter = 1;
                    }
                } else {
                    if (isFast == 0) {
                        strategy.style = 0;
                        strategy.runBaseRunners = 1;
                        strategy.runBatter = 0;
                    } else {
                        strategy.style = 0;
                        strategy.runBaseRunners = 1;
                        strategy.runBatter = 1;
                    }
                }
            }
        } else if (halfInningState->strikes == 2) {
            if (fieldStatus == 0) {
                if (isFast == 0) {
                    strategy.style = 2;
                    strategy.runBaseRunners = 0;
                    strategy.runBatter = 1;
                } else {
                    strategy.style = 0;
                    strategy.runBaseRunners = 0;
                    strategy.runBatter = 1;
                }
            } else if (fieldStatus == 1) {
                strategy.style = 2;
                strategy.runBaseRunners = 0;
                strategy.runBatter = 1;
            } else if (fieldStatus == 2) {
                if (hasPower == 1) {
                    strategy.style = 1;
                    strategy.runBaseRunners = 1;
                    strategy.runBatter = 1;
                } else {
                    strategy.style = 0;
                    strategy.runBaseRunners = 1;
                    strategy.runBatter = 1;
                }
            }
        }
    } else {
        if (halfInningState->strikes == 0 || halfInningState->strikes == 1) {
            strategy.style = 1;
            strategy.runBaseRunners = 0;
            strategy.runBatter = 0;
        } else {
            if (hasPower == 1) {
                strategy.style = 1;
                strategy.runBaseRunners = 1;
                strategy.runBatter = 1;
            } else {
                strategy.style = 0;
                strategy.runBaseRunners = 1;
                strategy.runBatter = 0;
            }
        }
    }
    return strategy;
}

int should_change_batter(int fieldStatus, int power, int speed)
{
    if (fieldStatus == 0) {
        if (speed > 2) {
            return 0;
        } else {
            return 1;
        }
    } else if (fieldStatus == 2) {
        if (power > 2) {
            return 0;
        } else {
            return 1;
        }
    } else {
        return 0;
    }
}

int is_wrong_pitch(float vx, float vy, float gravity, float plate_width)
{
    float v_x_abs = fabsf(vx);
    float t = vy * 2.0f / gravity;
    float offset = v_x_abs * t;
    if (offset > plate_width / 2.0f) {
        return 1;
    }
    return 0;
}

// Decide the direction (bat angle) of an AI swing.
//
// Direction is deliberately RANDOMIZED and independent of base runners: the AI spreads
// its hits across the whole field rather than aiming relative to a lead runner. The
// returned value is in bat-angle units (0 = straight ahead, positive = left field,
// negative = right field). The batting system clamps the actual angle to its own limit,
// so AI_MAX_BATTING_ANGLE is kept just inside that reachable range for an even spread.
float calculate_ai_batting_angle(int battingStyle, int randomValue)
{
    if (battingStyle == 0) { // Bunt: short hit, random side
        float variance = ((randomValue % 500) - 250) / 1000.0f; // -0.25 .. +0.25
        float angle = (randomValue % 100 < 50) ? 0.5f : -0.5f;
        return angle + variance;
    } else if (battingStyle == 2) { // Wounding swing: extreme angle to draw a fielder
        return -1.5f;
    }
    // Normal swing: uniform random direction across the reachable field.
    float t = (randomValue % 1000) / 1000.0f; // 0 .. ~1
    return t * (2.0f * AI_MAX_BATTING_ANGLE) - AI_MAX_BATTING_ANGLE; // -MAX .. +MAX
}
