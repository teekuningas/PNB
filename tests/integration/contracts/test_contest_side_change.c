#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_contracts.h"
#include "referee.h"

// CONTRACT: in the home-run contest, the side change hands the next half-inning its FIRST pair.
//
// The pair counter is legal state, and the referee is its one runtime writer: the half-inning that has just
// played its last pair leaves the counter at pairCount, and the referee puts it back to 0 at the side change,
// on the frame it pronounces RESETTING. The physical reset in that same frame then sets up the pair the
// counter names — so if the counter were still at pairCount, nobody would be sent to the plate at all.
//
// The reset lived in consolidation's reset recipe until 2026-10-27, the one write to the legal world outside
// the referee. Nothing in the test house noticed it moving: the contest sim tests end each turn before the
// next half-inning's pairs begin. Ablating the referee's reset turns this red.
int test_contest_side_change_starts_from_the_first_pair(void)
{
    ScenarioContext* ctx = create_scenario();
    Scoreboard* sb = &ctx->state->rules->scoreboard;
    RefereeState* referee = &ctx->state->rules->referee;

    // The first half of the contest's first round, all five pairs played.
    sb->period = 4;
    sb->inning = sb->halfInningsInPeriod * 2 + 2;
    sb->pairCount = 5;
    ctx->state->rules->homeRunContestState.runnerBatterPairCounter = 5;

    // The other side's first pair: player 3 bats, player 4 runs from third.
    const int nextBattingTeam = (sb->inning + 1 + sb->playsFirst + sb->period) % 2;
    for (int i = 0; i < MAX_HOMERUN_PAIRS; i++) {
        sb->teams[nextBattingTeam].batterRunnerIndices[0][i] = -1;
        sb->teams[nextBattingTeam].batterRunnerIndices[1][i] = -1;
    }
    sb->teams[nextBattingTeam].batterRunnerIndices[0][0] = 3;
    sb->teams[nextBattingTeam].batterRunnerIndices[1][0] = 4;

    // The referee has called the half-inning over and its timer is about to run out.
    referee->endOfInningState = END_INNING_STATE_DETECTED;
    referee->endInningTimer = 200;

    simulate_frames(ctx, 1);

    ASSERT_EQ(PERIOD_TRANSITION_NONE, (int)referee->periodTransition, "setup: a side change within the round");
    ASSERT_EQ(
        0, ctx->state->rules->homeRunContestState.runnerBatterPairCounter, "the next half-inning starts at pair one"
    );
    ASSERT_EQ(
        PLAYER_STATE_AT_BAT, (int)ctx->state->match->playerInfo[3].bTPI.state, "and that pair's batter is at the plate"
    );
    ASSERT_EQ(BASE_HOME, (int)ctx->state->match->playerInfo[3].bTPI.baseId, "at home");
    ASSERT_EQ(BASE_THIRD, (int)ctx->state->match->playerInfo[4].bTPI.baseId, "and its runner is on third");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
