/*
    ball_catch.c — a fielder meets the loose ball, part of SIMULATE.

    It runs right after the ball's flight and before any fielder moves this tick, so it sees the fielders
    where the previous tick left them; asked anywhere later in the step, catches would fall on other frames.
    A catch is detected and enacted at once — possession, control, the ball's state — and the frame's
    catchMade event is born here; whether it was a koppi is the referee's question, not this file's.
*/

#include "ball_catch.h"
#include "field.h"
#include "tick_outputs.h"
#include "world.h"
#include "player_motion.h"
#include "vector_math.h"

#define CATCH_DISTANCE 2.6f
#define PLAYER_TOO_CLOSE_TO_CATCH_LIMIT 15.0f

// so the goal is check if ball is close enough to any of the players and if so, do all the work needed
// to move game to post-catch state.
void check_if_ball_can_be_caught(MatchSession* match, int* play_sound_effect)
{
    int i;
    // so to go to do checking we should first confirm that it even is possible by checking that no one has ball
    // and pitch is not going on. also cant catch if it just happens to be frame of updating ranks
    // as that could make some weird things happen, or while the post-contact reaction delay is active.
    if (match->ball.holder == -1 && !match->ball.pitch_in_flight && match->chase_replan == CHASE_REPLAN_NONE &&
        match->ball.catch_delay_ticks == 0) {
        for (i = PLAYERS_IN_TEAM + JOKER_COUNT; i < PLAYERS_IN_TEAM * 2 + JOKER_COUNT; i++) {
            // so we confirm that this particular player is valid to catch, by checking the ball's released_by.
            // its purpose is to deny players catching balls right after throwing them.
            // but if ball has hit ground, we dont care about released_by anymore.
            // allow players to drop balls and catch if thrown close to them by themselves.
            if ((i != match->ball.released_by || match->ball.flight_has_hit_ground == 1)) {
                Vector3D distance;
                float limit = CATCH_DISTANCE;
                distance.x = match->players[i].body.location.x - match->ball.location.x;
                distance.z = match->players[i].body.location.z - match->ball.location.z;
                distance.y = 2 * match->players[i].body.location.y / 5 - match->ball.location.y;
                // check if ball is close enough to this particular player.
                if (vec3_is_small_enough_sphere(&distance, limit) == 1) {
                    // weird things would happen
                    int j;
                    int baseCatcherFlag = 0;
                    float p1x, p2x, p1z, p2z;
                    if (match->ball.released_by != -1) {
                        p1x = match->players[i].body.location.x;
                        p1z = match->players[i].body.location.z;
                        p2x = match->players[match->ball.released_by].body.location.x;
                        p2z = match->players[match->ball.released_by].body.location.z;
                        if (match->ball.thrown_to != BASE_NONE &&
                            (i == match->fielding_posts.baseman[match->ball.thrown_to] ||
                             i == match->fielding_posts.cover[match->ball.thrown_to])) {
                            baseCatcherFlag = 1;
                        }
                    }
                    // to avoid annyoing twitching we need to check that catching player is far enough
                    if (match->ball.released_by == -1 || match->ball.flight_has_hit_ground == 1 ||
                        baseCatcherFlag == 1 ||
                        !vec3_is_small_enough_circle_xz(p1x - p2x, p1z - p2z, PLAYER_TOO_CLOSE_TO_CATCH_LIMIT)) {
                        // ensure that player that previously was controlled doesnt continue his key-controlled movement
                        // when key is still down when control changes. note this could be different player than the one
                        // who caught the ball.
                        if (match->catching.controlled_fielder != -1 &&
                            match->players[match->catching.controlled_fielder].motion.moving == 1) {
                            stop_movement(match->players, match->catching.controlled_fielder);
                        }
                        if (match->catching.controlled_fielder != -1 &&
                            match->players[match->catching.controlled_fielder].fielding.throw_recovery_ticks == 0) {
                            record_player_activity(
                                &match->players[match->catching.controlled_fielder].events,
                                PLAYER_ACTIVITY_MOVEMENT_CANCELLED, false
                            );
                        }
                        // set busy_catching to 0 for every one who was busy if ball is caught so that
                        // players are free to be moved to their home locations.
                        for (j = PLAYERS_IN_TEAM + JOKER_COUNT; j < PLAYERS_IN_TEAM * 2 + JOKER_COUNT; j++) {
                            if (match->players[j].fielding.busy_catching == 1) {
                                match->players[j].fielding.busy_catching = 0;
                                // also stop them so that decisions about moving to home location can be made.
                                stop_movement(match->players, j);
                            }
                        }
                        // stop the guy who caught the ball
                        stop_movement(match->players, i);
                        // (No re-trigger here any more: a controller's destination is held until it
                        // replaces it, so a fielder stopped by the catch walks on by itself next
                        // tick. That is what smooth_out_movement used to fake by turning the held
                        // key flags back on.)
                        record_player_activity(&match->players[i].events, PLAYER_ACTIVITY_BALL_CAUGHT, false);

                        // Emit catch event (referee will evaluate if it's a wounding catch)
                        match->events.catch_made = 1;

                        // make sound
                        *play_sound_effect = SOUND_CATCH;
                        match->ball.thrown_to = BASE_NONE;
                        match->ball.catch_delay_ticks = 0;
                        match->catching.controlled_fielder = i;
                        match->ball.holder = i;

                        // and set running flag
                        match->players[match->catching.controlled_fielder].motion.running = 1;
                        // stop the ball and hide it. if player starts to move again,
                        // then ball's velocity will also be updated agian.
                        match->ball.visible = 0;
                        match->ball.moving = 0;
                        match->ball.on_ground = 0;
                        match->ball.flight_has_hit_ground = 0;
                        // and set ball's location to player's location (X, Z), but use proper held height (Y)
                        vec3_set_from_vector(&(match->ball.location), &(match->players[i].body.location));
                        match->ball.location.y = BALL_HEIGHT_WITH_PLAYER;
                    }
                }
            }
        }
    }
}

void advance_ball_catch_delay(BallInfo* ball)
{
    if (ball->catch_delay_ticks > 0) ball->catch_delay_ticks--;
}
