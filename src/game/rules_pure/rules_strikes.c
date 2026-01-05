#include "rules_strikes.h"

// §26 Syötön tuomitseminen
/**
 * Determines if the batter should change due to strikes.
 *
 * @param gameState Current game state containing strikes.
 * @param safe_on_first_base_index Index of the player safe on first base (-1 if none).
 * @return 1 if the batter should change, 0 otherwise.
 */
int should_change_batter_on_strikes(const GameState* gameState)
{
	if(gameState->strikes >= 3) {
		return 1;
	}
	return 0;
}

PitchResult determine_pitch_result(float ball_x, float plate_width, int bat_miss)
{
	if (bat_miss) {
		return PITCH_RESULT_NONE; // Missed swing strike is handled in batting_system.c
	}

	if (ball_x < plate_width / 2.0f && ball_x > -plate_width / 2.0f) {
		return PITCH_RESULT_STRIKE;
	} else {
		return PITCH_RESULT_BALL;
	}
}
