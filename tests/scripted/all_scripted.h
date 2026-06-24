#ifndef ALL_SCRIPTED_H
#define ALL_SCRIPTED_H

// Headless scripted-human tests (the "scripted" tier — driver 2 of ARCHITECTURE_VISION.md §8.5/§4.10).
int test_scripted_human_runs_headless(void);
int test_scripted_key_edges(void);
int test_scripted_input_reaches_pipeline(void);
int test_scripted_single_tap_does_not_run_batter(void);
int test_scripted_double_tap_runs_batter(void);

#endif /* ALL_SCRIPTED_H */
