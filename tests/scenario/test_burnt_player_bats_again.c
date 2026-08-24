#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_scenarios.h"
#include "rules_pure/player_utils.h"

/**
 * REGRESSION (owner playtest, 2026-08-19): after a half-inning in which the batting team scored,
 * every newly picked batter walked to the out marker and came back, and the half-inning could never
 * progress. The batting order is a CYCLE and being burnt never costs a place in it (§12),
 * so the order is right to come round to a burnt player — what was wrong is that the previous
 * at-bat's verdict was still attached when it did, and enforce_legal_state acted on it. Full trace:
 * the batter-reselection deadlock.
 */
int test_burnt_player_bats_again(void)
{
    ScenarioContext* ctx = create_scenario();

    move_pitcher_away(ctx);
    initialize_referee_from_physical_state(ctx);

    StateInfo* st = ctx->state;
    GameRulesState* rules = st->rules;
    int battingTeamIndex = get_batting_team_index(&rules->scoreboard);

    int orderIndex = rules->scoreboard.teams[battingTeamIndex].batterOrderIndex;
    int outPlayer = rules->scoreboard.teams[battingTeamIndex].batterOrder[orderIndex];

    // Burnt earlier in this half-inning; let consolidation actualize it.
    rules->referee.battingPlayers[outPlayer].status = PLAYER_STATUS_OUT;
    simulate_frames(ctx, 2);
    ASSERT_EQ(
        PLAYER_STATE_OUT, st->match->playerInfo[outPlayer].bTPI.state,
        "setup: the burnt player should have been physically moved out"
    );

    // The order comes round to them again and they are handed the bat.
    st->match->pII.batterSelectionIndex = outPlayer;
    st->match->flowControl.waitingForBatterDecision = 1;
    st->match->aF.bTAF.choose_batter = CHOOSE_BATTER_SELECT;

    simulate_frames(ctx, 5);

    PlayerUnitState state = st->match->playerInfo[outPlayer].bTPI.state;
    RefereePlayerStatus status = rules->referee.battingPlayers[outPlayer].status;
    cleanup_scenario(ctx);

    ASSERT_NE(PLAYER_STATE_OUT, state, "a burnt player handed the bat must not be forced back out");
    ASSERT_EQ(PLAYER_STATE_AT_BAT, state, "a burnt player handed the bat must actually come to bat");
    ASSERT_EQ(PLAYER_STATUS_ACTIVE, status, "taking the bat must clear the previous at-bat's verdict");
    return TEST_PASSED;
}
