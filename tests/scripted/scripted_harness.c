#include "scripted_harness.h"

#include <stdlib.h>
#include <string.h>

ScriptedGame* scripted_create(TeamID team1, TeamID team2, TeamControlMode c1, TeamControlMode c2, unsigned int seed)
{
    ScriptedGame* g = malloc(sizeof(ScriptedGame));
    memset(g, 0, sizeof(ScriptedGame));

    GameSetup setup;
    sim_make_normal_setup(&setup, team1, team2, c1, c2);
    g->sim = sim_create(&setup, seed);
    return g;
}

void scripted_hold(ScriptedGame* g, int pad, int key)
{
    if (!g || pad < 0 || pad >= HUMAN_PAD_COUNT || key < 0 || key >= KEY_COUNT) return;
    g->held[pad][key] = 1;
}

void scripted_release(ScriptedGame* g, int pad, int key)
{
    if (!g || pad < 0 || pad >= HUMAN_PAD_COUNT || key < 0 || key >= KEY_COUNT) return;
    g->held[pad][key] = 0;
}

void scripted_release_all(ScriptedGame* g)
{
    if (!g) return;
    memset(g->held, 0, sizeof(g->held));
}

int scripted_tick(ScriptedGame* g)
{
    if (!g || !g->sim) return 0;

    // Derive the production KeyStates from the held-key script. The release edge is
    // "was down last frame, not down this frame" — exactly what input.c produces and
    // what action_invocations' `released[...]` reads. The CONTROL_AI phantom row (2)
    // is intentionally left zeroed.
    KeyStates* ks = g->sim->state->keyStates;
    for (int pad = 0; pad < HUMAN_PAD_COUNT; pad++) {
        for (int key = 0; key < KEY_COUNT; key++) {
            int down = g->held[pad][key];
            ks->down[pad][key] = down;
            ks->released[pad][key] = (g->prev_down[pad][key] && !down) ? 1 : 0;
            ks->pressed[pad][key] = (!g->prev_down[pad][key] && down) ? 1 : 0;
            g->prev_down[pad][key] = down;
        }
    }

    return sim_tick(g->sim);
}

void scripted_tap(ScriptedGame* g, int pad, int key)
{
    scripted_hold(g, pad, key);
    scripted_tick(g); // key down (declarations fire on the release edge, not the press)
    scripted_release(g, pad, key);
    scripted_tick(g); // release edge -> action_invocations sets the intent, execute_actions consumes it
}

int scripted_tick_until_batter_decision(ScriptedGame* g, int budget)
{
    for (int i = 0; i < budget; i++) {
        scripted_tick(g);
        if (scripted_match(g)->flowControl.waitingForBatterDecision == 1) return 1;
    }
    return 0;
}

long scripted_run(ScriptedGame* g, long frames)
{
    long ticked = 0;
    for (long i = 0; i < frames; i++) {
        int alive = scripted_tick(g);
        ticked++;
        if (!alive) break;
    }
    return ticked;
}

void scripted_attach(ScriptedGame* g, SimFrameHook fn, void* ctx)
{
    if (!g) return;
    sim_attach(g->sim, fn, ctx);
}

StateInfo* scripted_state(ScriptedGame* g)
{
    return g ? g->sim->state : NULL;
}

MatchSession* scripted_match(ScriptedGame* g)
{
    return g ? g->sim->state->match : NULL;
}

int scripted_failed(const ScriptedGame* g)
{
    return g ? g->sim->failed : 1;
}

const char* scripted_fail_reason(const ScriptedGame* g)
{
    return g ? g->sim->fail_reason : "(null game)";
}

long scripted_frame(const ScriptedGame* g)
{
    return g ? g->sim->frame : 0;
}

void scripted_destroy(ScriptedGame* g)
{
    if (!g) return;
    sim_destroy(g->sim);
    free(g);
}
