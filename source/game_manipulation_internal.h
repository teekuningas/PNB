#ifndef GAME_MANIPULATION_INTERNAL_H
#define GAME_MANIPULATION_INTERNAL_H

extern StateInfo stateInfo;

static int closeToGround;

#define EVALUATION_CONSTANT_IN_AIR 200.0f
#define EVALUATION_CONSTANT_AFTER_HIT_ONCE 5.0f
#define CATCH_DISTANCE 2.6f

#define BALL_FINAL_POINT_APPROXIMATION_CONSTANT 45.0f 
#define BALL_DROP_TO_FINAL_POINT_APPROXIMATION_CONSTANT 20.0f
#define BALL_DROP_TARGET_CONSTANT 8.0f 
#define BALL_DROP_EVAL_CONSTANT 40.0f
#define BALL_ON_GROUND_SLOW_FACTOR 0.98f
#define BALL_SLOW_FACTOR_X 0.8f
#define BALL_SLOW_FACTOR_Z 0.8f
#define BALL_SLOW_FACTOR_Y 0.55f
#define BALL_BOUNCE_THRESHOLD 0.02f
#define PLAYER_TOO_CLOSE_TO_CATCH_LIMIT 15.0f
										

static __inline void updateBallToPlayer();
static __inline void updateBallStatus();
static __inline void checkIfBallCanBeCatched();
static __inline void playerLocationOrientationAndTargets();
static __inline void basemenReplacements();
static __inline void checkIfNearHomeLocation();
static __inline void baseRunnerMovementsOnBaseArrivals();
static __inline void moveIdlingPlayersToHomeLocation();
static __inline void rankPlayersAndMoveThem();
static __inline void updateModels();

#endif /* GAME_MANIPULATION_INTERNAL_H */