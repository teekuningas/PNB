#ifndef FIELD_H
#define FIELD_H

// The field and the physics on it: its dimensions, the speeds bodies move at, the radii and thresholds the
// mechanics measure by, and the hardcoded positions (FieldPositions, built in immutable_world.c). Immutable
// for the whole program.

#include "foundation.h"

#define PLATE_WIDTH 1.5f
#define GRAVITY 0.003f
#define ZERO_BATTING_ANGLE (19 * PI / 16)
#define WALK_SPEED 0.06f
#define RUN_SPEED 0.12f
#define LEAD_STEP 0.05f
#define BATTING_TEAM_RUN_FACTOR 1.1f

#define GROUND_WIDTH 40.0f
#define GROUND_LENGTH 30.0f
#define GROUND_OFFSET_X (-GROUND_WIDTH / 34)
#define GROUND_OFFSET_Z (-GROUND_LENGTH / 6)
#define FIELD_BACK (-4.5f * GROUND_LENGTH + GROUND_OFFSET_Z)
#define FIELD_FRONT (1.5f * GROUND_LENGTH + GROUND_OFFSET_Z)
#define FIELD_LEFT (-2.5f * GROUND_WIDTH + GROUND_OFFSET_X)
#define FIELD_RIGHT (2.5f * GROUND_WIDTH + GROUND_OFFSET_X)
#define FENCE_OFFSET 0.5f

#define HOME_RADIUS 6.0f
#define BASE_RADIUS 2.0f
// How close is too close to throw: a thrower already standing on the base it means to throw to has
// nothing to throw, and the engine refuses the windup. It lives here with the other distances rather
// than inside the throw because a controller walking a ball-carrier toward a base has to know where
// the engine starts refusing — walk inside this radius and the carrier is left holding the ball with
// no way to deliver it.
#define THROW_TO_BASE_DISTANCE 1.0f
#define HOME_LINE_Z -0.65f
#define BATTING_RADIUS 3.5f

#define BALL_HEIGHT_WITH_PLAYER 1.45f

#define BALL_INIT_SPEED_X 0.075f
#define BALL_INIT_SPEED_Z 0.03f
#define BALL_INIT_SPEED_Y 0.1f
#define BALL_INIT_LOCATION_X -5.0f
#define BALL_INIT_LOCATION_Y 3.0f
#define BALL_INIT_LOCATION_Z -2.0f

#define BALL_SIZE 0.1f

#define DISTANCE_FROM_HOME_LOCATION_THRESHOLD 1.75f
#define TARGET_ACHIEVED_THRESHOLD                                                                                      \
    0.25f // has to have some room here because we are calculating velocity only at the start of movement and everything
          // approximations anyway
// relevant field positions. hardcoded in the immutable_world.c
typedef struct _FieldPositions {
    Vector3D pitchPlate;
    Vector3D homeRunPoint;
    Vector3D pitcher;
    Vector3D firstBase;
    Vector3D secondBase;
    Vector3D thirdBase;
    Vector3D firstBaseRun;
    Vector3D secondBaseRun;
    Vector3D thirdBaseRun;
    Vector3D leftPoint;
    Vector3D runLeftPoint;
    Vector3D backLeftPoint;
    Vector3D backRightPoint;
    Vector3D rightPoint;
    Vector3D bottomRightCatcher;
    Vector3D middleLeftCatcher;
    Vector3D middleRightCatcher;
    Vector3D backLeftCatcher;
    Vector3D backRightCatcher;
} FieldPositions;

#endif /* FIELD_H */
