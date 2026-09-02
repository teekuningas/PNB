#include "batting_ai_strategy.h"
#include "actions_pure/swing_geometry.h"
#include <math.h>
#include <stdlib.h>

// Largest bat angle (each side) the AI aims for. The batting system doubles batter_angle into the
// launch heading (theta = -batter_angle*2), and the foul lines sit at about ±0.75 rad. So an angle
// A maps to a launch fan of ±2A: choose A so the fan fills the fair field and only the very edges
// clip foul. Normal swing 0.38 → ±0.76 rad (fills the field, rare foul). Bunts are short and were
// the main out-of-bounds culprit (the old fixed ±0.5 clamped to 0.449 → ±0.90 rad, well past the
// foul line), so the bunt fan is kept a touch tighter at 0.32 → ±0.64 rad. Both stay inside the
// system's own clamp (PI/7 ≈ 0.449), so a uniform draw stays uniform rather than piling at a clamp.
#define AI_MAX_BATTING_ANGLE 0.38f
#define AI_BUNT_BATTING_ANGLE 0.32f

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

// Which of the players allowed to bat this controller wants, given the field.
//
// This is the same preference `should_change_batter` has always expressed, said outright instead of
// walked. The old shape asked "am I happy with the one being shown?" and, when the answer was no,
// mutated engine state to be shown the next one — so the deliberation happened IN the world, took a
// frame or two per step for the click-simulation locks, and needed a cycle-detection guard plus a
// band-aid counter to stop it looping forever over a joker slot it kept skipping (bug #5). Choosing
// from a list needs none of that: there is nothing to loop.
//
// The preference order is preserved exactly. The old walk started at the engine's offer and stepped
// through the cycle, stopping at the first candidate it did not want to change away from, and
// falling back to where it started when it had seen them all. So: the first acceptable candidate in
// the caller's order, else the first candidate.
int choose_batter(const BatterCandidate* candidates, int count, int fieldStatus)
{
    if (count <= 0) return -1;
    for (int i = 0; i < count; i++) {
        if (!should_change_batter(fieldStatus, candidates[i].power, candidates[i].speed)) {
            return candidates[i].index;
        }
    }
    return candidates[0].index;
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
// Direction is deliberately RANDOMIZED and independent of base runners: the AI spreads its hits
// across the whole field rather than aiming relative to a lead runner. The returned value is in
// bat-angle units (0 = straight ahead, positive = left field, negative = right field). Both the
// normal swing AND the bunt draw a UNIFORM direction across their fan (the bunt's is a touch
// tighter, since a short hit fouls more easily) — a flat, full-field spread, not a pile at the
// extremes or a collapse to centre. (Wound style is a deliberate fixed extreme; left as-is.)
float calculate_ai_batting_angle(int battingStyle, int randomValue)
{
    float t = (randomValue % 1000) / 1000.0f; // 0 .. ~1, uniform
    if (battingStyle == 2) { // Wounding swing: extreme angle to draw a fielder
        return -1.5f;
    }
    float maxAngle = (battingStyle == 0) ? AI_BUNT_BATTING_ANGLE : AI_MAX_BATTING_ANGLE;
    return t * (2.0f * maxAngle) - maxAngle; // uniform in [-maxAngle, +maxAngle]
}

// How far the AI's declared elevation scatters either side of the sweet spot, in declared units.
// Calibrated, not guessed: the batting AI's realised scatter on the code this replaced was about
// three frames of meter travel, which is this width as a uniform draw. Widening it makes a worse
// batter and narrowing it a better one, and unlike the accident it replaces, either is a decision.
#define SWING_AI_VERTICAL_SPREAD 0.077f

// Power bands per style, in declared units. These are the levels the old meter thresholds actually
// realised, carried across so the batting side keeps the character it had: a bunt soft, a normal
// swing spread across a competent band, a wounding swing firm.
#define SWING_AI_BUNT_POWER 0.389f
#define SWING_AI_WOUND_POWER 0.722f
#define SWING_AI_NORMAL_POWER_MAX 0.917f
#define SWING_AI_NORMAL_POWER_STEP 0.0278f // one meter step, in declared units

SwingDecision decide_swing(int battingStyle, int rand_power, int rand_vertical)
{
    SwingDecision decision;

    if (battingStyle == 0) {
        decision.power = SWING_AI_BUNT_POWER;
    } else if (battingStyle == 2) {
        decision.power = SWING_AI_WOUND_POWER;
    } else {
        decision.power = SWING_AI_NORMAL_POWER_MAX - (float)rand_power * SWING_AI_NORMAL_POWER_STEP;
    }
    if (decision.power < 0.0f) decision.power = 0.0f;
    if (decision.power > 1.0f) decision.power = 1.0f;

    // Aim at the sweet spot and miss it by a little, deliberately. [0,201) -> [-1,1].
    float offset = ((float)rand_vertical - 100.0f) / 100.0f;
    decision.vertical = SWING_VERTICAL_FOCAL + offset * SWING_AI_VERTICAL_SPREAD;

    // Stay inside the meter a human would have been reading: above its top is a place no gesture
    // could reach, and the engine trusts values rather than clamping them.
    float top = swing_marker_top(decision.power);
    if (decision.vertical > top) decision.vertical = top;
    if (decision.vertical < 0.0f) decision.vertical = 0.0f;

    return decision;
}
