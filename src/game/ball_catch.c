/*
    ball_catch.c — a fielder meets the loose ball, part of SIMULATE.

    It runs right after the ball's flight and before any fielder moves this tick, so it sees the fielders
    where the previous tick left them: moving it after the fielders' behaviours would move every catch.
    A catch is detected and enacted at once — possession, control, the ball's state — and the frame's
    catchMade event is born here; whether it was a koppi is the referee's question, not this file's.
*/

#include "ball_catch.h"
#include "common_logic.h"
#include "vector_math.h"

#define CATCH_DISTANCE 2.6f
#define PLAYER_TOO_CLOSE_TO_CATCH_LIMIT 15.0f

// so the goal is check if ball is close enough to any of the players and if so, do all the work needed
// to move game to post-catch state.
void check_if_ball_can_be_caught(MatchSession* match, const FieldPositions* field_positions, int* play_sound_effect)
{
    int i;
    // so to go to do checking we should first confirm that it even is possible by checking that no one has ball
    // and pitch is not going on. also cant catch if it just happens to be frame of updating ranks
    // as that could make some weird things happen. try on the next frame again.
    if (match->ballInfo.holder == -1 && !match->ballInfo.pitch_in_flight && match->chase_replan == CHASE_REPLAN_NONE) {
        for (i = PLAYERS_IN_TEAM + JOKER_COUNT; i < PLAYERS_IN_TEAM * 2 + JOKER_COUNT; i++) {
            // so we confirm that this particular player is valid to catch, by checking the ball's released_by.
            // its purpose is to deny players catching balls right after throwing them.
            // but if ball has hit ground, we dont care about released_by anymore.
            // allow players to drop balls and catch if thrown close to them by themselves.
            if ((i != match->ballInfo.released_by || match->ballInfo.currentFlightHasHitGround == 1)) {
                Vector3D distance;
                float limit = CATCH_DISTANCE;
                distance.x = match->playerInfo[i].tPI.location.x - match->ballInfo.location.x;
                distance.z = match->playerInfo[i].tPI.location.z - match->ballInfo.location.z;
                distance.y = 2 * match->playerInfo[i].tPI.location.y / 5 - match->ballInfo.location.y;
                // check if ball is close enough to this particular player.
                if (vec3_is_small_enough_sphere(&distance, limit) == 1) {
                    // weird things would happen
                    int j;
                    int baseCatcherFlag = 0;
                    float p1x, p2x, p1z, p2z;
                    if (match->ballInfo.released_by != -1) {
                        p1x = match->playerInfo[i].tPI.location.x;
                        p1z = match->playerInfo[i].tPI.location.z;
                        p2x = match->playerInfo[match->ballInfo.released_by].tPI.location.x;
                        p2z = match->playerInfo[match->ballInfo.released_by].tPI.location.z;
                        if (match->ballInfo.thrown_to != BASE_NONE &&
                            (i == match->fielding_posts.baseman[match->ballInfo.thrown_to] ||
                             i == match->fielding_posts.cover[match->ballInfo.thrown_to])) {
                            baseCatcherFlag = 1;
                        }
                    }
                    // to avoid annyoing twitching we need to check that catching player is far enough
                    if (match->ballInfo.released_by == -1 || match->ballInfo.currentFlightHasHitGround == 1 ||
                        baseCatcherFlag == 1 ||
                        !vec3_is_small_enough_circle_xz(p1x - p2x, p1z - p2z, PLAYER_TOO_CLOSE_TO_CATCH_LIMIT)) {
                        // ensure that player that previously was controlled doesnt continue his key-controlled movement
                        // when key is still down when control changes. note this could be different player than the one
                        // who caught the ball.
                        if (match->catchingState.controlled_fielder != -1 &&
                            match->playerInfo[match->catchingState.controlled_fielder].cPI.moving == 1) {
                            stop_movement(match->playerInfo, match->catchingState.controlled_fielder);
                        }
                        // if throwRecoil is 1, model will be automatically set to 0 when animation ends
                        // but if its 0, we must change it from having ball to not having ball.
                        if (match->catchingState.controlled_fielder != -1 &&
                            match->playerInfo[match->catchingState.controlled_fielder].cTPI.throwRecoil == 0) {
                            match->playerInfo[match->catchingState.controlled_fielder].cPI.model =
                                PLAYER_ANIM_STAND_NO_BALL;
                        }
                        // set busyCatching to 0 for every one who was busy if ball is caught so that
                        // players are free to be moved to their home locations.
                        for (j = PLAYERS_IN_TEAM + JOKER_COUNT; j < PLAYERS_IN_TEAM * 2 + JOKER_COUNT; j++) {
                            if (match->playerInfo[j].cTPI.busyCatching == 1) {
                                match->playerInfo[j].cTPI.busyCatching = 0;
                                // also stop them so that decisions about moving to home location can be made.
                                stop_movement(match->playerInfo, j);
                            }
                        }
                        // stop the guy who caught the ball
                        stop_movement(match->playerInfo, i);
                        // (No re-trigger here any more: a controller's destination is held until it
                        // replaces it, so a fielder stopped by the catch walks on by itself next
                        // tick. That is what smooth_out_movement used to fake by turning the held
                        // key flags back on.)
                        // set the has ball model.
                        match->playerInfo[i].cPI.model = PLAYER_ANIM_STAND_WITH_BALL;

                        // Emit catch event (referee will evaluate if it's a wounding catch)
                        match->gameEvents.catchMade = 1;

                        // make sound
                        *play_sound_effect = SOUND_CATCH;
                        match->ballInfo.thrown_to = BASE_NONE;
                        match->catchingState.controlled_fielder = i;
                        match->ballInfo.holder = i;

                        // and set running flag
                        match->playerInfo[match->catchingState.controlled_fielder].cPI.running = 1;
                        // stop the ball and hide it. if player starts to move again,
                        // then ball's velocity will also be updated agian.
                        match->ballInfo.visible = 0;
                        match->ballInfo.moving = 0;
                        match->ballInfo.onGround = 0;
                        match->ballInfo.currentFlightHasHitGround = 0;
                        // and set ball's location to player's location (X, Z), but use proper held height (Y)
                        vec3_set_from_vector(&(match->ballInfo.location), &(match->playerInfo[i].tPI.location));
                        match->ballInfo.location.y = BALL_HEIGHT_WITH_PLAYER;
                    }
                }
            }
        }
    }
}
