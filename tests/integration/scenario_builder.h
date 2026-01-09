#ifndef SCENARIO_BUILDER_H
#define SCENARIO_BUILDER_H

#include "globals.h"
#include "menu_types.h"

/**
 * @brief Full-scenario integration test helpers
 * 
 * These helpers create physically and logically consistent game states
 * and run full game progression loops, avoiding fragile manual state manipulation.
 */

typedef struct {
	StateInfo* state;
	unsigned int seed;
	MenuInfo menu;
	int currentFrame;
} ScenarioContext;

/**
 * @brief Create a new scenario with a fresh game state
 */
ScenarioContext* create_scenario(void);

/**
 * @brief Place a runner at or between bases with consistent state
 * 
 * @param ctx The scenario context
 * @param playerIndex Player index (0-23)
 * @param base The base they're associated with (have safety at)
 * @param progressToNext 0.0 = at base, 1.0 = at next base, 0.5 = halfway
 */
void place_runner_at_base(ScenarioContext* ctx, int playerIndex, BaseID base, float progressToNext);

/**
 * @brief Place the ball at a specific location (not in anyone's hands)
 */
void place_ball_at_location(ScenarioContext* ctx, Vector3D location);

/**
 * @brief Give the ball to a specific fielder
 */
void give_ball_to_fielder(ScenarioContext* ctx, int fielderIndex);

/**
 * @brief Throw ball toward a base using game's calibrated throwing mechanics
 */
void throw_ball_to_base(ScenarioContext* ctx, Vector3D fromLocation, BaseID targetBase);

/**
 * @brief Hit a fly ball (high arc) to a specific location
 * 
 * Sets the ball state to look like it came from a bat hit (woundingCatchPending=1).
 */
void hit_fly_ball_to_location(ScenarioContext* ctx, Vector3D fromLocation, Vector3D targetLocation);

/**
 * @brief Simulate game progression for N frames
 */
int simulate_frames(ScenarioContext* ctx, int maxFrames);

/**
 * @brief Trigger a player to start running to the next base
 * 
 * This taps into the game's base-running machinery (runToNextBase)
 * which sets the target location and initiates movement.
 * 
 * @param ctx The scenario context
 * @param playerIndex Player to make run
 * @param fromBase The base they're running from
 */
void trigger_player_run_to_next_base(ScenarioContext* ctx, int playerIndex, BaseID fromBase);

/**
 * @brief Trigger a player to start running to the previous base (return)
 * 
 * @param ctx The scenario context
 * @param playerIndex Player to make run
 * @param toBase The base they're returning to
 */
void trigger_player_run_to_previous_base(ScenarioContext* ctx, int playerIndex, BaseID toBase);

/**
 * @brief Setup a batter at home ready to run
 * 
 * Common pattern: fresh batter with no safety, ready to advance to first.
 * 
 * @param ctx The scenario context
 * @param playerIndex Player index to set as batter
 */
void setup_batter_at_home(ScenarioContext* ctx, int playerIndex);

/**
 * @brief Simulate frames until a condition is met or timeout
 * 
 * @param ctx The scenario context
 * @param condition Function returning 1 when goal is reached
 * @param maxFrames Maximum frames before giving up
 * @return Number of frames simulated
 */
int simulate_until(ScenarioContext* ctx, int (*condition)(ScenarioContext*), int maxFrames);

/**
 * @brief Cleanup scenario and free resources
 */
void cleanup_scenario(ScenarioContext* ctx);

#endif // SCENARIO_BUILDER_H
