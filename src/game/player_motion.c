/*
    player_motion.c — a body's movement primitives.

    They set where a body is going and how fast; SIMULATE's body step then moves it.
*/

#include "player_motion.h"
#include "field.h"
#include "world.h"
#include "vector_math.h"

/*
    Index is index of the player in the playerInfo-array. stop_movement stops a body where it stands.
    A change of controlled fielder, a catch or a throw can leave a fielder walking toward a destination
    that no longer matters, so this is commonly used to stop those.
*/
void stop_movement(PlayerInfo* playerInfo, int index)
{
    if (index != -1) {
        if (playerInfo[index].fielding.throw_recovery_ticks == 0) {
            record_player_activity(&playerInfo[index].events, PLAYER_ACTIVITY_MOVEMENT_CANCELLED, false);
        }
        playerInfo[index].motion.following_target = 0;
        playerInfo[index].motion.moving = 0;

        playerInfo[index].motion.sync_previous_location = 1;
    }
}
// this is for batting team players
void stop_target_looking_player(PlayerMotion* motion)
{
    motion->moving = 0;
    motion->running = 0;
    motion->following_target = 0;
    motion->sync_previous_location = 1;
}

void set_orientation(PlayerBody* body, const Vector3D* ball_location)
{
    float dx = ball_location->x - body->location.x;
    float dz = ball_location->z - body->location.z;
    body->orientation.x = dx;
    body->orientation.z = dz;
}

/*
    this function puts player with index in the argument moving to some specified
    target by walking. is used for both fielders and batting team.
*/
void move_to_target(PlayerInfo* playerInfo, int index, const Vector3D* target)
{
    if (index != -1) {
        if (playerInfo[index].fielding.throw_recovery_ticks == 0) {
            float dx;
            float dz;
            float norm;
            playerInfo[index].body.target_location.x = target->x;
            playerInfo[index].body.target_location.z = target->z;
            // following_target is important flag to avoid unnecessary
            // overhead of checking whether the player has
            // arrived to target location.
            playerInfo[index].motion.following_target = 1;
            // first find the unit vector for direction and then set player's
            // velocity to be the direction vector times the walk_speed.
            dx = playerInfo[index].body.target_location.x - playerInfo[index].body.location.x;
            dz = playerInfo[index].body.target_location.z - playerInfo[index].body.location.z;

            norm = vec3_distance_xz(&playerInfo[index].body.target_location, &playerInfo[index].body.location);

            if (norm < EPSILON) norm = 1.0f;
            vec3_set_xz(&playerInfo[index].body.velocity, dx * WALK_SPEED / norm, dz * WALK_SPEED / norm);
            // if the player for some reason was running before this, set that to 0.
            // could happen for example if baserunner gets out.
            playerInfo[index].motion.running = 0;
            // and set moving to 1 so that player's location will be updated.
            playerInfo[index].motion.moving = 1;

            record_player_activity(&playerInfo[index].events, PLAYER_ACTIVITY_WALKING_TO_TARGET, true);
        }
    }
}
// so this function is called when outs happen but also when wounds happen. it just moves
// players out of the field and then to homebase.
void move_player_out(PlayerInfo* playerInfo, const FieldPositions* field_positions, int index)
{
    Vector3D target;
    // we are walking
    playerInfo[index].motion.running = 0;
    // left or right?
    if (playerInfo[index].body.location.x < 0) {
        target.x = field_positions->leftPoint.x - 5.0f;
        target.z = field_positions->leftPoint.z + 10.0f;
    } else {
        target.x = field_positions->rightPoint.x + 5.0f;
        target.z = field_positions->rightPoint.z + 10.0f;
    }
    // path point not passed yet.
    playerInfo[index].path.waypoint = 0;

    // and move to target takes care of the rest.
    move_to_target(playerInfo, index, &target);
}
// so we have the ranked fielders-array and those are players who are somewhat important in relation
// to ball's current location and velocity. so its natural that we have those players moving to catch
// the ball.
