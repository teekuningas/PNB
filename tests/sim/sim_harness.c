#include "sim_harness.h"

// The test-state allocator lives in tests/integration/. Reference it by relative
// path so the quoted include resolves there and not to src/include/fixtures.h
// (a different header, GameSetup factories, that shares the file name).
#include "../integration/fixtures.h"

#include "game_frame.h"
#include "game_setup.h"
#include "game_reset.h"
#include "referee.h"
#include "state_validator.h"
#include "fill_player_data.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SimGame* sim_create(const GameSetup* setup, unsigned int seed)
{
    SimGame* g = malloc(sizeof(SimGame));
    memset(g, 0, sizeof(SimGame));
    g->seed = seed;

    // Allocate the world (match, rules, teamData, fieldPositions). setup_test_state
    // leaves keyStates and gameConclusion NULL — the real pipeline dereferences both
    // (action_invocations reads keyStates; consolidation writes gameConclusion at
    // period/game end), so allocate them here as zeroed.
    g->state = setup_test_state();
    g->state->keyStates = calloc(1, sizeof(KeyStates));
    g->state->gameConclusion = calloc(1, sizeof(GameConclusion));

    // Replace the dummy rosters with the real teams (data/teams.xml — team 0 is
    // vankkurit, team 1 huti, ...). setup_test_state only fills player names and
    // leaves speed/power uninitialized, which both breaks the batting AI's strategy
    // and makes the run nondeterministic. Real rosters give realistic, stable stats
    // — the same data the graphical client plays with. Free the dummy first (it was
    // allocated by setup_test_state), then load the real data over it.
    for (int t = 0; t < 2; t++) {
        for (int j = 0; j < PLAYERS_IN_TEAM + JOKER_COUNT; j++) {
            free(g->state->teamData[t].players[j].name);
        }
        free(g->state->teamData[t].players);
    }
    free(g->state->teamData);
    g->state->teamData = NULL;
    if (fill_player_data(g->state, "data/teams.xml") != 0) {
        printf("[sim] FATAL: could not load data/teams.xml (run from the PNB dir)\n");
    }

    // Render-free engine init (mirrors init_game_screen -> init_game_frame at startup;
    // init_player/init_ball are no-ops under -DNO_RENDER, so the ResourceManager is unused).
    if (init_game_frame(g->state, NULL) != 0) {
        printf("[sim] init_game_frame failed\n");
    }

    // Launch the game from the setup (mirrors a menu launch).
    initialize_game_from_menu(g->state, setup, &g->seed);

    // Mirror main.c::applyFixture period seeding for non-normal modes, so the
    // half-inning reset below builds the right phase (super inning / homerun contest).
    {
        Scoreboard* sb = &g->state->rules->scoreboard;
        if (setup->gameMode == GAME_MODE_SUPER_INNING) {
            sb->period = 2;
            sb->inning = sb->halfInningsInPeriod * 2;
        } else if (setup->gameMode == GAME_MODE_HOMERUN_CONTEST) {
            sb->period = 4;
            sb->inning = sb->halfInningsInPeriod * 2 + 2;
        }
    }

    // Perform the half-inning setup that load_game_screen_settings does on entering
    // SCREEN_GAME (initialize_game_from_menu set changeScreen=1 to request it).
    reset_for_new_half_inning(g->state->match, g->state->fieldPositions, g->state->teamData, g->state->rules, &g->seed);
    referee_reset_for_new_inning(
        &g->state->rules->referee, &g->state->rules->halfInningState, &g->state->rules->betweenPitchState
    );
    initialize_referee(g->state, &g->state->rules->referee);

    g->state->changeScreen = 0; // consumed
    g->state->updated = 1;
    g->state->screen = SCREEN_GAME;

    g->start_inning = g->state->rules->scoreboard.inning;
    g->start_period = g->state->rules->scoreboard.period;

    // Activate the production state validator so its invariants guard every frame.
    // init(NULL) clears the snapshot ring buffer and leaves no dump path (so dumps
    // are no-ops); set_active(1) turns the per-frame check on.
    state_validator_init(NULL);
    state_validator_set_active(1);

    return g;
}

void sim_make_normal_setup(GameSetup* setup, TeamID team1, TeamID team2, TeamControlMode c1, TeamControlMode c2)
{
    memset(setup, 0, sizeof(*setup));
    setup->launchType = GAME_LAUNCH_NEW;
    setup->gameMode = GAME_MODE_NORMAL;
    setup->team1 = team1;
    setup->team2 = team2;
    setup->team1_control = c1;
    setup->team2_control = c2;
    setup->halfInningsInPeriod = 4;
    setup->playsFirst = 0;
    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        setup->team1_batting_order[i] = i;
        setup->team2_batting_order[i] = i;
    }
}

void sim_attach(SimGame* g, SimFrameHook fn, void* ctx)
{
    if (!g || !fn || g->hook_count >= SIM_MAX_HOOKS) return;
    g->hooks[g->hook_count].fn = fn;
    g->hooks[g->hook_count].ctx = ctx;
    g->hook_count++;
}

void sim_fail(SimGame* g, const char* reason)
{
    if (!g || g->failed) return; // first failure wins
    g->failed = 1;
    snprintf(g->fail_reason, SIM_FAIL_REASON_LEN, "%s", reason ? reason : "(unknown)");
}

int sim_tick(SimGame* g)
{
    if (!g || g->failed) return 0;
    if (g->state->screen != SCREEN_GAME) return 0;

    update_game_frame(g->state, &g->menu, &g->seed);
    g->frame++;

    // The production validator pauses the game on an invariant violation. Headless
    // there is no other pause source (no HOME key), so a set pause means a violation.
    if (g->state->match->flowControl.pause) {
        sim_fail(g, "state validator tripped (flowControl.pause set)");
    }

    for (int i = 0; i < g->hook_count; i++) {
        g->hooks[i].fn(g, g->hooks[i].ctx);
    }

    if (g->failed) return 0;
    if (g->state->screen != SCREEN_GAME) return 0; // left to menu (period end / game over)
    return 1;
}

long sim_run_until(SimGame* g, SimPredicate pred, long max_frames)
{
    long start = g->frame;
    for (;;) {
        if (pred && pred(g)) return g->frame - start;
        if (g->frame - start >= max_frames) return -1;
        int alive = sim_tick(g);
        if (!alive && !(pred && pred(g))) return -1;
    }
}

void sim_destroy(SimGame* g)
{
    if (!g) return;
    if (g->state) {
        // teamData came from fill_player_data (not setup_test_state), so it must be freed
        // with clean_player_data — cleanup_test_state assumes the dummy layout. Free the
        // remaining setup_test_state allocations explicitly here.
        free(g->state->keyStates);
        free(g->state->gameConclusion);
        clean_player_data(g->state); // frees real teamData
        free(g->state->rules);
        free(g->state->clientInput);
        free(g->state->match);
        free(g->state->fieldPositions);
        free(g->state);
    }
    free(g);
}

int sim_pred_half_inning_ended(const SimGame* g)
{
    return g->state->rules->scoreboard.inning != g->start_inning;
}

int sim_pred_left_game_screen(const SimGame* g)
{
    return g->state->screen != SCREEN_GAME;
}
