/*
    player_motion.c — a body's movement primitives.

    They set where a body is going and how fast, and the pose it takes while it goes; SIMULATE's body step
    then moves it. That they also choose a mesh and an animation is the drawn state the World still
    stores (drawn_state.c), not a second job.
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
        // and after stopping movement, also ensure that no animation stays.
        if (playerInfo[index].cTPI.throw_recovery_ticks == 0) {
            playerInfo[index].cPI.model = PLAYER_ANIM_STAND_NO_BALL;
        }
        playerInfo[index].cPI.looksForTarget = 0;
        playerInfo[index].cPI.moving = 0;

        playerInfo[index].cPI.lastLastLocationUpdate = 1;
    }
}
// this is for batting team players
void stop_target_looking_player(PlayerInfo* playerInfo, int index)
{
    playerInfo[index].cPI.moving = 0;
    playerInfo[index].cPI.running = 0;
    playerInfo[index].cPI.looksForTarget = 0;
    playerInfo[index].cPI.lastLastLocationUpdate = 1;
}

void set_orientation(PlayerInfo* playerInfo, const BallInfo* ballInfo, int i)
{
    // simply set player to orient towards the ball
    if (i != -1) {
        float dx = ballInfo->location.x - playerInfo[i].tPI.location.x;
        float dz = ballInfo->location.z - playerInfo[i].tPI.location.z;
        playerInfo[i].tPI.orientation.x = dx;
        playerInfo[i].tPI.orientation.z = dz;
    }
}

/*
    this function puts player with index in the argument moving to some specified
    target by walking. is used for both fielders and batting team.
*/
void move_to_target(PlayerInfo* playerInfo, int index, const Vector3D* target)
{
    if (index != -1) {
        if (playerInfo[index].cTPI.throw_recovery_ticks == 0) {
            float dx;
            float dz;
            float norm;
            playerInfo[index].tPI.targetLocation.x = target->x;
            playerInfo[index].tPI.targetLocation.z = target->z;
            // looksForTarget is important flag to avoid unnecessary
            // overhead of checking whether the player has
            // arrived to target location.
            playerInfo[index].cPI.looksForTarget = 1;
            // first find the unit vector for direction and then set player's
            // velocity to be the direction vector times the walk_speed.
            dx = playerInfo[index].tPI.targetLocation.x - playerInfo[index].tPI.location.x;
            dz = playerInfo[index].tPI.targetLocation.z - playerInfo[index].tPI.location.z;

            norm = vec3_distance_xz(&playerInfo[index].tPI.targetLocation, &playerInfo[index].tPI.location);

            if (norm < EPSILON) norm = 1.0f;
            vec3_set_xz(&playerInfo[index].tPI.velocity, dx * WALK_SPEED / norm, dz * WALK_SPEED / norm);
            // if the player for some reason was running before this, set that to 0.
            // could happen for example if baserunner gets out.
            playerInfo[index].cPI.running = 0;
            // and set moving to 1 so that player's location will be updated.
            playerInfo[index].cPI.moving = 1;

            // choose different walking animation for fielders and batting team.
            if (index < PLAYERS_IN_TEAM + JOKER_COUNT) {
                playerInfo[index].cPI.model = PLAYER_ANIM_WALK_BARE;
            } else {
                playerInfo[index].cPI.model = PLAYER_ANIM_WALK_NO_BALL;
            }
            playerInfo[index].cPI.animationStage = 0;
            playerInfo[index].cPI.animationStageCount = 16;
            playerInfo[index].cPI.animationFrequency = 3;
        }
    }
}
// so this function is called when outs happen but also when wounds happen. it just moves
// players out of the field and then to homebase.
void move_player_out(
    PlayerInfo* playerInfo, PlayerRuntimeState* playerRuntime, const FieldPositions* field_positions, int index
)
{
    Vector3D target;
    // we are walking
    playerInfo[index].cPI.running = 0;
    // left or right?
    if (playerInfo[index].tPI.location.x < 0) {
        target.x = field_positions->leftPoint.x - 5.0f;
        target.z = field_positions->leftPoint.z + 10.0f;
    } else {
        target.x = field_positions->rightPoint.x + 5.0f;
        target.z = field_positions->rightPoint.z + 10.0f;
    }
    // path point not passed yet.
    playerRuntime[index].passedPathPoint = 0;

    // and move to target takes care of the rest.
    move_to_target(playerInfo, index, &target);
}
// so we have the ranked fielders-array and those are players who are somewhat important in relation
// to ball's current location and velocity. so its natural that we have those players moving to catch
// the ball.
