#ifndef ALL_SIMS_H
#define ALL_SIMS_H

// Headless AI-vs-AI simulation tests (the "sim" tier).
int test_ai_vs_ai_half_inning(void);
int test_ai_vs_ai_homerun(void);
int test_homerun_contest_seed_sweep(void);
int test_ai_vs_ai_determinism(void);
int test_different_seeds_produce_different_games(void);
int test_sim_hash_matches_recorded_baseline(void);
int test_ai_offense_breakdown(void);
int test_no_batter_lock_stall(void);

// The tick equation made executable: World(T) = tick(World(T-1), messages).
int test_world_snapshot_retick_is_identical(void);
int test_retick_diverges_when_engine_seed_is_not_restored(void);

// Controller symmetry, law 1 — controllers may not read (or write) frame events.
int test_ai_ignores_frame_events(void);

#endif /* ALL_SIMS_H */
