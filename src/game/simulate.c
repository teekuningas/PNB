/*
    simulate.c — SIMULATE, stage 4 of the tick: the order of the physical step.

    The order is the behaviour. Detection is not a stage of its own after the motion, because it is not
    after it: the catch is asked between the ball's step and the fielders', against the fielders where
    the previous tick left them; the ground is met inside the ball's step, an arrival inside the body's.
    Moving any of them would change which frame a catch, a bounce or an arrival happens on.
*/

#include "simulate.h"
#include "ball_flight.h"
#include "ball_catch.h"
#include "fielder_behaviours.h"
#include "body_movement.h"
#include "drawn_state.h"

void simulate(
    MatchSession* match, const FieldPositions* field_positions, const RefereeState* referee, int* play_sound_effect
)
{
    update_ball_status(match); // the loose ball flies one step, meets the fence and the ground
    check_if_ball_can_be_caught(match, field_positions, play_sound_effect); // a fielder meets the ball
    check_if_near_home_location(match);
    basemen_replacements(match);
    move_idling_players_to_home_location(match);
    rank_players_and_move_them(match); // its ranked lists are also what control changes walk through
    player_location_orientation_and_targets(match, field_positions); // every body steps; arrivals happen
    base_runner_movements_on_base_arrivals(match, referee); // answered the frame they happen
    update_ball_to_player(match); // the held ball follows its holder's new step
    update_home_run_camera(match, field_positions, referee);
    update_models(match);
}
