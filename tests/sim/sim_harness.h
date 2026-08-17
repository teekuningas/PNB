#ifndef SIM_HARNESS_H
#define SIM_HARNESS_H

#include "globals.h"
#include "menu_types.h"
#include "game_setup.h"

/**
 * @file sim_harness.h
 * @brief Headless simulation harness — drives the *real* per-frame game pipeline.
 *
 * Unlike the scenario harness (tests/integration/scenario_builder.c), which
 * deliberately bypasses input, AI, meters and menus and injects intents by hand,
 * this harness boots a real game from a GameSetup (the same fixtures the graphical
 * client uses) and ticks the production `update_game_frame()` — including the whole
 * CONTROL stage (action_invocations + ai_update) plus update_meters + referee +
 * consolidation. With both teams set to CONTROL_AI this runs a real AI-vs-AI game
 * with zero rendering.
 *
 * Callers "attach" observers (per-frame hooks) to watch state evolve, assert
 * invariants, and record traces. See sim_observers.h for the built-in observers.
 */

#define SIM_MAX_HOOKS 8
#define SIM_FAIL_REASON_LEN 256

typedef struct SimGame SimGame;

/** Per-frame observer callback. Invoked after each ticked frame. */
typedef void (*SimFrameHook)(const SimGame* g, void* ctx);

/** Stop predicate for sim_run_until. Returns nonzero to stop. */
typedef int (*SimPredicate)(const SimGame* g);

struct SimGame {
    StateInfo* state; // owned (allocated via setup_test_state)
    MenuInfo menu; // zeroed; consumed by consolidation
    unsigned int seed; // deterministic RNG state, threaded through the pipeline
    long frame; // frames ticked so far

    int start_inning; // scoreboard.inning at create() — baseline for "inning changed"
    int start_period; // scoreboard.period at create()

    int failed; // set when an observer or the pipeline reports a problem
    char fail_reason[SIM_FAIL_REASON_LEN];

    struct {
        SimFrameHook fn;
        void* ctx;
    } hooks[SIM_MAX_HOOKS];
    int hook_count;
};

/**
 * @brief Boot a real headless game from a GameSetup, seeded deterministically.
 *
 * Mirrors the production startup + launch path: setup structs → init_game_frame
 * (render-free) → initialize_game_from_menu → the half-inning setup that
 * load_game_screen_settings performs. The validator is activated so the
 * production self-check guards every frame (a violation pauses the game, which
 * the harness reports as a failure).
 *
 * @param setup Fully populated GameSetup (use fixture_create_* helpers).
 * @param seed  Initial RNG seed (fixed → reproducible run).
 */
SimGame* sim_create(const GameSetup* setup, unsigned int seed);

/**
 * @brief Fill a GameSetup for a fresh normal game (sequential batting orders).
 *
 * The shared fixtures (src/core/fixtures.c) cover super-inning/homerun/cup but not
 * a plain normal-game start, which the scenarios here need. Mirrors those factories.
 */
void sim_make_normal_setup(GameSetup* setup, TeamID team1, TeamID team2, TeamControlMode c1, TeamControlMode c2);

/** Attach a per-frame observer. Hooks run in attach order after each tick. */
void sim_attach(SimGame* g, SimFrameHook fn, void* ctx);

/** Mark the run as failed with a reason (first failure wins). Used by observers. */
void sim_fail(SimGame* g, const char* reason);

/**
 * @brief Tick exactly one frame of the real pipeline, then run observers.
 * @return 1 if the game is still live and ok; 0 if it failed or left SCREEN_GAME.
 */
int sim_tick(SimGame* g);

/**
 * @brief Tick until `pred` is satisfied, the frame budget is hit, or a failure.
 * @return frames ticked if `pred` was met; -1 if the run failed or the budget
 *         was exhausted before `pred` (the caller treats -1 as "did not progress").
 */
long sim_run_until(SimGame* g, SimPredicate pred, long max_frames);

void sim_destroy(SimGame* g);

/* ---- World capture / restore ------------------------------------------- *
 * Everything that can influence a future frame: the World (ARCHITECTURE_VISION.md
 * §8.8 — MatchSession + GameRulesState) plus the controller memory that sits
 * beside it and outside it (the human's ClientInputState, the AI's
 * AIControllerState). Capture it, tick on, restore it, and the run resumes as if
 * nothing had happened — which is what lets a test re-tick a stretch of play, or
 * probe a stage with a perturbed input and then put the world back. */
typedef struct {
    MatchSession match;
    GameRulesState rules;
    ClientInputState clientInput;
    AIControllerState aiController;
    MenuInfo menu;
    long frame;
} SimWorldCapture;

void sim_capture_world(SimWorldCapture* cap, const SimGame* g);
void sim_restore_world(const SimWorldCapture* cap, SimGame* g);

/* ---- Common stop predicates -------------------------------------------- */

/** True once the half-inning has rolled over (scoreboard.inning advanced). */
int sim_pred_half_inning_ended(const SimGame* g);

/** True once the pipeline has left SCREEN_GAME (period boundary / game over). */
int sim_pred_left_game_screen(const SimGame* g);

#endif /* SIM_HARNESS_H */
