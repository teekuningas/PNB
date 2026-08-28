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

#endif
