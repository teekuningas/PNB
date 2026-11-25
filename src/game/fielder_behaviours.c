/*
    fielder_behaviours.c — the catching team's engine behaviours, part of SIMULATE.

    What every fielder does that no controller asked for, identical under every controller: knowing
    whether he is near his post, basemen covering for each other, idle fielders going home, and the chase
    ranking that sends the best-placed fielders after a loose ball and through which control is handed
    over. These are behaviours, never "AI": the controlled fielder's own walk is ACTUALIZE's, from the
    destination his controller declared.
*/

#include "fielder_behaviours.h"
#include <math.h>
#include "field.h"
#include "world.h"
#include "player_motion.h"
#include "vector_math.h"

static void move_ranked_to_catch(MatchSession* match);

#define EVALUATION_CONSTANT_IN_AIR 200.0f
#define EVALUATION_CONSTANT_AFTER_HIT_ONCE 5.0f
#define BALL_FINAL_POINT_APPROXIMATION_CONSTANT 45.0f
#define BALL_DROP_TO_FINAL_POINT_APPROXIMATION_CONSTANT 20.0f
#define BALL_DROP_EVAL_CONSTANT 40.0f

// when moving fielders automatically, we often use the knowledge of whether we are near home location or not
// so here we check if we thats the case
void check_if_near_home_location(MatchSession* match)
{
    int i;
    for (i = PLAYERS_IN_TEAM + JOKER_COUNT; i < PLAYERS_IN_TEAM * 2 + JOKER_COUNT; i++) {
        // we check it only for moving players to reduce overhead a bit.
        if (match->players[i].motion.moving == 1) {

            float dx = match->players[i].body.location.x - match->players[i].body.home_location.x;
            float dz = match->players[i].body.location.z - match->players[i].body.home_location.z;
            // if we are already close, we check if happen to have gone out.
            if (match->players[i].fielding.near_home == 1) {
                if (vec3_is_small_enough_circle_xz(dx, dz, DISTANCE_FROM_HOME_LOCATION_THRESHOLD) == 0) {
                    match->players[i].fielding.near_home = 0;
                }
            }
            // if are out we check if happened to wander in.
            else if (match->players[i].fielding.near_home == 0) {
                if (vec3_is_small_enough_circle_xz(dx, dz, DISTANCE_FROM_HOME_LOCATION_THRESHOLD) == 1) {
                    match->players[i].fielding.near_home = 1;
                }
            }
        }
    }
}

static int baseman_needs_cover(const MatchSession* match, int base_idx)
{
    const PlayerInfo* baseman = &match->players[match->fielding_posts.baseman[base_idx]];
    if (baseman->fielding.near_home != 0) return 0;
    if (base_idx == BASE_HOME) {
        // §25(4): during pitch_in_flight the lukkari is evading their own pitch, and stepping aside
        // inside HOME_RADIUS is still at home base — do not pull home's cover fielder out of position.
        if (match->ball.pitch_in_flight) return 0;
        float dx = baseman->body.location.x - baseman->body.home_location.x;
        float dz = baseman->body.location.z - baseman->body.home_location.z;
        return !vec3_is_small_enough_circle_xz(dx, dz, HOME_RADIUS);
    }
    return 1;
}

void basemen_replacements(MatchSession* match)
{
    int i;
    for (i = 0; i < BASE_COUNT; i++) {
        // if player is away from his home base
        if (baseman_needs_cover(match, i)) {
            // start moving replacer there instead if he is not moving already or if player does not control him
            if (match->fielding_posts.cover[i] != match->catching.controlled_fielder) {
                if (match->players[match->fielding_posts.cover[i]].fielding.replacement == REPLACEMENT_IDLE) {
                    move_to_target(
                        match->players, match->fielding_posts.cover[i],
                        &(match->players[match->fielding_posts.baseman[i]].body.home_location)
                    );

                    match->players[match->fielding_posts.cover[i]].fielding.replacement = REPLACEMENT_ACTIVE;
                    match->players[match->fielding_posts.cover[i]].fielding.replacement_base = (BaseID)i;
                    // can go replacing even if was busy catching before.
                    match->players[match->fielding_posts.cover[i]].fielding.busy_catching = 0;
                }
            }
        }
        // if player is on his base
        else if (match->players[match->fielding_posts.baseman[i]].fielding.near_home == 1) {
            // and there's someone there or going there, move him back ( if player doesnt control )
            if (match->fielding_posts.cover[i] != match->catching.controlled_fielder) {
                if (match->players[match->fielding_posts.cover[i]].fielding.replacement != REPLACEMENT_IDLE) {
                    if (match->players[match->fielding_posts.cover[i]].fielding.replacement_base == (BaseID)i) {
                        move_to_target(
                            match->players, match->fielding_posts.cover[i],
                            &(match->players[match->fielding_posts.cover[i]].body.home_location)
                        );

                        match->players[match->fielding_posts.cover[i]].fielding.replacement = REPLACEMENT_IDLE;
                        match->players[match->fielding_posts.cover[i]].fielding.replacement_base = BASE_NONE;
                    }
                }
            }
        }
    }
}

void move_idling_players_to_home_location(MatchSession* match)
{
    int i;
    // if not catching ball or replacing or in home location, move to home location.
    for (i = PLAYERS_IN_TEAM + JOKER_COUNT; i < PLAYERS_IN_TEAM * 2 + JOKER_COUNT; i++) {
        // §25(4): keep the lukkari at their evaded spot while the pitch is still in flight.
        if (i == match->fielding_posts.baseman[BASE_HOME] && match->ball.pitch_in_flight) {
            continue;
        }
        if (match->players[i].motion.moving == 0) {
            if (i != match->catching.controlled_fielder) {
                if (match->players[i].fielding.near_home == 0) {
                    if (match->players[i].fielding.replacement == REPLACEMENT_IDLE) {
                        if (match->players[i].fielding.busy_catching == 0) {
                            move_to_target(match->players, i, &(match->players[i].body.home_location));
                        }
                    }
                }
            }
        }
    }
}

void rank_players_and_move_them(MatchSession* match)
{
    // The one consumer of a chase re-plan (see ChaseReplan), and the one thing that clears it — at the end,
    // whether or not it was acted on. Ranking only means something with nobody holding the ball; a
    // request is never raised with the ball in a hand, and the catch check does not run while one is
    // pending, so the hand cannot fill between the raise and here.
    if (match->chase_replan != CHASE_REPLAN_NONE && match->ball.holder == -1) {

        int i, j;
        int rankedIndices[RANKED_FIELDERS_COUNT];
        float rankedEvals[RANKED_FIELDERS_COUNT];
        float v = match->ball.velocity.y;
        float a = -GRAVITY;
        float s = 0.0f;
        float ballDropX; // where the ball will land
        float ballDropZ;
        float evalBallX; // place that we use to calculate player rankings
        float evalBallZ;
        float dirX; // ball's velocity's direction
        float dirZ;
        float time; // used to calculate ballDrop. framecount it takes from ball to land after movement's start ( like
                    // in throwin or batting )
        float dot;
        float speed;
        float distance;
        float evaluation;
        if (match->ball.flight_has_hit_ground == 0) {
            if (match->ball.thrown_to == BASE_NONE) {
                s = 1.1f; // when batting the height is something like this
            } else {
                s = BALL_HEIGHT_WITH_PLAYER;
            }
        } else {
            s = 0;
        }
        // initialize rankedIndices and rankedEvals
        for (i = 0; i < RANKED_FIELDERS_COUNT; i++) {
            rankedIndices[i] = 0;
            rankedEvals[i] = 500.0f; // infinity
        }
        // calculate how long it takes for ball come back down from heights
        time = (float)(-v - sqrt(v * v - 2 * a * s)) / a;
        // ballDropX is the point where ball will approximately land after the air time
        ballDropX = match->ball.location.x + time * match->ball.velocity.x;
        ballDropZ = match->ball.location.z + time * match->ball.velocity.z;

        dirX = match->ball.velocity.x;
        dirZ = match->ball.velocity.z;
        speed = (float)sqrt(dirX * dirX + dirZ * dirZ);
        if (speed < EPSILON) speed = 0.1f;
        dirX = dirX / speed;
        dirZ = dirZ / speed;

        // here some approximations for situations when ball is still in air and when ball has hit the ground.
        // targetpoint is the place where we will move the catchers and what we will use for ranking the fielders.
        if (match->ball.flight_has_hit_ground == 0) {
            float finalPointXApprox = ballDropX + match->ball.velocity.x * BALL_FINAL_POINT_APPROXIMATION_CONSTANT;
            float finalPointZApprox = ballDropZ + match->ball.velocity.z * BALL_FINAL_POINT_APPROXIMATION_CONSTANT;
            float multiplier = speed / (time * 0.01f);
            if (multiplier < 0.35f)
                multiplier = 0.0f;
            else if (multiplier > 0.6f)
                multiplier = 0.6f;
            else
                multiplier = (multiplier - 0.35f) * 0.6f / 0.25f;

            match->catching.ball_target_point.x = (1 - multiplier) * ballDropX + multiplier * finalPointXApprox;
            match->catching.ball_target_point.z = (1 - multiplier) * ballDropZ + multiplier * finalPointZApprox;

            evalBallX = ballDropX + match->ball.velocity.x * BALL_DROP_EVAL_CONSTANT * 0.25f / (time * 0.01f);
            evalBallZ = ballDropZ + match->ball.velocity.z * BALL_DROP_EVAL_CONSTANT * 0.25f / (time * 0.01f);
        } else if (match->ball.flight_has_hit_ground == 1 && match->ball.on_ground == 0) {
            match->catching.ball_target_point.x =
                ballDropX + match->ball.velocity.x * BALL_DROP_TO_FINAL_POINT_APPROXIMATION_CONSTANT;
            match->catching.ball_target_point.z =
                ballDropZ + match->ball.velocity.z * BALL_DROP_TO_FINAL_POINT_APPROXIMATION_CONSTANT;

            evalBallX = match->catching.ball_target_point.x;
            evalBallZ = match->catching.ball_target_point.z;
        } else {
            match->catching.ball_target_point.x =
                ballDropX + match->ball.velocity.x * BALL_FINAL_POINT_APPROXIMATION_CONSTANT;
            match->catching.ball_target_point.z =
                ballDropZ + match->ball.velocity.z * BALL_FINAL_POINT_APPROXIMATION_CONSTANT;

            evalBallX = match->catching.ball_target_point.x;
            evalBallZ = match->catching.ball_target_point.z;
        }

        // check only catching team players
        for (i = PLAYERS_IN_TEAM + JOKER_COUNT; i < PLAYERS_IN_TEAM * 2 + JOKER_COUNT; i++) {
            float dx; // vector from ball drop to player
            float dz;
            float distance2;
            dx = match->players[i].body.location.x - evalBallX;
            dz = match->players[i].body.location.z - evalBallZ;
            distance = (float)sqrt(dx * dx + dz * dz);
            if (distance < EPSILON) distance = 1.0f;
            dx = match->players[i].body.location.x - match->ball.location.x;
            dz = match->players[i].body.location.z - match->ball.location.z;
            distance2 = (float)sqrt(dx * dx + dz * dz);
            if (distance2 < EPSILON) distance2 = 1.0f;
            dx = dx / distance2;
            dz = dz / distance2;
            dot = (dx * dirX + dz * dirZ) * (dx * dirX + dz * dirZ);
            if (match->ball.flight_has_hit_ground == 0) {
                evaluation = distance - dot * EVALUATION_CONSTANT_IN_AIR * speed * speed;
            } else if (match->ball.flight_has_hit_ground == 1 && match->ball.on_ground == 0) {
                evaluation = distance - dot * EVALUATION_CONSTANT_AFTER_HIT_ONCE;
            } else {
                evaluation = distance;
            }
            // and then we create a array of 5 of these players ranked on their evaluation number
            for (j = 0; j < RANKED_FIELDERS_COUNT; j++) {
                if (evaluation < rankedEvals[j]) {
                    int k;
                    for (k = RANKED_FIELDERS_COUNT - 1; k > j; k--) {
                        rankedEvals[k] = rankedEvals[k - 1];
                        rankedIndices[k] = rankedIndices[k - 1];
                    }
                    rankedEvals[j] = evaluation;
                    rankedIndices[j] = i;
                    break;
                }
            }
        }
        // so now we have ranked indices -array
        for (j = 0; j < RANKED_FIELDERS_COUNT; j++) {
            match->catching.chase_ranking[j] = rankedIndices[j];
        }

        // then change cursor to best choice of players if flags says so
        // set for example when ball is first bat/thrown etc or when change player key is pressed.
        if (match->chase_replan == CHASE_REPLAN_AND_HAND_OVER) {
            match->catching.ranking_cursor = 0;
            change_player(match);
        } else {
            match->catching.ranking_cursor = -1;
        }

        // move these critical players to ball's final position
        move_ranked_to_catch(match);
    }
    match->chase_replan = CHASE_REPLAN_NONE;
}

static void move_ranked_to_catch(MatchSession* match)
{
    int i;

    for (i = 0; i < RANKED_FIELDERS_COUNT; i++) {
        int index = match->catching.chase_ranking[i];
        // controlled player wont get the chance.
        if (index != match->catching.controlled_fielder &&
            match->players[index].fielding.replacement == REPLACEMENT_IDLE) {
            // if we are throwing ( towards a base ) we dont want the baseman there to start moving
            // as it would be nice that he is at the base when ball is caught if baserunner is going there.
            if (match->ball.thrown_to == BASE_NONE || (match->fielding_posts.baseman[match->ball.thrown_to] != index)) {
                int k;
                int done = 0;
                // and we have special condition not to move any basemen
                // automatically at all.
                for (k = 0; k < BASE_COUNT; k++) {
                    if (match->fielding_posts.baseman[k] == index) {
                        done = 1;
                    }
                }
                if (done == 0) {
                    // set busycatching flag, and move player towards the target point
                    // that has been specified beforehand.
                    match->players[match->catching.chase_ranking[i]].fielding.busy_catching = 1;
                    move_to_target(match->players, index, &match->catching.ball_target_point);
                }
            }
        }
    }
}

void request_chase_replan(MatchSession* match, ChaseReplan request)
{
    if (request > match->chase_replan) match->chase_replan = request;
}

void change_player(MatchSession* match)
{
    // this is called by user explicitly and sometimes after updating change_player lists — both only
    // with nobody holding the ball, so never during a windup. Not while a pitched ball is in the air.
    if (!match->ball.pitch_in_flight) {
        // player will start randomly floating after control changes to next player.
        if (match->catching.controlled_fielder != -1) {
            stop_movement(match->players, match->catching.controlled_fielder);
        }

        if (match->catching.chase_ranking[match->catching.ranking_cursor] !=
            -1) { // set control to new index from the array
            match->catching.controlled_fielder = match->catching.chase_ranking[match->catching.ranking_cursor];
            // and set him to run
            match->players[match->catching.controlled_fielder].motion.running = 1;
            // logic about coming back from replacement if controlled
            if (match->players[match->catching.controlled_fielder].fielding.replacement == REPLACEMENT_ACTIVE) {
                match->players[match->catching.controlled_fielder].fielding.replacement = REPLACEMENT_IDLE;
            }
            // same for busy_catching.
            if (match->players[match->catching.controlled_fielder].fielding.busy_catching == 1) {
                match->players[match->catching.controlled_fielder].fielding.busy_catching = 0;
            }
            // move others to catch( so that the previous one for example doesnt just stop if its near the ball )
            move_ranked_to_catch(match);
            // stop player who has the selection now
            stop_movement(match->players, match->catching.controlled_fielder);
        }
    }
}
