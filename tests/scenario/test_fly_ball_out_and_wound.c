#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_scenarios.h"
#include "referee.h"
#include "common_logic.h"
#include "base_control.h"
#include <stdio.h>
#include <math.h>

/*
  Rules for Koppi:
   1. Caught in-between bases:
       - If runner is between bases when ball is caught:
           - If they continue to next base (before ball): WOUNDED.
           - If they return to previous base: WOUNDED (if they were "in-between" at catch moment).
   2. Caught at base:
       - If runner is at original base: SAFE.
       - If runner is at next base (and left pervious base before the catch): WOUNDED.
   3. Out (Palo):
       - You said: "You can make a OUT in a koppilyönti if player is in-between the bases."
       - And: "The ball is delivered to second base before the player from first base gets there and its a OUT."
       - This implies that if the ball is caught (fly), AND then thrown to the target base before the runner arrives,
  it's an OUT.

  Scenario Reconstruction:
   1. Batter (Starts Home) -> Runs to 1st.
   2. Runner (Starts 1st) -> Runs to 2nd.
   3. Fly Ball Caught (Koppi).
   4. Ball thrown to 2nd base.
   5. Runner (1->2) in-between bases when ball is caught.
   6. Batter (Home->1) arrives at 1st base  -> WOUNDED.

  The Bug: Batter is NOT wounded.
*/

int test_full_fly_ball_out_and_wound(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* match = ctx->state->match;
    GameRulesState* rules = ctx->state->rules;

    printf("\n=== FLY BALL OUT AND WOUND TEST ===\n");
    printf("Field positions:\n");
    printf(
        "  2nd Base: (%.1f, %.1f, %.1f)\n", ctx->state->fieldPositions->secondBase.x,
        ctx->state->fieldPositions->secondBase.y, ctx->state->fieldPositions->secondBase.z
    );
    printf("  Center Field target: (0.0, 0.0, 30.0)\n");

    // 1. Setup PHYSICAL state: Runner A at 1st, Batter B at Home
    place_runner_at_base(ctx, 0, BASE_FIRST, 0.0f); // Runner A
    setup_batter_at_home(ctx, 1); // Batter B
    match->pII.batterSelectionIndex = 1;

    // 2. Initialize referee from physical state
    initialize_referee_from_physical_state(ctx);

    // 3. Emit pitchReleased to snapshot baseAtPitchStart
    snapshot_pitch_start_state(ctx);

    // 4. Position fielders
    move_pitcher_away(ctx);

    int fielderIdx = 16; // Center fielder
    Vector3D centerField = {0.0f, 0.0f, 30.0f};
    match->playerInfo[fielderIdx].tPI.location = centerField;
    match->playerInfo[fielderIdx].tPI.homeLocation = centerField;

    int secondBasemanIdx = 18;
    Vector3D secondBase = ctx->state->fieldPositions->secondBase;
    match->playerInfo[secondBasemanIdx].tPI.location = secondBase;
    match->playerInfo[secondBasemanIdx].tPI.homeLocation = secondBase;

    // 5. Trigger runners to advance
    match->pRAI.batterCanAdvance = 1;
    trigger_player_run_to_next_base(ctx, 0, BASE_FIRST); // Runner A: 1st → 2nd
    trigger_player_run_to_next_base(ctx, 1, BASE_HOME); // Batter B: Home → 1st

    // 6. Hit fly ball to center field
    Vector3D pitchPlate = ctx->state->fieldPositions->pitchPlate;
    hit_fly_ball_to_location(ctx, pitchPlate, centerField);

    printf("Setup complete: Runner A at 1st, Batter B at Home\n");
    printf("Fly ball hit to center field (0, 0, 30)\n");

    // 7. Simulation loop
    int catchFrame = -1;
    int outFrame = -1;
    int woundFrame = -1;
    int batterArrivalFrame = -1;
    int runnerArrivalFrame = -1;
    int throwStartFrame = -1;
    int ballAtSecondFrame = -1;
    int throwTriggered = 0;

    for (int frame = 0; frame < 1200; frame++) {
        // Trigger throw to 2nd base after catch
        if (catchFrame != -1 && throwTriggered == 0) {
            throwTriggered = 1;
            throwStartFrame = frame;
            throw_ball_to_base(ctx, match->playerInfo[fielderIdx].tPI.location, BASE_SECOND);
            printf("Frame %d: Throw to 2nd base triggered\n", frame);
        }

        simulate_frames(ctx, 1);

        // --- Check Events ---
        if (rules->betweenPitchState.catchHasBeenMade && catchFrame == -1) {
            catchFrame = frame;
            printf("Frame %d: CATCH by player %d\n", frame, match->pII.hasBallIndex);
        }

        if (rules->halfInningState.event == EVENT_OUT && outFrame == -1) {
            outFrame = frame;
            printf("Frame %d: OUT event (Runner A)\n", frame);
        }

        if (rules->halfInningState.event == EVENT_WOUNDED) {
            if (match->playerInfo[1].bTPI.state == PLAYER_STATE_WOUNDED && woundFrame == -1) {
                woundFrame = frame;
                printf("Frame %d: BATTER WOUNDED\n", frame);
            }
        }

        // Track arrivals
        if (match->playerInfo[1].bTPI.baseId == BASE_FIRST && batterArrivalFrame == -1) {
            batterArrivalFrame = frame;
            int safetyBase = rules->referee.battingPlayers[1].currentSafetyBase;
            printf(
                "Frame %d: Batter arrived at 1st (state=%d, safety=%d)\n", frame, match->playerInfo[1].bTPI.state,
                safetyBase
            );
        }

        if (match->playerInfo[0].bTPI.baseId == BASE_SECOND && runnerArrivalFrame == -1) {
            runnerArrivalFrame = frame;
            printf("Frame %d: Runner A arrived at 2nd (state=%d)\n", frame, match->playerInfo[0].bTPI.state);
        }

        // Track when players lose their base (BASE_NONE)
        static int batterLostBase = 0;
        static int runnerLostBase = 0;
        if (!batterLostBase && match->playerInfo[1].bTPI.baseId == BASE_NONE) {
            batterLostBase = 1;
            int safetyBase = rules->referee.battingPlayers[1].currentSafetyBase;
            printf(
                "Frame %d: Batter lost base (baseId=NONE, safety=%d, state=%d)\n", frame, safetyBase,
                match->playerInfo[1].bTPI.state
            );
        }
        if (!runnerLostBase && match->playerInfo[0].bTPI.baseId == BASE_NONE) {
            runnerLostBase = 1;
            int safetyBase = rules->referee.battingPlayers[0].currentSafetyBase;
            printf(
                "Frame %d: Runner A lost base (baseId=NONE, safety=%d, state=%d)\n", frame, safetyBase,
                match->playerInfo[0].bTPI.state
            );
        }

        // Track ball at 2nd base
        int ballAtBase = get_ball_at_base_index(ctx->state->match, ctx->state->fieldPositions);
        if (ballAtBase == BASE_SECOND && ballAtSecondFrame == -1) {
            ballAtSecondFrame = frame;
            printf("Frame %d: Ball at 2nd base\n", frame);
        }

        // Debug logging every 20 frames or on key events
        if (frame % 20 == 0 || frame == catchFrame || frame == throwStartFrame ||
            (throwStartFrame != -1 && frame < throwStartFrame + 40) ||
            (woundFrame != -1 && frame >= woundFrame && frame < woundFrame + 100 && frame % 10 == 0) ||
            (outFrame != -1 && frame >= outFrame && frame < outFrame + 100 && frame % 10 == 0)) {
            printf(
                "  F%3d: RunnerA[St=%d Bs=%d Pos=(%.1f,%.1f)] BatterB[St=%d Bs=%d Pos=(%.1f,%.1f)] Ball[AtBase=%d]\n",
                frame, match->playerInfo[0].bTPI.state, match->playerInfo[0].bTPI.baseId,
                match->playerInfo[0].tPI.location.x, match->playerInfo[0].tPI.location.z,
                match->playerInfo[1].bTPI.state, match->playerInfo[1].bTPI.baseId, match->playerInfo[1].tPI.location.x,
                match->playerInfo[1].tPI.location.z, ballAtBase
            );
        }
    }

    printf("\n=== EVENT SUMMARY ===\n");
    printf("Catch: %d\n", catchFrame);
    printf("Throw started: %d\n", throwStartFrame);
    printf("Ball at 2nd: %d\n", ballAtSecondFrame);
    printf("Runner A at 2nd: %d\n", runnerArrivalFrame);
    printf("OUT event: %d\n", outFrame);
    printf("Batter at 1st: %d\n", batterArrivalFrame);
    printf("Batter WOUNDED: %d\n", woundFrame);
    printf("\n=== FINAL STATES ===\n");
    printf(
        "Batter: state=%d, baseId=%d, safety=%d, position=(%.1f, %.1f, %.1f)\n", match->playerInfo[1].bTPI.state,
        match->playerInfo[1].bTPI.baseId, rules->referee.battingPlayers[1].currentSafetyBase,
        match->playerInfo[1].tPI.location.x, match->playerInfo[1].tPI.location.y, match->playerInfo[1].tPI.location.z
    );
    printf(
        "Runner A: state=%d, baseId=%d, safety=%d, position=(%.1f, %.1f, %.1f)\n", match->playerInfo[0].bTPI.state,
        match->playerInfo[0].bTPI.baseId, rules->referee.battingPlayers[0].currentSafetyBase,
        match->playerInfo[0].tPI.location.x, match->playerInfo[0].tPI.location.y, match->playerInfo[0].tPI.location.z
    );
    printf("\nFor reference:\n");
    printf(
        "  Home: (%.1f, %.1f, %.1f)\n", ctx->state->fieldPositions->pitchPlate.x,
        ctx->state->fieldPositions->pitchPlate.y, ctx->state->fieldPositions->pitchPlate.z
    );
    printf(
        "  1st Base: (%.1f, %.1f, %.1f)\n", ctx->state->fieldPositions->firstBase.x,
        ctx->state->fieldPositions->firstBase.y, ctx->state->fieldPositions->firstBase.z
    );
    printf(
        "  2nd Base: (%.1f, %.1f, %.1f)\n", ctx->state->fieldPositions->secondBase.x,
        ctx->state->fieldPositions->secondBase.y, ctx->state->fieldPositions->secondBase.z
    );

    printf("=====================\n\n");

    cleanup_scenario(ctx);

    ASSERT_NE(-1, catchFrame, "Ball should have been caught");
    ASSERT_NE(-1, outFrame, "Runner A should have been forced OUT");
    ASSERT_NE(-1, batterArrivalFrame, "Batter B should have arrived at 1st");

    // THE BUG ASSERTION:
    // We expect the Batter to be WOUNDED.
    ASSERT_NE(-1, woundFrame, "Batter should be WOUNDED after arriving on fly ball");

    // CRITICAL BUG: Safety should be stripped when wounded on fly ball
    // Currently the batter retains safety=1 (BASE_FIRST) even after wounding
    // Expected: safety should be -1 (BASE_NONE) like Runner A who got OUT
    int batterFinalSafety = rules->referee.battingPlayers[1].currentSafetyBase;
    ASSERT_EQ(BASE_NONE, batterFinalSafety, "Wounded batter should lose safety (bug: they keep it)");

    return TEST_PASSED;
}
