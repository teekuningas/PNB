/*
    ball_flight.c — the ball's own motion, part of SIMULATE.

    A loose ball flies: velocity, the fence, gravity, and its contact with the ground, which is where the
    frame's ball_hit_ground event is born — detected inside the integration step that produces it, because
    a bounce changes the velocity the next step reads. A held ball is carried by its holder. And an
    actualizer that launches the ball (a pitch, a throw, a hit) does it through generic_sling_ball.
*/

#include "ball_flight.h"
#include "field.h"
#include "world.h"
#include "fielder_behaviours.h"
#include "ball_physics.h"
#include "collision.h"
#include "vector_math.h"

#define BALL_ON_GROUND_SLOW_FACTOR 0.98f
#define BALL_SLOW_FACTOR_X 0.8f
#define BALL_SLOW_FACTOR_Z 0.8f
#define BALL_SLOW_FACTOR_Y 0.55f
#define BALL_BOUNCE_THRESHOLD 0.02f

void update_ball_status(MatchSession* match)
{
    if (match->ball.sync_previous_location == 1) {
        vec3_set_from_vector(&(match->ball.previous_location), &(match->ball.location));
        match->ball.sync_previous_location = 0;
    }
    // check first if ball's moving flag is 1, so that no overhead of doing updates on nothing.
    if (match->ball.moving == 1) {
        // update previous_location and location.
        vec3_set_from_vector(&(match->ball.previous_location), &(match->ball.location));
        physics_apply_velocity(&(match->ball.location), &(match->ball.velocity));
        if (match->ball.holder == -1) {
            // if ball is free then we make sure that it stays within the play area.
            if (physics_resolve_field_boundaries(
                    &(match->ball.location), &(match->ball.velocity), FIELD_FRONT, FIELD_BACK, FIELD_LEFT, FIELD_RIGHT,
                    BALL_SLOW_FACTOR_Y
                )) {
                // if ball hit a wall, we need to refresh AI targets because the prediction is now wrong
                request_chase_replan(match, CHASE_REPLAN);
            }
            if (match->ball.on_ground == 0) {
                // if ball is not on the ground yet, let it be affected by gravity.
                physics_apply_gravity(&(match->ball.velocity), 1.0f);
                // if ball now comes close to the ground
                if (physics_check_ground_collision(match->ball.location.y, BALL_SIZE / 2)) {
                    // it could be its first time
                    if (match->ball.flight_has_hit_ground ==
                        0) { // to avoid situation where it feels like player changing key doesnt work
                        // because if this wasnt here, rankedindicesarrayindex would be always initialized to 0 with
                        // every change.
                        match->ball.flight_has_hit_ground = 1;
                        match->events.ball_hit_ground = 1;
                        // this is used to track if ball has been dropped after a catch to avoid wounding
                        match->ball.hit_ground_to_unwound = 1;
                    }
                    // Strike/Ball counting logic moved to Referee_Update (referee.c)
                    // Pitch state reset moved to consolidation (game_consolidation.c)

                    // if ball has enough y-velocity it will bounce
                    if (match->ball.velocity.y < -BALL_BOUNCE_THRESHOLD) {
                        // so try to update ranks of fielders
                        request_chase_replan(match, CHASE_REPLAN);
                        // change the direction of y-velocity
                        match->ball.velocity.y = -match->ball.velocity.y;
                        vec3_set_xyz(
                            &(match->ball.velocity), BALL_SLOW_FACTOR_X * match->ball.velocity.x,
                            BALL_SLOW_FACTOR_Y * match->ball.velocity.y, BALL_SLOW_FACTOR_Z * match->ball.velocity.z
                        );

                    }
                    // if instead we dont have enough y-velocity to bounce, we just set velocity to 0, location to
                    // ground and change ranked indices for the last time.
                    else if (match->ball.velocity.y < 0) {
                        match->ball.on_ground = 1;
                        request_chase_replan(match, CHASE_REPLAN);
                        match->ball.location.y = BALL_SIZE / 2;
                        match->ball.velocity.y = 0.0f;
                    }
                }

            } else {
                // if we are rolling on the ground already, slow down the velocity a bit and stop ball if x and z
                // -velocities are small enough
                vec3_set_xz(
                    &(match->ball.velocity), BALL_ON_GROUND_SLOW_FACTOR * match->ball.velocity.x,
                    BALL_ON_GROUND_SLOW_FACTOR * match->ball.velocity.z
                );
                if (vec3_is_small_enough_circle_xz_v(&(match->ball.velocity), 0.01f)) {
                    match->ball.moving = 0;
                    match->ball.sync_previous_location = 1;
                    vec3_set_xyz(&(match->ball.velocity), 0.0f, 0.0f, 0.0f);
                }
            }
        }
    }
}

void update_ball_to_player(MatchSession* match)
{
    if (match->ball.holder != -1) {
        // if someone has ball and that someone is moving and ball movement needs updating, give it same v as the player
        // has
        if (match->players[match->ball.holder].motion.moving == 1) {
            if (match->ball.needs_move_update == 1) {
                match->ball.moving = 1;
                match->ball.needs_move_update = 0;
                vec3_set_xyz(
                    &(match->ball.velocity), match->players[match->ball.holder].body.velocity.x, 0.0f,
                    match->players[match->ball.holder].body.velocity.z
                );
            }
        }
        // if someone has ball and that someone doesn't move but ball still moves, stop the ball
        else {
            if (match->ball.moving == 1) {
                match->ball.moving = 0;
                match->ball.sync_previous_location = 1;
                vec3_set_xyz(&(match->ball.velocity), 0.0f, 0.0f, 0.0f);
            }
        }
    }
}

void generic_sling_ball(BallInfo* ballInfo, float x, float y, float z)
{
    // Make ball visible and moving
    ballInfo->visible = 1;
    ballInfo->moving = 1;

    // Set the velocity
    vec3_set_xyz(&(ballInfo->velocity), x, y, z);
}
