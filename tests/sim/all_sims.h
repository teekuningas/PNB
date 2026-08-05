#ifndef ALL_SIMS_H
#define ALL_SIMS_H

// Headless AI-vs-AI simulation tests (the "sim" tier).
int test_ai_vs_ai_half_inning(void);
int test_ai_vs_ai_homerun(void);
int test_ai_vs_ai_determinism(void);
int test_different_seeds_produce_different_games(void);
int test_ai_offense_breakdown(void);
int test_no_batter_lock_stall(void);

// The tick equation made executable (ARCHITECTURE_VISION.md §8.8).
int test_world_snapshot_retick_is_identical(void);
int test_retick_diverges_when_engine_seed_is_not_restored(void);

#endif /* ALL_SIMS_H */
