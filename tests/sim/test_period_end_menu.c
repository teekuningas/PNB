#include "test_helpers.h"
#include "sim_harness.h"
#include "fixtures.h" // src/include/fixtures.h — GameSetup factories
#include "menu_types.h"

/**
 * A period's end opens the menu its transition calls for.
 *
 * ENFORCE only says that the game screen is done and which transition finished it (ConsolidationOutput);
 * the session turns that into the menu to open. That routing crosses the engine↔session boundary, and
 * the golden traces cannot see it — they stop on the frame the game screen is left — so it is held here,
 * by playing a real AI-vs-AI game of each mode to the end of its period:
 *
 *   - a normal game's first period ends into the between-periods menu, whatever the score;
 *   - a super inning ends into the home-run contest if it is still tied, into the game-over screen if not;
 *   - a contest round ends into another round if still tied, into the game-over screen if not;
 *   - and a game-over names the leader as the winner (GameConclusion, which ENFORCE still writes).
 */

#define PERIOD_BUDGET 120000L

static int play_to_the_period_end(SimGame* g)
{
    for (long i = 0; i < PERIOD_BUDGET; i++) {
        if (!sim_tick(g)) break;
    }
    return !g->failed && g->state->screen == SCREEN_MAIN_MENU;
}

// The referee files a finished period's runs into that period's own field and zeroes the running count
// (the contest's rounds accumulate), so the score that decided the period is read from there.
static int super_inning_runs(const SimGame* g, int t)
{
    return g->state->rules->scoreboard.teams[t].period2Runs;
}
static int contest_runs(const SimGame* g, int t)
{
    return g->state->rules->scoreboard.teams[t].period3Runs;
}

// Either the period was tied and the next menu continues the game, or it was not, and the game-over
// screen opens with the leader as the winner.
static int ends_as_its_score_says(const SimGame* g, int (*runs)(const SimGame*, int))
{
    int a = runs(g, 0), b = runs(g, 1);
    if (a == b) return g->menu.mode == MENU_ENTRY_HOMERUN_CONTEST;
    return g->menu.mode == MENU_ENTRY_GAME_OVER && g->state->gameConclusion->winner == (a > b ? 0 : 1);
}

int test_a_period_end_opens_the_menu_its_transition_calls_for(void)
{
    GameSetup setup;

    sim_make_normal_setup(&setup, 0, 1, CONTROL_AI, CONTROL_AI);
    SimGame* g = sim_create(&setup, 0x1234ABCDu);
    ASSERT_TRUE(play_to_the_period_end(g), "the normal game reaches the end of its first period cleanly");
    ASSERT_EQ(MENU_ENTRY_INTER_PERIOD, g->menu.mode, "the first period ends into the between-periods menu");
    sim_destroy(g);

    fixture_create_super_inning(&setup, 0, 1, CONTROL_AI, CONTROL_AI);
    g = sim_create(&setup, 0x50500000u);
    ASSERT_TRUE(play_to_the_period_end(g), "the super inning reaches its end cleanly");
    ASSERT_TRUE(
        ends_as_its_score_says(g, super_inning_runs),
        "a super inning ends into the contest if tied, into the game-over screen (and its winner) if not"
    );
    sim_destroy(g);

    fixture_create_homerun_contest(&setup, 0, 1, CONTROL_AI, CONTROL_AI);
    g = sim_create(&setup, 0x48520000u);
    ASSERT_TRUE(play_to_the_period_end(g), "the contest round reaches its end cleanly");
    ASSERT_TRUE(
        ends_as_its_score_says(g, contest_runs),
        "a contest round ends into another round if tied, into the game-over screen (and its winner) if not"
    );
    sim_destroy(g);

    return TEST_PASSED;
}
