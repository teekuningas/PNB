/*
    body_step.c — every body takes its step, part of SIMULATE (the bodies' counterpart to ball_flight.c).

    Location and orientation from velocity, runners along their paths, and a runner's arrival at a base,
    which is where the frame's playerArrivedAtBase event is born — detected inside the step that makes
    it, then answered at once by the runners' own movement on arrival.
*/

#include "body_step.h"
#include "field.h"
#include "rules_state.h"
#include "world.h"
#include "player_motion.h"
#include "runner_paths.h"
#include "actions/batting_system.h"
#include "vector_math.h"
#include "base_logic.h"
#include "base_control.h"
#include "rules_pure/player_utils.h"

void player_location_orientation_and_targets(MatchSession* match, const FieldPositions* field_positions)
{
    int i;
    int activeBatterIndex = get_active_batter_index(match);

    for (i = 0; i < PLAYERS_IN_TEAM * 2 + JOKER_COUNT; i++) {
        // orientation updated only if ball or player moving
        if (match->ballInfo.moving == 1 || match->playerInfo[i].motion.moving == 1) {
            if (i != match->ballInfo.holder) {
                if (match->playerInfo[i].motion.running == 0) {
                    if (i != activeBatterIndex) {
                        set_orientation(&match->playerInfo[i].body, &match->ballInfo.location);
                    }
                }
            }
        }
    }
    // move players
    for (i = 0; i < PLAYERS_IN_TEAM * 2 + JOKER_COUNT; i++) {
        // players only moved if moving-flag is set to 1.
        if (match->playerInfo[i].motion.moving == 1) {
            vec3_set_xz(
                &match->playerInfo[i].body.previous_location, match->playerInfo[i].body.location.x,
                match->playerInfo[i].body.location.z
            );
            // if we are pressing against the fences, fence wont give up. we need to tell that to ball also.
            // this can happen only for fielders.
            if (i >= PLAYERS_IN_TEAM + JOKER_COUNT) {
                if ((match->playerInfo[i].body.location.z > FIELD_FRONT - FENCE_OFFSET &&
                     match->playerInfo[i].body.velocity.z > 0) ||
                    (match->playerInfo[i].body.location.z < FIELD_BACK + FENCE_OFFSET &&
                     match->playerInfo[i].body.velocity.z < 0)) {
                    match->ballInfo.needsMoveUpdate = 1;
                    match->playerInfo[i].body.velocity.z = 0.0f;
                }
                if ((match->playerInfo[i].body.location.x > FIELD_RIGHT - FENCE_OFFSET &&
                     match->playerInfo[i].body.velocity.x > 0) ||
                    (match->playerInfo[i].body.location.x < FIELD_LEFT + FENCE_OFFSET &&
                     match->playerInfo[i].body.velocity.x < 0)) {
                    match->playerInfo[i].body.velocity.x = 0.0f;
                    match->ballInfo.needsMoveUpdate = 1;
                }
            }
            // update the location by player's current velocity.
            vec3_add_xz(
                &match->playerInfo[i].body.location, match->playerInfo[i].body.velocity.x,
                match->playerInfo[i].body.velocity.z
            );

            // DEBUG for Wounded Players
            if (match->playerInfo[i].batting.presence.state == PLAYER_STATE_WOUNDED) {
                // printf("[DEBUG] Player %d WOUNDED Moving. Pos: (%.2f, %.2f) Vel: (%.2f, %.2f) Target: (%.2f,
                // %.2f)\n", i, match->playerInfo[i].body.location.x,
                // match->playerInfo[i].body.location.z, match->playerInfo[i].body.velocity.x,
                // match->playerInfo[i].body.velocity.z, match->playerInfo[i].body.target_location.x,
                // match->playerInfo[i].body.target_location.z);
            }

            // a lot of movement in the game is handled by these se called targets. we set target location and
            // following_target flag for a player and he will move to that direction with constant velocity until he
            // reaches the target. reaching is decided here. but first step to optimize this process is to have this
            // following_target flag so that we can distinct between user moved players and automatically moved players.
            if (match->playerInfo[i].motion.following_target == 1) {
                // so here we now check if we happened to be close enough to our target so that we could wrap this
                // journey up.
                float dx = match->playerInfo[i].body.location.x - match->playerInfo[i].body.target_location.x;
                float dz = match->playerInfo[i].body.location.z - match->playerInfo[i].body.target_location.z;
                if (vec3_is_small_enough_circle_xz(dx, dz, TARGET_ACHIEVED_THRESHOLD) == 1) {
                    int needToStop = 1;
                    if (i == activeBatterIndex && match->playerRuntime[i].goingForward == 0) {
                        prepare_batter(match);
                    } else {
                        record_player_activity(&match->playerInfo[i].events, PLAYER_ACTIVITY_TARGET_REACHED, false);
                    }
                    // if no out is made of this player ( batting team player )
                    if (i < PLAYERS_IN_TEAM + JOKER_COUNT) {
                        // so if we are moving out of the field after being wounded or tagged.
                        // here we first move to a some point outside the limits of game area
                        // and then back to a point in homebase, and then to own place on the circle.
                        // check for running because otherwise we players who were wounded before arriving a base
                        // would be stuck there standing.
                        if ((match->playerInfo[i].batting.presence.state == PLAYER_STATE_OUT) ||
                            (match->playerInfo[i].batting.presence.state == PLAYER_STATE_SCORED) ||
                            (match->playerInfo[i].batting.presence.state == PLAYER_STATE_WOUNDED &&
                             match->playerInfo[i].motion.running == 0)) {
                            Vector3D target;
                            if (match->playerRuntime[i].passedPathPoint == 0) {
                                match->playerRuntime[i].passedPathPoint = 1;
                                target.x = field_positions->pitchPlate.x;
                                target.z = field_positions->pitchPlate.z + HOME_RADIUS;
                            } else if (match->playerRuntime[i].passedPathPoint == 1) {
                                match->playerRuntime[i].passedPathPoint = 2;
                                target.x = match->playerInfo[i].body.home_location.x;
                                target.z = match->playerInfo[i].body.home_location.z;
                            } else {
                                stop_target_looking_player(&match->playerInfo[i].motion);
                                set_orientation(&match->playerInfo[i].body, &match->ballInfo.location);
                                continue;
                            }
                            // and move to target
                            move_to_target(match->playerInfo, i, &target);
                            continue;
                        }
                        // if we are legally running bases
                        else {
                            // if we are at homebase, first base or second base and not leading
                            if (base_can_advance(match->playerInfo[i].batting.presence.base_id) &&
                                match->playerInfo[i].batting.presence.state != PLAYER_STATE_LEADING) {
                                // and moving forward
                                if (match->playerRuntime[i].goingForward == 1) {
                                    // set going forward flag to 0 as we are stopped now
                                    match->playerRuntime[i].goingForward = 0;
                                    // set new base-value and isOnBase-vaule to 1.
                                    match->playerInfo[i].batting.presence.base_id =
                                        base_get_next(match->playerInfo[i].batting.presence.base_id);

                                    if (match->playerInfo[i].batting.presence.state != PLAYER_STATE_WOUNDED &&
                                        match->playerInfo[i].batting.presence.state != PLAYER_STATE_OUT) {
                                        match->playerInfo[i].batting.presence.state = PLAYER_STATE_ON_BASE;
                                    } else {
                                    }
                                    // also these are needed to do continued calculations only when needed.
                                    match->gameEvents.playerArrivedAtBase = 1;
                                    match->playerRuntime[i].arrivedToBase = 1;

                                } else {
                                    if (match->playerInfo[i].batting.presence.state != PLAYER_STATE_WOUNDED &&
                                        match->playerInfo[i].batting.presence.state != PLAYER_STATE_OUT &&
                                        match->playerInfo[i].batting.presence.state != PLAYER_STATE_AT_BAT) {
                                        match->playerInfo[i].batting.presence.state = PLAYER_STATE_ON_BASE;
                                    }
                                }
                            }
                            // so if we are on base 3
                            else if (match->playerInfo[i].batting.presence.base_id == BASE_THIRD) {
                                if (match->playerRuntime[i].goingForward == 1) {
                                    // if we are moving forward and have not passed the flag yet, change the direction
                                    if (match->playerRuntime[i].passedPathPoint == 0) {
                                        match->playerRuntime[i].passedPathPoint = 1;
                                        run_to_next_base(match, field_positions, i, BASE_THIRD);
                                        needToStop = 0;
                                    }
                                    // if have passed the flag we are at homebase, so set the base to 4
                                    // and move player to its circle.
                                    else if (match->playerRuntime[i].passedPathPoint == 1) {
                                        Vector3D target;
                                        match->playerRuntime[i].passedPathPoint = 2;

                                        match->playerInfo[i].batting.presence.base_id = BASE_HOME_SCORED;
                                        if (match->playerInfo[i].batting.presence.state != PLAYER_STATE_WOUNDED &&
                                            match->playerInfo[i].batting.presence.state != PLAYER_STATE_OUT) {
                                            match->playerInfo[i].batting.presence.state = PLAYER_STATE_ON_BASE;
                                        }
                                        match->playerRuntime[i].goingForward = 0;
                                        match->gameEvents.playerArrivedAtBase = 1;
                                        match->playerRuntime[i].arrivedToBase = 1;
                                        target.x = match->playerInfo[i].body.home_location.x;
                                        target.z = match->playerInfo[i].body.home_location.z;
                                        move_to_target(match->playerInfo, i, &target);
                                        needToStop = 0;
                                    }
                                } else {
                                    // if we are coming back and have passed the point, change direction
                                    if (match->playerRuntime[i].passedPathPoint == 1) {
                                        match->playerRuntime[i].passedPathPoint = 0;
                                        run_to_previous_base(match, field_positions, i, BASE_THIRD);
                                        needToStop = 0;
                                    } else {
                                        // otherwise we are at the third base

                                        if (match->playerInfo[i].batting.presence.state != PLAYER_STATE_WOUNDED &&
                                            match->playerInfo[i].batting.presence.state != PLAYER_STATE_OUT) {
                                            match->playerInfo[i].batting.presence.state = PLAYER_STATE_ON_BASE;
                                        }
                                        stop_target_looking_player(&match->playerInfo[i].motion);
                                        needToStop = 0;
                                    }
                                }
                            }
                        }
                    }
                    // if procedures higher didnt handle stopping themselves
                    // we stop the player here.
                    if (needToStop == 1) {
                        stop_target_looking_player(&match->playerInfo[i].motion);
                        set_orientation(&match->playerInfo[i].body, &match->ballInfo.location);
                    }
                }
            }
        }
        // if we dont move, it could be that we just stopped, so sync the previous position.
        if (match->playerInfo[i].motion.sync_previous_location == 1) {
            vec3_set_xz(
                &match->playerInfo[i].body.previous_location, match->playerInfo[i].body.location.x,
                match->playerInfo[i].body.location.z
            );
            match->playerInfo[i].motion.sync_previous_location = 0;
        }
    }
}

void base_runner_movements_on_base_arrivals(MatchSession* match, const RefereeState* referee)
{
    // so everything starts with some player arriving base, this flag is set on target checking function.
    if (match->gameEvents.playerArrivedAtBase == 1) {
        int i;
        // we check every player who is a baserunner ( or batter )
        for (i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
            int index = i;
            // check if this particular player was one who arrived. there can be many though so we cant break out after
            // this.
            if (match->playerInfo[index].batting.presence.base_id != BASE_NONE &&
                match->playerRuntime[index].arrivedToBase == 1) {

                // no need to check on this player anymore when next runner arrives and this one has stayed where he
                // was.
                match->playerRuntime[index].arrivedToBase = 0;
                // if out has been made, player is removed immediately from the game so we wouldnt be here.
                // if wound has been made, player still tries to get to next base, so we have to handle that here.
                if (match->playerInfo[index].batting.presence.state != PLAYER_STATE_OUT) {
                    int j;
                    // so after we have a valid player, we are going to check to which base he arrived
                    // and after that do the actions corresponding to base and direction.
                    // we dont have the code handling batter arriving homebase here as
                    // it didn't need much of a work, so its handled in target checking function.
                    // homeruns are checked here though, in that case base==4? so its out of this loop.
                    for (j = 1; j < BASE_COUNT; j++) {
                        if (base_to_int_index(match->playerInfo[index].batting.presence.base_id) == j) {
                            // arrived to base
                            // if we were taking a free walk, we stop it now, BUT only if we aren't wounded/out
                            if (match->playerInfo[index].batting.presence.state == PLAYER_STATE_ADVANCING_FREELY &&
                                match->playerInfo[index].batting.presence.state != PLAYER_STATE_WOUNDED &&
                                match->playerInfo[index].batting.presence.state != PLAYER_STATE_OUT) {
                                match->playerInfo[index].batting.presence.state =
                                    PLAYER_STATE_ON_BASE; // Transition out of ADVANCING_FREELY
                            }
                        }
                    }
                    // if the player arrived to a base and had it marked as 4, it means that it possibly a run.
                    if (match->playerInfo[index].batting.presence.base_id == BASE_HOME_SCORED) {
                        // if we were safe at base 3, we are not anymore.
                        if (get_base_controller(match, referee, BASE_THIRD) == index) {
                        }
                        // We rely on Referee to adjudicate the run via is_regular_run.
                        // No need for pre-checks or manual removal here; reconcile will handle it.
                    }
                }
            }
        }
        // set the flag off as now everything has been handled.
        // match->gameEvents.playerArrivedAtBase = 0; // Handled by clear_frame_events
    }
}
