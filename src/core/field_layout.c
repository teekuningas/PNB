#include "field_layout.h"
#include "field.h"
#include <math.h>

void field_init_positions(FieldPositions* positions)
{
    // y-coordinate here is adjusted to be height of a ball when player has it, just because thats basically only way it
    // is used. some of the values are hard coded, which is fine. cannot expect to calculate all players' positions
    // relative to something.

    positions->pitch_plate.x = 0.0f;
    positions->pitch_plate.y = BALL_HEIGHT_WITH_PLAYER;
    positions->pitch_plate.z = 0.0f;

    positions->pitcher.x = 1.5f;
    positions->pitcher.y = BALL_HEIGHT_WITH_PLAYER;
    positions->pitcher.z = 0.0f;

    positions->first_base_run.x = -20.0f;
    positions->first_base_run.y = BALL_HEIGHT_WITH_PLAYER;
    positions->first_base_run.z = -23.5f;

    positions->first_base.x = positions->first_base_run.x;
    positions->first_base.y = BALL_HEIGHT_WITH_PLAYER;
    positions->first_base.z = positions->first_base_run.z + 0.5f;

    positions->second_base_run.x = +30.0f;
    positions->second_base_run.y = BALL_HEIGHT_WITH_PLAYER;
    positions->second_base_run.z = -43.8f;

    positions->second_base.x = positions->second_base_run.x;
    positions->second_base.y = BALL_HEIGHT_WITH_PLAYER;
    positions->second_base.z = positions->second_base_run.z + 1.0f;

    positions->third_base_run.x = -30.0f;
    positions->third_base_run.y = BALL_HEIGHT_WITH_PLAYER;
    positions->third_base_run.z = -43.8f;

    positions->third_base.x = positions->third_base_run.x;
    positions->third_base.y = BALL_HEIGHT_WITH_PLAYER;
    positions->third_base.z = positions->third_base_run.z + 1.0f;

    positions->left_point.x = -31.2f;
    positions->left_point.y = BALL_HEIGHT_WITH_PLAYER;
    positions->left_point.z = -33.0f;

    positions->run_left_point.x = positions->left_point.x;
    positions->run_left_point.y = BALL_HEIGHT_WITH_PLAYER;
    positions->run_left_point.z = -25.0f;

    positions->back_left_point.x = positions->left_point.x;
    positions->back_left_point.y = BALL_HEIGHT_WITH_PLAYER;
    positions->back_left_point.z = -83.8f;

    positions->right_point.x = 31.5f;
    positions->right_point.y = BALL_HEIGHT_WITH_PLAYER;
    positions->right_point.z = -32.0f;

    positions->back_right_point.x = positions->right_point.x;
    positions->back_right_point.y = BALL_HEIGHT_WITH_PLAYER;
    positions->back_right_point.z = positions->back_left_point.z;

    positions->bottom_right_catcher.x = positions->second_base.x - 21.0f;
    positions->bottom_right_catcher.y = BALL_HEIGHT_WITH_PLAYER;
    positions->bottom_right_catcher.z = positions->second_base.z + 22.0f;

    positions->middle_left_catcher.x = positions->third_base.x + 18.0f;
    positions->middle_left_catcher.y = BALL_HEIGHT_WITH_PLAYER;
    positions->middle_left_catcher.z = positions->third_base.z + 6.0f;

    positions->middle_right_catcher.x = -positions->middle_left_catcher.x - 9.0f;
    positions->middle_right_catcher.y = BALL_HEIGHT_WITH_PLAYER;
    positions->middle_right_catcher.z = positions->second_base.z - 6.0f;

    positions->back_left_catcher.x = positions->back_left_point.x + 20.0f;
    positions->back_left_catcher.y = BALL_HEIGHT_WITH_PLAYER;
    positions->back_left_catcher.z = positions->back_left_point.z + 10.0f;

    positions->back_right_catcher.x = -positions->back_left_catcher.x;
    positions->back_right_catcher.y = BALL_HEIGHT_WITH_PLAYER;
    positions->back_right_catcher.z = positions->back_left_catcher.z;

    positions->home_run_point.x = positions->pitch_plate.x - HOME_RADIUS;
    positions->home_run_point.y = BALL_HEIGHT_WITH_PLAYER;
    positions->home_run_point.z = HOME_LINE_Z;
}

Vector3D field_boundary_point_along(const Vector3D* from, float dirX, float dirZ)
{
    Vector3D out = *from;

    float norm = (float)sqrt(dirX * dirX + dirZ * dirZ);
    if (norm < EPSILON) return out;
    dirX /= norm;
    dirZ /= norm;

    const float minX = FIELD_LEFT + FENCE_OFFSET;
    const float maxX = FIELD_RIGHT - FENCE_OFFSET;
    const float minZ = FIELD_BACK + FENCE_OFFSET;
    const float maxZ = FIELD_FRONT - FENCE_OFFSET;

    // Walk to whichever wall the heading meets first. The starting bound is longer than any
    // traversal of the field, so it only survives when the heading meets no wall at all.
    float t = (maxX - minX) + (maxZ - minZ);
    if (dirX > EPSILON && (maxX - from->x) / dirX < t) t = (maxX - from->x) / dirX;
    if (dirX < -EPSILON && (minX - from->x) / dirX < t) t = (minX - from->x) / dirX;
    if (dirZ > EPSILON && (maxZ - from->z) / dirZ < t) t = (maxZ - from->z) / dirZ;
    if (dirZ < -EPSILON && (minZ - from->z) / dirZ < t) t = (minZ - from->z) / dirZ;

    // Already at (or past) the fence and still pushing into it: the destination is where you stand,
    // which the engine reads as "stop" — the same nothing the fence would have given anyway.
    if (t < 0.0f) t = 0.0f;

    out.x = from->x + dirX * t;
    out.z = from->z + dirZ * t;
    return out;
}
