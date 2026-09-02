#ifndef BATTING_AI_STRATEGY_H
#define BATTING_AI_STRATEGY_H

#include "globals.h"

typedef struct {
    int style; // 0=bunt, 1=normal, 2=wound
    int runBaseRunners; // 0 or 1
    int runBatter; // 0 or 1
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
 *   power    : [0,1]. Which style is being played decides the band — a bunt is soft by definition, a
 *              normal swing varies across a competent mid-to-strong range so at-bats differ, and the
 *              wounding swing is firm.
 *   vertical : [0,1], scattered about SWING_VERTICAL_FOCAL. The scatter IS the difficulty: the AI
 *              used to inherit its timing error by accident, from releasing at a fixed meter level
 *              while the sweet spot moved with power. Making it deliberate is what turns "how good is
 *              the batter" into a number that can be tuned, and eventually a difficulty setting.
 *
 * rand_power in [0, 19), rand_vertical in [0, 201). */
typedef struct {
    float power;
    float vertical;
} SwingDecision;

SwingDecision decide_swing(int battingStyle, int rand_power, int rand_vertical);

#endif
