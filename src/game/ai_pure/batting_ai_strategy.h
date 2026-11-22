#ifndef BATTING_AI_STRATEGY_H
#define BATTING_AI_STRATEGY_H

#include "rules_state.h"

typedef struct {
    int style; // 0=bunt, 1=normal, 2=wound
    int run_base_runners; // 0 or 1
    int run_batter; // 0 or 1
} BattingStrategy;

BattingStrategy
calculate_batting_strategy(const HalfInningState* halfInningState, int fieldStatus, int power, int speed, int period);

int should_change_batter(int fieldStatus, int power, int speed);

/* One candidate for the bat, as the batting controller sees it: who, and what they are worth. The
 * caller builds this list from the legal candidates (rules_pure/rules_batting_order.h) — the
 * strategy never decides who is ALLOWED to bat, only which of the allowed it prefers. */
typedef struct {
    int index;
    int power;
    int speed;
} BatterCandidate;

int choose_batter(const BatterCandidate* candidates, int count, int fieldStatus);

int is_wrong_pitch(float vx, float vy, float gravity, float plate_width);

float calculate_ai_batting_angle(int battingStyle, int randomValue);

/* The swing itself, as two declared values — the batting side's answer to decide_pitch_aim, and
 * symmetric with it in every way that matters: a pure function of the situation plus RNG draws the
 * caller supplies, producing a COMPLETE intent at once. The controller has no meter to time and no
 * gesture to assemble, so there is nothing to sequence and no lock to hold.
 *
 *   power    : [0,1]. Style selects a profile table entry: bunts stay below SWING_BUNT_MAX_POWER,
 *              normal swings stay above it in a firm driving band, and wounding swings are firm.
 *   vertical : [0,1]. Style biases relative to SWING_VERTICAL_FOCAL (downward bunts, low-liner
 *              normal swings, lofted koppi) with deliberate scatter around the style's target.
 *
 * rand_power in [0, 19), rand_vertical in [0, 201). */
typedef struct {
    float power;
    float vertical;
} SwingDecision;

SwingDecision decide_swing(int battingStyle, int rand_power, int rand_vertical);

#endif
