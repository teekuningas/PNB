#include "batting_ai_strategy.h"
#include <math.h>
#include <stdlib.h>

BattingStrategy calculate_batting_strategy(const GameState* gameState, int fieldStatus, int power, int speed, int period)
{
	BattingStrategy strategy = {1, 0, 0}; // Default: normal style, no running

	// §26 Syötön tuomitseminen
	// If 2 strikes, be more conservative
	if (gameState->strikes >= 2) {
		strategy.style = 1; // Normal swing to ensure contact
	}
	int hasPower = (power > 2) ? 1 : 0;
	int isFast = (speed > 2) ? 1 : 0;

	if (period < 4) {
		if (gameState->strikes == 0) {
			strategy.style = 1;
			strategy.runBaseRunners = 0;
			strategy.runBatter = 0;
		} else if (gameState->strikes == 1) {
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
		} else if (gameState->strikes == 2) {
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
		if (gameState->strikes == 0 || gameState->strikes == 1) {
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

float calculate_ai_batting_angle(int battingStyle, BaseID leadBase, int randomValue)
{
	float angle = 0.0f;
	// Generate variance between -0.1f and +0.1f
	float variance = ((randomValue % 200) - 100) / 1000.0f;

	if (battingStyle == 0) { // Bunt
		angle = (randomValue % 100 < 50) ? 0.5f : -0.5f;
		// Bunts can have slight variance too
		angle += variance * 0.5f;
	} else if (battingStyle == 1) { // Normal
		if (leadBase == BASE_THIRD) {
			angle = 0.8f; // Hit to left field
		} else if (leadBase == BASE_SECOND) {
			angle = -0.8f; // Hit to right field
		} else {
			angle = 0.0f;
		}
		angle += variance;
	} else if (battingStyle == 2) { // Wound
		angle = -1.5f; // Bad angle
	}
	return angle;
}
