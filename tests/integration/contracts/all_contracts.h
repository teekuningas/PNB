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
// §27/§12/§7 — who may take the bat, refused at the INGEST gate (tests/.../test_batter_selection.c)
int test_seat_refuses_regular_when_order_spent(void);
int test_seat_refuses_a_spent_joker(void);
int test_seat_refuses_a_regular_out_of_turn(void);
int test_seat_waits_for_home_then_seats_the_named_joker(void);
int test_repeated_seat_requests_seat_once(void);
int test_seat_refuses_when_no_decision_is_open(void);
int test_declared_aim_walks_the_batter_and_arrives_exactly(void);
int test_aim_beyond_the_arc_stands_at_its_end(void);
int test_restating_the_aim_changes_nothing(void);
int test_silence_leaves_the_aim_alone(void);
int test_seat_naming_a_non_player_is_malformed(void);

// The swing slice — declared values, repeat-safety, silence, withdrawal, and the gate's refusal.
int test_swing_declared_values_drive_the_contact(void);
int test_swing_declarations_are_repeat_safe(void);
int test_a_swing_never_declared_is_not_a_miss(void);
int test_a_withdrawn_swing_does_not_reach_the_ball(void);
int test_a_swing_declared_with_no_pitch_is_refused(void);

#endif // ALL_CONTRACTS_H
