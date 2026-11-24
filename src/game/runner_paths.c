/*
    runner_paths.c — a batting player runs the bases.

    Each base is a path point; a runner is sent toward the next or the previous one at his own running
    speed, or leads off the base he holds. Pure movement mechanics: the §32 permission to leave home is
    judged at the INGEST gate (ingest.c).
*/

#include "runner_paths.h"
#include "player_motion.h"
#include <math.h>
#include "field.h"
#include "world.h"
#include "vector_math.h"

static void run_to_target(PlayerInfo* playerInfo, int index, const Vector3D* target)
{
    if (index != -1) {
        float dx;
        float dz;
        float speed;
        float norm;
        // so set target location
        playerInfo[index].body.target_location.x = target->x;
        playerInfo[index].body.target_location.z = target->z;
        // looking for target yeah
        playerInfo[index].motion.following_target = 1;
        // find the direction
        dx = playerInfo[index].body.target_location.x - playerInfo[index].body.location.x;
        dz = playerInfo[index].body.target_location.z - playerInfo[index].body.location.z;

        norm = vec3_distance_xz(&playerInfo[index].body.target_location, &playerInfo[index].body.location);

        if (norm < EPSILON) norm = 1.0f;
        // set the velocity

        speed = BATTING_TEAM_RUN_FACTOR * RUN_SPEED + (RUN_SPEED / 16) * playerInfo[index].attributes.speed;
        vec3_set_xz(&playerInfo[index].body.velocity, dx * speed / norm, dz * speed / norm);
        // we are running now, ( so for example our orientation wont change now unless we stop running)
        playerInfo[index].motion.running = 1;
        // we are moving too
        playerInfo[index].motion.moving = 1;
        // orientation to our direction
        playerInfo[index].body.orientation.x = dx;
        playerInfo[index].body.orientation.z = dz;
        record_player_activity(&playerInfo[index].events, PLAYER_ACTIVITY_RUNNING_ROUTE, true);
    }
}
void run_to_next_base(MatchSession* match, const FieldPositions* field_positions, int index, BaseID base)
{
    if (index != -1) {
        Vector3D target;
        // first we select the target corresponding to base argument
        if (base == BASE_HOME) {
            target.x = field_positions->first_base_run.x;
            target.z = field_positions->first_base_run.z;
            // here as it is the batter, we'll also stop any batting to be able to run freely.
            match->at_bat.cycle = BATTING_CYCLE_CLOSED;
        } else if (base == BASE_FIRST) {
            target.x = field_positions->second_base_run.x;
            target.z = field_positions->second_base_run.z;
        } else if (base == BASE_SECOND) {
            target.x = field_positions->third_base_run.x;
            target.z = field_positions->third_base_run.z;
        } else if (base == BASE_THIRD) {
            // if we are running home, there is the "flag" point, and we must change the direction there.
            // how it matters here is that if we have already passed the flag, we must run towards homebase,
            // if not, we must run towards flag.
            if (match->players[index].path.waypoint == 0) {
                target.x = field_positions->run_left_point.x;
                target.z = field_positions->run_left_point.z;

                match->events.homeward_runs_started++;
            } else if (match->players[index].path.waypoint == 1) {
                target.x = field_positions->home_run_point.x;
                target.z = field_positions->home_run_point.z;
            } else {
                return;
            }

        } else {
            return;
        }
        // and set it so that next player has to have a will of his own to run
        match->runner_commitments.armed[base] = 0;
        // set state to running, BUT only if we aren't already WOUNDED, OUT, or ADVANCING_FREELY
        // (which are terminal/override states that must not be downgraded)
        if (match->players[index].batting.presence.state != PLAYER_STATE_WOUNDED &&
            match->players[index].batting.presence.state != PLAYER_STATE_OUT &&
            match->players[index].batting.presence.state != PLAYER_STATE_ADVANCING_FREELY) {
            match->players[index].batting.presence.state = PLAYER_STATE_RUNNING;
        }
        // and we are moving forward
        match->players[index].path.going_forward = 1;
        // and run_to_target can continue the job with index and the already set target.
        run_to_target(match->players, index, &target);
    }
}

void run_to_previous_base(MatchSession* match, const FieldPositions* field_positions, int index, BaseID base)
{
    if (index != -1) {
        Vector3D target;
        // run to previous base works similarly to run to next base.
        // starting point here is that we arent on any base, and the base variable is telling us
        // the previous base
        if (base == BASE_HOME) {
            // so when batter returns, he will go to his ready position again.
            target.x = (float)(field_positions->pitch_plate.x + cos(ZERO_BATTING_ANGLE) * BATTING_RADIUS);
            target.z = (float)(field_positions->pitch_plate.z - sin(ZERO_BATTING_ANGLE) * BATTING_RADIUS);
        } else if (base == BASE_FIRST) {
            target.x = field_positions->first_base_run.x;
            target.z = field_positions->first_base_run.z;
        } else if (base == BASE_SECOND) {
            target.x = field_positions->second_base_run.x;
            target.z = field_positions->second_base_run.z;
        } else if (base == BASE_THIRD) {
            // here we again select the target by our current location relative to flag
            if (match->players[index].path.waypoint == 0) {
                target.x = field_positions->third_base_run.x;
                target.z = field_positions->third_base_run.z;
            } else if (match->players[index].path.waypoint == 1) {
                target.x = field_positions->run_left_point.x;
                target.z = field_positions->run_left_point.z;
            } else {
                return;
            }

        } else {
            return;
        }

        // and set it so that next player has to have a will of his own to run
        match->runner_commitments.armed[base] = 0;
        // we arent going forward
        match->players[index].path.going_forward = 0;
        // set state to running, BUT only if we aren't already WOUNDED or OUT
        if (match->players[index].batting.presence.state != PLAYER_STATE_WOUNDED &&
            match->players[index].batting.presence.state != PLAYER_STATE_OUT) {
            match->players[index].batting.presence.state = PLAYER_STATE_RUNNING;
        }
        // and run_to_target can handle the rest

        run_to_target(match->players, index, &target);
    }
}

void lead_from_base(PlayerInfo* playerInfo, const FieldPositions* field_positions, int index)
{
    if (index != -1) {
        int done = 0;
        Vector3D target;
        // now to lead off we must be either on first base or second base, as it doesnt make much sense in
        // third base nor in homebase.
        if (playerInfo[index].batting.presence.base_id == BASE_FIRST) {
            // the lead target is selected by adding a small step to current location to next bases' direction
            // using first_base instead of location in the difference is because we want the step size to stay
            // same
            target.x = playerInfo[index].body.location.x +
                       LEAD_STEP * (field_positions->second_base_run.x - field_positions->first_base_run.x);
            target.z = playerInfo[index].body.location.z +
                       LEAD_STEP * (field_positions->second_base_run.z - field_positions->first_base_run.z);
            // if we go over half way, we disallow any leading, you should just run from there.
            if (playerInfo[index].body.location.x >
                field_positions->first_base_run.x +
                    0.5f * (field_positions->second_base_run.x - field_positions->first_base_run.x))
                done = 1;
        } else if (playerInfo[index].batting.presence.base_id == BASE_SECOND) {
            // same as in previous but from second to third base
            target.x = playerInfo[index].body.location.x +
                       LEAD_STEP * (field_positions->third_base_run.x - field_positions->second_base_run.x);
            target.z = playerInfo[index].body.location.z +
                       LEAD_STEP * (field_positions->third_base_run.z - field_positions->second_base_run.z);
            if (playerInfo[index].body.location.x <
                field_positions->second_base_run.x +
                    0.5f * (field_positions->third_base_run.x - field_positions->second_base_run.x))
                done = 1;
        } else {
            done = 1;
        }
        // if our
        if (done == 0) {
            // walk to our target
            move_to_target(playerInfo, index, &target);
            // now we in fact are leading
            playerInfo[index].batting.presence.state = PLAYER_STATE_LEADING;
            // but we dont set going forward flag.
            playerInfo[index].path.going_forward = 0;
        }
    }
}
