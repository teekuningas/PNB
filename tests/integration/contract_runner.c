#include "test_helpers.h"
#include "fixtures.h"
#include "contracts/all_contracts.h"

#include <string.h>

int tests_run = 0;
int tests_failed = 0;

static void run_contract_tests(void)
{
    printf("Running Contract Tests (1-Frame Pipeline Proofs)...\n");

    // Lifecycle contracts
    RUN_TEST(test_clear_frame_events_completeness);

    // Referee reaction contracts
    RUN_TEST(test_referee_starts_wounding_on_catch);
    RUN_TEST(test_referee_snapshots_on_pitch_released);

    // Foul detection contracts
    RUN_TEST(test_foul_detected_on_out_of_bounds_hit);

    // End-of-inning run blocking contracts
    RUN_TEST(test_no_pending_runs_during_end_of_inning);
    RUN_TEST(test_no_free_walk_runs_during_end_of_inning);

    // Phase 6.5 Compound Resets
    RUN_TEST(test_compound_foul_and_end_of_inning);
    RUN_TEST(test_compound_hr_pair_and_uncatchable);

    // Event→Decision promotion contracts
    RUN_TEST(test_bat_outcome_promotion);

    // Action state auto-clear contracts (Bug #8 regression)

    // Run-scoring guard contracts (Bug #9 regression)
    RUN_TEST(test_wounded_runner_cannot_score_run);

    // Catching-AI intent contracts (the drop migration)
    RUN_TEST(test_reset_clears_declared_intent);
    RUN_TEST(test_ai_declares_and_executes_tactical_drop);

    // Pipeline ordering contracts (the control-stage slice — CONTROL at the frame top)
    RUN_TEST(test_control_stage_precedes_execution);

    // Pitch slice contracts (§5 — declared phased pitch → engine-owned windup)
    RUN_TEST(test_pitch_aimed_releases_with_declared_velocity);
    RUN_TEST(test_pitch_unaimed_is_valesyotto);
    RUN_TEST(test_no_pitch_after_three_correct_pitches);

    // Throw contracts (phased throw declaration → engine windup clock; power declared as a value)
    RUN_TEST(test_throw_windup_frames_scale_with_power);
    RUN_TEST(test_throw_committed_releases_sized_to_power);
    RUN_TEST(test_throw_initiated_then_committed_engine_times_release);

    printf("\n--- Fielder movement (the destination message and the engine mover) ---\n");
    RUN_TEST(test_move_target_sets_velocity_toward_the_point);
    RUN_TEST(test_move_target_at_the_fielder_is_a_no_op);
    RUN_TEST(test_move_target_stops_on_arrival);
    RUN_TEST(test_restating_the_move_target_changes_nothing);
    RUN_TEST(test_duplicate_messages_in_one_frame_equal_one);
    RUN_TEST(test_move_target_refused_without_a_controlled_fielder);
    RUN_TEST(test_reset_recipes_leave_no_fielding_state);
    RUN_TEST(test_spent_order_offers_a_joker_while_home_is_still_owned);
}

int main(int argc, char* argv[])
{
    printf("========================================\n");
    printf("PNB Contract Test Suite\n");
    printf("========================================\n\n");

    run_contract_tests();

    printf("\n========================================\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests failed: %d\n", tests_failed);
    printf("========================================\n");

    return tests_failed;
}
