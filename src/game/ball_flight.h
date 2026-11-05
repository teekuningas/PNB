#ifndef BALL_FLIGHT_H
#define BALL_FLIGHT_H

#include "world.h"

// A loose ball flies one step and meets the fence and the ground; ballHitGround is raised here.
void update_ball_status(MatchSession* match);
// A held ball moves with its holder, and stops when he does.
void update_ball_to_player(MatchSession* match);
// Launch the ball: visible, moving, at this velocity.
void generic_sling_ball(BallInfo* ballInfo, float x, float y, float z);

#endif /* BALL_FLIGHT_H */
