#ifndef BALL_FLIGHT_H
#define BALL_FLIGHT_H

#include "world.h"

// A loose ball flies one step and meets the fence and the ground; ball_hit_ground is raised here.
void update_ball_status(MatchSession* match);
// A held ball moves with its holder, and stops when he does.
void update_ball_to_player(MatchSession* match);

// Typed ball launches — every loose flight sets its complete BallInfo flight state in one place so a
// launch site cannot leave stale released_by, pitch_in_flight, thrown_to, or catch_delay_ticks behind.
void launch_pitched_ball(BallInfo* ball, Vector3D velocity, int pitcher);
void launch_batted_ball(BallInfo* ball, Vector3D velocity, int catch_delay_ticks);
void launch_thrown_ball(BallInfo* ball, Vector3D velocity, int thrower, BaseID target);
void launch_dropped_ball(BallInfo* ball, Vector3D velocity, int dropper);

#endif /* BALL_FLIGHT_H */
