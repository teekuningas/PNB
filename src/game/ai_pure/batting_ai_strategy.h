#ifndef BATTING_AI_STRATEGY_H
#define BATTING_AI_STRATEGY_H

#include "globals.h"

typedef struct {
	int style;              // 0=bunt, 1=normal, 2=wound
	int runBaseRunners;     // 0 or 1
	int runBatter;          // 0 or 1
} BattingStrategy;

BattingStrategy calculate_batting_strategy(const GameState* gameState, int fieldStatus, int power, int speed, int period);

int should_change_batter(int fieldStatus, int power, int speed);

int is_wrong_pitch(float vx, float vy, float gravity, float plate_width);

float calculate_ai_batting_angle(int battingStyle, int leadBase, int randomValue);

#endif
