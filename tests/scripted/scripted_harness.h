#ifndef SCRIPTED_HARNESS_H
#define SCRIPTED_HARNESS_H

#include "sim_harness.h"

/**
 * @file scripted_harness.h
 * @brief Headless "scripted human" harness — drives the *real* input path.
 *
 * The three headless drivers that converge on one actualizer (ARCHITECTURE_VISION.md §8.5, §4.10):
 *   1. AI            — the sim tier (tests/sim/): both teams CONTROL_AI.
 *   2. Scripted human — THIS tier: a human-controlled team fed scripted key presses.
 *   3. Live human    — the graphical client.
 *
 * Unlike the scenario tier (tests/integration/scenario_builder.c), which injects ActionFlags /
 * machinery calls directly and *bypasses* action_invocations, this harness writes a scripted
 * `KeyStates` and ticks the production `update_game_frame()`, so the **real action_invocations**
 * translates keys → intent every frame. It is the one tier that exercises the human input path
 * headless — the path with zero coverage before now.
 *
 * It is a thin layer over the sim harness: it reuses sim_create/sim_tick/sim_destroy (boot, real
 * rosters, the production state validator, observers, teardown) and only adds the key-script feed.
 *
 * Key model: a script declares which keys are *held* on a given pad each frame; before each tick the
 * harness derives the production `KeyStates` from that:
 *   - `down[pad][key]`     = currently held,
 *   - `released[pad][key]` = was held last frame and not this frame (the one-frame release edge),
 * mirroring src/core/input.c semantics. action_invocations consumes both. A pad is a
 * TeamControlMode input slot (CONTROL_PLAYER_1 = 0, CONTROL_PLAYER_2 = 1).
 */

typedef struct ScriptedGame {
    SimGame* sim; // owned; the underlying real-pipeline game
    int held[HUMAN_PAD_COUNT][KEY_COUNT]; // desired held state for the NEXT tick (persists until changed)
    int prev_down[HUMAN_PAD_COUNT][KEY_COUNT]; // what was down last tick (to derive release edges)
} ScriptedGame;

/**
 * @brief Boot a real headless game with at least one human-controlled team.
 *
 * Mirrors sim_make_normal_setup + sim_create, but the control modes are the caller's: pass e.g.
 * (CONTROL_AI, CONTROL_PLAYER_1) for human-vs-AI. All keys start released.
 */
ScriptedGame* scripted_create(TeamID team1, TeamID team2, TeamControlMode c1, TeamControlMode c2, unsigned int seed);

/** Set / clear the held state of a key on a pad for subsequent ticks (held persists until changed). */
void scripted_hold(ScriptedGame* g, int pad, int key);
void scripted_release(ScriptedGame* g, int pad, int key);
/** Release every key on every pad. */
void scripted_release_all(ScriptedGame* g);

/**
 * @brief Tick exactly one frame: derive KeyStates from the held state, run the real pipeline.
 * @return 1 if the game is still live and ok; 0 if it failed or left SCREEN_GAME (see sim_tick).
 */
int scripted_tick(ScriptedGame* g);

/** Tick `frames` frames with the current held state held throughout. Returns frames actually ticked. */
long scripted_run(ScriptedGame* g, long frames);

/** Attach a per-frame observer (forwards to sim_attach on the underlying game). */
void scripted_attach(ScriptedGame* g, SimFrameHook fn, void* ctx);

/* ---- accessors --------------------------------------------------------- */
StateInfo* scripted_state(ScriptedGame* g);
MatchSession* scripted_match(ScriptedGame* g);
int scripted_failed(const ScriptedGame* g);
const char* scripted_fail_reason(const ScriptedGame* g);
long scripted_frame(const ScriptedGame* g);

void scripted_destroy(ScriptedGame* g);

#endif /* SCRIPTED_HARNESS_H */
