#ifndef FOUNDATION_H
#define FOUNDATION_H

// The vocabulary every layer of the game speaks: the units of time and space, the team sizes, the bases, and
// who controls a team. Nothing here belongs to any one world — the physical World, the legal world, the
// controllers and the screen all name these — so it includes nothing.

// global constants
#define PI 3.141f
#define EPSILON 0.005f
#define UPDATE_INTERVAL 20
typedef enum {
    BASE_HOME = 0,
    BASE_FIRST = 1,
    BASE_SECOND = 2,
    BASE_THIRD = 3,
    BASE_HOME_SCORED = 4, // Explicitly distinct from starting at home
    BASE_NONE = -1
} BaseID;

// counts
#define JOKER_COUNT 3
#define PLAYERS_IN_TEAM 9
#define RANKED_FIELDERS_COUNT 5
#define CHANGE_PLAYER_COUNT 3
#define BASE_COUNT 4
#define MAX_HOMERUN_PAIRS 5

// simple struct to keep spatial information
typedef struct _Vector3D {
    float x;
    float y;
    float z;
} Vector3D;
typedef enum { CONTROL_PLAYER_1 = 0, CONTROL_PLAYER_2 = 1, CONTROL_AI = 2 } TeamControlMode;

#endif /* FOUNDATION_H */
