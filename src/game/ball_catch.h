#ifndef BALL_CATCH_H
#define BALL_CATCH_H

#include "world.h"

// Physical reaction-time lock on a newly batted ball (ticks at 50 Hz, 18 = 0.36 s).
// Prevents off-the-bat clips and falling-bunt vacuums through the lukkari's 2.6-unit catch sphere
// without shrinking CATCH_DISTANCE, while leaving genuine pop-ups (kotipesäkoppi, ~35–75+ ticks)
// and close-infield fly balls cleanly catchable.
#define BATTED_BALL_CATCH_DELAY_TICKS 18

// If a fielder is within reach of the loose ball and may take it, he catches it; catch_made is raised here.
void check_if_ball_can_be_caught(MatchSession* match, int* play_sound_effect);

// Decrement the batted-ball reaction-time lock at the end of SIMULATE.
void advance_ball_catch_delay(BallInfo* ball);

#endif /* BALL_CATCH_H */
