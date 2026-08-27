#include "field_layout.h"
#include <math.h>

void field_init_positions(FieldPositions* positions)
{
    // y-coordinate here is adjusted to be height of a ball when player has it, just because thats basically only way it
    // is used. some of the values are hard coded, which is fine. cannot expect to calculate all players' positions
    // relative to something.

    positions->pitchPlate.x = 0.0f;
    positions->pitchPlate.y = BALL_HEIGHT_WITH_PLAYER;
    positions->pitchPlate.z = 0.0f;

    positions->pitcher.x = 1.5f;
    positions->pitcher.y = BALL_HEIGHT_WITH_PLAYER;
    positions->pitcher.z = 0.0f;

    positions->firstBaseRun.x = -20.0f;
    positions->firstBaseRun.y = BALL_HEIGHT_WITH_PLAYER;
    positions->firstBaseRun.z = -23.5f;

    positions->firstBase.x = positions->firstBaseRun.x;
    positions->firstBase.y = BALL_HEIGHT_WITH_PLAYER;
    positions->firstBase.z = positions->firstBaseRun.z + 0.5f;

    positions->secondBaseRun.x = +30.0f;
    positions->secondBaseRun.y = BALL_HEIGHT_WITH_PLAYER;
    positions->secondBaseRun.z = -43.8f;

    positions->secondBase.x = positions->secondBaseRun.x;
    positions->secondBase.y = BALL_HEIGHT_WITH_PLAYER;
    positions->secondBase.z = positions->secondBaseRun.z + 1.0f;

    positions->thirdBaseRun.x = -30.0f;
    positions->thirdBaseRun.y = BALL_HEIGHT_WITH_PLAYER;
    positions->thirdBaseRun.z = -43.8f;

    positions->thirdBase.x = positions->thirdBaseRun.x;
    positions->thirdBase.y = BALL_HEIGHT_WITH_PLAYER;
    positions->thirdBase.z = positions->thirdBaseRun.z + 1.0f;

    positions->leftPoint.x = -31.2f;
    positions->leftPoint.y = BALL_HEIGHT_WITH_PLAYER;
    positions->leftPoint.z = -33.0f;

    positions->runLeftPoint.x = positions->leftPoint.x;
    positions->runLeftPoint.y = BALL_HEIGHT_WITH_PLAYER;
    positions->runLeftPoint.z = -25.0f;

    positions->backLeftPoint.x = positions->leftPoint.x;
    positions->backLeftPoint.y = BALL_HEIGHT_WITH_PLAYER;
    positions->backLeftPoint.z = -83.8f;

    positions->rightPoint.x = 31.5f;
    positions->rightPoint.y = BALL_HEIGHT_WITH_PLAYER;
    positions->rightPoint.z = -32.0f;

    positions->backRightPoint.x = positions->rightPoint.x;
    positions->backRightPoint.y = BALL_HEIGHT_WITH_PLAYER;
    positions->backRightPoint.z = positions->backLeftPoint.z;

    positions->bottomRightCatcher.x = positions->secondBase.x - 21.0f;
    positions->bottomRightCatcher.y = BALL_HEIGHT_WITH_PLAYER;
    positions->bottomRightCatcher.z = positions->secondBase.z + 22.0f;

    positions->middleLeftCatcher.x = positions->thirdBase.x + 18.0f;
    positions->middleLeftCatcher.y = BALL_HEIGHT_WITH_PLAYER;
    positions->middleLeftCatcher.z = positions->thirdBase.z + 6.0f;

    positions->middleRightCatcher.x = -positions->middleLeftCatcher.x - 9.0f;
    positions->middleRightCatcher.y = BALL_HEIGHT_WITH_PLAYER;
    positions->middleRightCatcher.z = positions->secondBase.z - 6.0f;

    positions->backLeftCatcher.x = positions->backLeftPoint.x + 20.0f;
    positions->backLeftCatcher.y = BALL_HEIGHT_WITH_PLAYER;
    positions->backLeftCatcher.z = positions->backLeftPoint.z + 10.0f;

    positions->backRightCatcher.x = -positions->backLeftCatcher.x;
    positions->backRightCatcher.y = BALL_HEIGHT_WITH_PLAYER;
    positions->backRightCatcher.z = positions->backLeftCatcher.z;

    positions->homeRunPoint.x = positions->pitchPlate.x - HOME_RADIUS;
    positions->homeRunPoint.y = BALL_HEIGHT_WITH_PLAYER;
    positions->homeRunPoint.z = HOME_LINE_Z;
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
