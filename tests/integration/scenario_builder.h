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
 * @brief Initialize referee state from physical world
 *
 * Must be called after placing players but BEFORE starting the test.
 * Directly calls initialize_referee() to scan physical state and
 * establish legal tracking.
 *
 * @param ctx The scenario context
 */
void initialize_referee_from_physical_state(ScenarioContext* ctx);

/**
 * @brief Emit pitchReleased event and let referee snapshot state
 *
 * Should be called AFTER placing players and initializing referee,
 * but BEFORE the actual test actions (hitting ball, running, etc).
 * This sets baseAtPitchStart for all players based on their current positions.
 *
 * @param ctx The scenario context
 */
void snapshot_pitch_start_state(ScenarioContext* ctx);

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
 * @brief Move the pitcher (Lukkari, idx 12) away from home plate
 *
 * In pesäpallo, the pitcher stands at home plate and can catch fly balls.
 * This helper moves them away to prevent interference in test scenarios.
 *
 * Should be called in almost all tests BEFORE initialize_referee_from_physical_state()
 * to ensure pitcher doesn't catch balls or tag runners.
 *
 * Only skip this if you're specifically testing pitcher/home plate defense.
 *
 * @param ctx The scenario context
 */
void move_pitcher_away(ScenarioContext* ctx);

/**
 * @brief Give the ball to the pitcher (Lukkari, idx 12) at home plate
 *
 * In pesäpallo, the pitcher stands at home plate. This helper ensures the
 * ball is physically held by the pitcher at their home location, so that
 * gameManipulation computes ballHome = 1.
 *
 * Use this when your test needs the ball to be "home" — setting
 * gameFlowState.ballHome directly won't survive a pipeline frame because
 * gameManipulation recomputes it from physics every frame.
 *
 * Do NOT combine with move_pitcher_away in the same test.
 *
 * @param ctx The scenario context
 */
void give_ball_to_pitcher(ScenarioContext* ctx);

/**
 * @brief Place the ball in the air above a location so it drops naturally
 *
 * The ball will be placed 5m above the target location with downward velocity.
 * It will fall naturally and trigger hasBallHitGround when it lands.
 * Call this AFTER initialize_referee_from_physical_state() and snapshot_pitch_start_state().
 *
 * @param ctx The scenario context
 * @param targetLocation Where the ball should land
 */
void place_ball_over_location(ScenarioContext* ctx, Vector3D targetLocation);

/**
 * @brief Throw ball toward a base using game's calibrated throwing mechanics
 */
void throw_ball_to_base(ScenarioContext* ctx, Vector3D fromLocation, BaseID targetBase);

/**
 * @brief Hit a fly ball (high arc) to a specific location
 *
 * Sets the ball state to look like it came from a bat hit (woundingCatchPending=1).
 *
 * @param ctx The scenario context
 * @param fromLocation Where the ball starts (usually pitch plate)
 * @param targetLocation Where the ball should land
 * @param flightFrames Number of frames for the ball to be in the air (default 150)
 */
void hit_fly_ball_to_location_with_time(
    ScenarioContext* ctx, Vector3D fromLocation, Vector3D targetLocation, float flightFrames
);

/**
 * @brief Hit a fly ball with default flight time (150 frames)
 */
void hit_fly_ball_to_location(ScenarioContext* ctx, Vector3D fromLocation, Vector3D targetLocation);

/**
 * @brief Simulate game progression for N frames
 */
int simulate_frames(ScenarioContext* ctx, int maxFrames);

/**
 * @brief Trigger a player to start running to the next base
 *
 * This taps into the game's base-running machinery (run_to_next_base)
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
 * @brief Simulate a pitch thrown to a specific X coordinate relative to plate center
 *
 * Sets up all necessary state (pitchState, ball velocity, referee snapshots) to mimic
 * a real pitch release.
 *
 * @param ctx The scenario context
 * @param targetX The target X coordinate (0.0 = center/strike, >0.75 = ball)
 */
void perform_pitch(ScenarioContext* ctx, float targetX);

/**
 * @brief Cleanup scenario and free resources
 */
void cleanup_scenario(ScenarioContext* ctx);

#endif // SCENARIO_BUILDER_H
