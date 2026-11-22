#include "scripted_harness.h"
#include "sim_harness.h"

int __wrap_sim_tick(SimGame* game);
int __wrap_scripted_tick(ScriptedGame* game);

// Linked only into the failure probe: the real golden loops must propagate a failed game.
int __wrap_sim_tick(SimGame* game)
{
    sim_fail(game, "injected golden-tool test failure");
    return 0;
}

int __wrap_scripted_tick(ScriptedGame* game)
{
    return __wrap_sim_tick(game->sim);
}
