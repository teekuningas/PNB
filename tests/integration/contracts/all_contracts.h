#ifndef ALL_CONTRACTS_H
#define ALL_CONTRACTS_H

// Lifecycle contract tests
int test_clear_frame_events_completeness(void);

// Referee reaction contract tests
int test_referee_starts_wounding_on_catch(void);
int test_referee_snapshots_on_pitch_released(void);

// Foul detection contract test
int test_foul_detected_on_out_of_bounds_hit(void);

// End-of-inning run blocking contract tests
int test_no_pending_runs_during_end_of_inning(void);
int test_no_free_walk_runs_during_end_of_inning(void);

// Phase 6.5
int test_compound_foul_and_end_of_inning(void);
int test_compound_hr_pair_and_uncatchable(void);

// Event→Decision promotion contracts
int test_bat_outcome_promotion(void);

// Run-scoring guard contracts (a wounded runner must not be awarded a run)
int test_wounded_runner_cannot_score_run(void);

// Reset-recipe contracts (nothing a producer declared survives a reset)
int test_reset_clears_declared_intent(void);

// Catching-AI intent contracts (the drop migration)
int test_ai_declares_and_executes_tactical_drop(void);

// Pipeline ordering contracts (the control-stage slice — CONTROL at the frame top)
int test_control_stage_precedes_execution(void);

// Pitch slice contracts (§5 — declared phased pitch → engine-owned windup)
int test_pitch_aimed_releases_with_declared_velocity(void);
int test_pitch_unaimed_is_valesyotto(void);
int test_no_pitch_after_three_correct_pitches(void);

// Throw contracts (phased throw declaration → engine windup clock; power declared as a value)
int test_throw_windup_frames_scale_with_power(void);
int test_throw_committed_releases_sized_to_power(void);
int test_throw_initiated_then_committed_engine_times_release(void);

// Fielder-movement contracts (the destination message and the engine mover)
int test_move_target_sets_velocity_toward_the_point(void);
int test_move_target_at_the_fielder_is_a_no_op(void);
int test_move_target_stops_on_arrival(void);
int test_restating_the_move_target_changes_nothing(void);
int test_duplicate_messages_in_one_frame_equal_one(void);
int test_move_target_refused_without_a_controlled_fielder(void);
int test_reset_recipes_leave_no_fielding_state(void);

// §12 batter-offer contract — only a joker may be seated once the order has come round
int test_spent_order_offers_a_joker_while_home_is_still_owned(void);

#endif // ALL_CONTRACTS_H
