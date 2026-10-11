#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_contracts.h"
#include "execute_actions.h"
#include <string.h>

/**
 * CONTRACT: a base-run command is for the runner holding that base at the top of the tick.
 *
 * The gate asks who holds the base and binds the command to that player; with nobody there it refuses,
 * and the actualizer is never asked to move nobody. The binding is the frame-top reading by
 * construction: holding a base is the referee's safety plus the player's baseId, and nothing between
 * the gate and the base running can change either for a player who already holds one.
 */

static void tick(ScenarioContext* ctx)
{
    execute_actions(
        ctx->state->match, ctx->state->rules, ctx->state->fieldPositions, &ctx->state->channels,
        &ctx->state->playSoundEffect
    );
}

static void run_command(ScenarioContext* ctx, BaseID base, RunIntent command)
{
    intent_push(
        &ctx->state->channels.batting,
        (IntentMessage){.kind = INTENT_BASE_RUN, .as.base_run = {.base = base, .command = command}}
    );
}

// The batting side reduced to one settled runner, holding first.
static int one_runner_on_first(ScenarioContext* ctx)
{
    MatchSession* m = ctx->state->match;
    RefereeState* referee = &ctx->state->rules->referee;
    initialize_referee_from_physical_state(ctx);
    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        m->playerInfo[i].bTPI.state = PLAYER_STATE_IDLE;
        m->playerInfo[i].bTPI.baseId = BASE_NONE;
        referee->battingPlayers[i].currentSafetyBase = BASE_NONE;
    }
    const int runner = 3;
    m->playerInfo[runner].bTPI.baseId = BASE_FIRST;
    m->playerInfo[runner].bTPI.state = PLAYER_STATE_ON_BASE;
    m->playerInfo[runner].cPI.moving = 0;
    referee->battingPlayers[runner].currentSafetyBase = BASE_FIRST;
    referee->battingPlayers[runner].baseAtPitchStart = BASE_FIRST;
    referee->battingPlayers[runner].status = PLAYER_STATUS_ACTIVE;
    return runner;
}

int test_a_base_run_needs_a_runner(void)
{
    ScenarioContext* ctx = create_scenario();
    one_runner_on_first(ctx);
    MatchSession* m = ctx->state->match;

    MatchSession before;
    memcpy(&before, m, sizeof(MatchSession));
    run_command(ctx, BASE_SECOND, RUN_COMMIT); // nobody holds second
    run_command(ctx, BASE_THIRD, RUN_BACK); // ...or third
    tick(ctx);
    MatchSession refused;
    memcpy(&refused, m, sizeof(MatchSession));

    memcpy(m, &before, sizeof(MatchSession));
    tick(ctx);
    ASSERT(memcmp(&refused, m, sizeof(MatchSession)) == 0, "commands for empty bases change nothing at all");

    // Control: the runner who does hold a base is armed by the same command.
    memcpy(m, &before, sizeof(MatchSession));
    run_command(ctx, BASE_FIRST, RUN_FORWARD);
    tick(ctx);
    ASSERT_EQ(1, m->pRAI.will_start_running[BASE_FIRST], "the runner on first is armed to go on the pitch");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// A command for a base that does not exist, or a command that is none of the three, is not refused — it is
// malformed: a broken producer, which fails the frame through the state validator rather than being told no.
int test_a_base_run_for_no_base_is_malformed(void)
{
    ScenarioContext* ctx = create_scenario();
    one_runner_on_first(ctx);
    run_command(ctx, (BaseID)BASE_COUNT, RUN_FORWARD);
    tick(ctx);
    ASSERT_EQ(1, ctx->state->channels.batting.malformed, "a base outside the field is flagged as malformed");
    cleanup_scenario(ctx);

    // And a command that is not one of the three: read as RUN_BACK it would quietly send a runner home.
    ctx = create_scenario();
    one_runner_on_first(ctx);
    run_command(ctx, BASE_FIRST, (RunIntent)(RUN_BACK + 1));
    tick(ctx);
    ASSERT_EQ(1, ctx->state->channels.batting.malformed, "a command outside RunIntent is flagged as malformed");
    cleanup_scenario(ctx);
    return TEST_PASSED;
}
