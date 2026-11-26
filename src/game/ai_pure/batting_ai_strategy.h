#ifndef BATTING_AI_STRATEGY_H
#define BATTING_AI_STRATEGY_H

#include "rules_state.h"

/* Explicit batting-strategy vocabulary — shared between the pure strategy module and its thin
 * controller shell (`batting_ai.c`), so the offense's tactics are named in one place instead of
 * scattered as magic integers. */
typedef enum {
    AI_STYLE_BUNT = 0, // näpy: soft downward tap, primarily down the 1st-base line for fast leadoff
    AI_STYLE_NORMAL = 1, // lyönti: firm driving swing into seams and outfield gaps
    AI_STYLE_WOUND = 2 // koppilyönti: deliberate deep fair fly to wound a slow batter on 2 strikes
} AIBattingStyle;

typedef enum {
    AI_FIELD_EMPTY = 0, // no runners on 1st, 2nd, or 3rd
    AI_FIELD_SCORING = 1, // runner(s) on 2nd and/or 3rd, 1st empty
    AI_FIELD_FIRST_OCCUPIED = 2 // runner on 1st (possibly others as well)
} AIFieldOccupancy;

typedef struct {
    AIBattingStyle style;
    int run_base_runners; // 0 or 1
    int run_batter; // 0 or 1
} BattingStrategy;

BattingStrategy calculate_batting_strategy(
    const HalfInningState* halfInningState, AIFieldOccupancy fieldStatus, int power, int speed, int period
);

int should_change_batter(AIFieldOccupancy fieldStatus, int power, int speed);

/* One candidate for the bat, as the batting controller sees it: who, and what they are worth. The
 * caller builds this list from the legal candidates (rules_pure/rules_batting_order.h) — the
 * strategy never decides who is ALLOWED to bat, only which of the allowed it prefers. */
typedef struct {
    int index;
    int power;
    int speed;
} BatterCandidate;

int choose_batter(const BatterCandidate* candidates, int count, AIFieldOccupancy fieldStatus);

/* Exact ballistic check: will a pitch currently at `(x, y)` with velocity `(vx, vy)` land
 * outside `plate_width` under `gravity` (when reaching ground y = 0)? Used directly by the
 * batting AI to withdraw swings and recall runners on any väärä pitch. */
int is_wrong_pitch(float x, float y, float vx, float vy, float gravity, float plate_width);

/* Opportunistic advance after a fair hit has bounced (koppi-safe, foul-safe): returns 1 when the
 * loose ball has broken out of bounds (läpilyönti) or pierced deep into the fair outfield. */
int should_advance_on_loose_ball(
    BatOutcome bat_outcome, int catch_has_been_made, FoulPlayState foul_state, BaseID thrown_to, int ball_holder,
    int ball_moving, int has_ball_hit_ground, float ball_z, int ball_out_of_bounds
);

float calculate_ai_batting_angle(AIBattingStyle battingStyle, int randomValue);

/* The swing itself, as two declared values — the batting side's answer to decide_pitch_aim, and
 * symmetric with it in every way that matters: a pure function of the situation plus RNG draws the
 * caller supplies, producing a COMPLETE intent at once. The controller has no meter to time and no
 * gesture to assemble, so there is nothing to sequence and no lock to hold.
 *
 *   power    : [0,1]. Style selects a profile table entry: bunts stay below SWING_BUNT_MAX_POWER,
 *              normal swings stay above it in a firm driving band, and wounding swings are firm.
 *   vertical : [0,1]. Style biases relative to SWING_VERTICAL_FOCAL (downward bunts, low-liner
 *              normal swings, lofted koppi) with explicit variance around the style's target.
 *
 * rand_power in [0, 19), rand_vertical in [0, 201). */
typedef struct {
    float power;
    float vertical;
} SwingDecision;

SwingDecision decide_swing(AIBattingStyle battingStyle, int rand_power, int rand_vertical);

#endif
