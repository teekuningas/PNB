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

// Action state auto-clear contracts (Bug #8 regression)
int test_interrupted_throw_clears_action_state(void);

// Run-scoring guard contracts (Bug #9 regression)
int test_wounded_runner_cannot_score_run(void);

// Catching-AI intent contracts (§4.12 — drop migration)
int test_ai_declares_and_executes_tactical_drop(void);

#endif // ALL_CONTRACTS_H
