#include "all_contracts.h"
#include "game_reset.h"
#include "rules_pure/player_utils.h"
#include "scenario_builder.h"
#include "test_helpers.h"

static void seed_play_resources(MatchSession* match, PlayerInfo expected[PLAYERS_IN_TEAM + JOKER_COUNT])
{
    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        PlayerInfo* p = &match->playerInfo[i];
        p->attributes.speed = 1 + i % 5;
        p->attributes.power = 5 - i % 5;
        p->batting.joker = i < PLAYERS_IN_TEAM ? JOKER_REGULAR : i == PLAYERS_IN_TEAM ? JOKER_USED : JOKER_AVAILABLE;
        p->batting.presence.state = PLAYER_STATE_RUNNING;
        p->batting.presence.base_id = BASE_SECOND;
        expected[i] = *p;
    }
}

static int check_play_resources(const MatchSession* match, const PlayerInfo expected[PLAYERS_IN_TEAM + JOKER_COUNT])
{
    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        const PlayerInfo* p = &match->playerInfo[i];
        ASSERT_EQ(expected[i].attributes.speed, p->attributes.speed, "a play reset preserves copied speed");
        ASSERT_EQ(expected[i].attributes.power, p->attributes.power, "a play reset preserves copied power");
        ASSERT_EQ(
            expected[i].batting.number, p->batting.number, "only the current contest batter gets a new display number"
        );
        ASSERT_EQ(expected[i].batting.joker, p->batting.joker, "a play reset neither refunds nor spends a joker");
    }
    return TEST_PASSED;
}

int test_half_inning_replaces_batting_attributes_and_resources(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* match = ctx->state->match;
    Scoreboard* scoreboard = &ctx->state->rules->scoreboard;
    for (int team = 0; team < 2; team++) {
        for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
            ctx->state->teamData[team].players[i].speed = 1 + (i + team * 2) % 5;
            ctx->state->teamData[team].players[i].power = 1 + (i + team) % 5;
            scoreboard->teams[team].batterOrder[i] = (i * 5 + team + 2) % (PLAYERS_IN_TEAM + JOKER_COUNT);
        }
    }
    for (int turn = 0; turn < 2; turn++) {
        scoreboard->inning = turn;
        const int side = get_batting_team_index(scoreboard);
        const TeamInfo* team = &scoreboard->teams[side];
        TeamData* roster = &ctx->state->teamData[team->value - 1];
        for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
            match->playerInfo[i].attributes.speed = -1;
            match->playerInfo[i].attributes.power = -1;
            match->playerInfo[i].batting.number = -1;
            match->playerInfo[i].batting.joker = JOKER_USED;
            match->playerInfo[i].batting.presence.state = PLAYER_STATE_RUNNING;
            match->playerInfo[i].batting.presence.base_id = BASE_SECOND;
        }

        reset_for_new_half_inning(match, ctx->state->fieldPositions, ctx->state->teamData, ctx->state->rules);

        for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
            PlayerInfo* p = &match->playerInfo[i];
            const int speed = roster->players[i].speed;
            const int power = roster->players[i].power;
            ASSERT_EQ(
                speed, p->attributes.speed, "speed comes from the current roster slot, not its batting-order position"
            );
            ASSERT_EQ(power, p->attributes.power, "power follows the side change, not the previous batting team");
            roster->players[i].speed = 0;
            roster->players[i].power = 0;
            ASSERT_EQ(speed, p->attributes.speed, "World attributes are copies, not live roster reads");
            ASSERT_EQ(power, p->attributes.power, "later roster changes do not change the active World");
            ASSERT_EQ(PLAYER_STATE_IDLE, p->batting.presence.state, "the previous play's physical presence is gone");
            ASSERT_EQ(BASE_NONE, p->batting.presence.base_id, "no previous base progress survives the half-inning");
        }
        for (int order = 0; order < PLAYERS_IN_TEAM + JOKER_COUNT; order++) {
            const PlayerInfo* p = &match->playerInfo[team->batterOrder[order]];
            ASSERT_EQ(
                order < PLAYERS_IN_TEAM ? order + 1 : 0, p->batting.number, "display numbers follow the batting order"
            );
            ASSERT_EQ(
                order < PLAYERS_IN_TEAM ? JOKER_REGULAR : JOKER_AVAILABLE, p->batting.joker, "new jokers are available"
            );
        }
    }
    cleanup_scenario(ctx);
    return TEST_PASSED;
}

int test_foul_restores_presence_without_replacing_batting_resources(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* match = ctx->state->match;
    RefereeState* referee = &ctx->state->rules->referee;
    PlayerInfo expected[PLAYERS_IN_TEAM + JOKER_COUNT];
    seed_play_resources(match, expected);
    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        referee->battingPlayers[i] =
            (RefereePlayerState){.baseAtPitchStart = BASE_NONE, .currentSafetyBase = BASE_NONE};
    }
    referee->battingPlayers[0].baseAtPitchStart = BASE_HOME;
    referee->battingPlayers[1].baseAtPitchStart = BASE_FIRST;
    referee->battingPlayers[2].baseAtPitchStart = BASE_SECOND;
    referee->battingPlayers[3].baseAtPitchStart = BASE_THIRD;
    referee->battingPlayers[4].status = PLAYER_STATUS_OUT;
    referee->battingPlayers[5].status = PLAYER_STATUS_WOUNDED;
    referee->battingPlayers[6].hasScored = 1;
    const PlayerUnitState states[] = {PLAYER_STATE_AT_BAT,  PLAYER_STATE_ON_BASE, PLAYER_STATE_ON_BASE,
                                      PLAYER_STATE_ON_BASE, PLAYER_STATE_OUT,     PLAYER_STATE_WOUNDED,
                                      PLAYER_STATE_SCORED};
    const BaseID bases[] = {BASE_HOME, BASE_FIRST, BASE_SECOND, BASE_THIRD, BASE_NONE, BASE_NONE, BASE_NONE};

    reset_for_foul_play(match, ctx->state->fieldPositions, ctx->state->rules);

    ASSERT(check_play_resources(match, expected) == TEST_PASSED, "a foul preserves half-inning resources");
    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        ASSERT_EQ(
            i < 7 ? states[i] : PLAYER_STATE_IDLE, match->playerInfo[i].batting.presence.state,
            "presence follows legal snapshots"
        );
        ASSERT_EQ(
            i < 7 ? bases[i] : BASE_NONE, match->playerInfo[i].batting.presence.base_id,
            "base progress is restored or cleared"
        );
    }
    ASSERT_FLOAT_EQ(
        ctx->state->fieldPositions->firstBaseRun.x, match->playerInfo[1].body.location.x, 1e-6f,
        "restored presence still puts the runner physically at first"
    );
    ASSERT_FLOAT_EQ(
        ctx->state->fieldPositions->thirdBaseRun.z, match->playerInfo[3].body.location.z, 1e-6f,
        "restored third-base progress is not merely a flag"
    );
    cleanup_scenario(ctx);
    return TEST_PASSED;
}

int test_contest_pair_numbers_and_presence_have_distinct_lifetimes(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* match = ctx->state->match;
    Scoreboard* scoreboard = &ctx->state->rules->scoreboard;
    HomeRunContestState* contest = &ctx->state->rules->homeRunContestState;
    scoreboard->period = 4;
    scoreboard->pairCount = 3;
    TeamInfo* team = &scoreboard->teams[get_batting_team_index(scoreboard)];
    const int batters[] = {5, 7, 1};
    const int runners[] = {8, 3, 11};
    for (int pair = 0; pair < 3; pair++) {
        team->batterRunnerIndices[0][pair] = batters[pair];
        team->batterRunnerIndices[1][pair] = runners[pair];
    }
    PlayerInfo expected[PLAYERS_IN_TEAM + JOKER_COUNT];
    seed_play_resources(match, expected);
    for (int stage = 0; stage < 3; stage++) {
        const int pair = stage == 2 ? 2 : 1;
        const int batter = batters[pair];
        const int runner = runners[pair];
        contest->runnerBatterPairCounter = pair;
        expected[batter].batting.number = pair + 1;
        for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
            match->playerInfo[i].batting.presence.state = PLAYER_STATE_RUNNING;
            match->playerInfo[i].batting.presence.base_id = BASE_SECOND;
        }
        if (stage == 1) {
            reset_for_foul_play(match, ctx->state->fieldPositions, ctx->state->rules);
            ASSERT_FLOAT_EQ(
                match->playerInfo[batter].body.target_location.x, match->playerInfo[batter].body.location.x, 1e-6f,
                "a contest foul resumes the same batter at its target"
            );
            ASSERT_FLOAT_EQ(
                match->playerInfo[batter].body.target_location.z, match->playerInfo[batter].body.location.z, 1e-6f,
                "the returning batter need not walk in again"
            );
        } else {
            reset_for_next_pair(match, ctx->state->fieldPositions, scoreboard, contest);
        }
        ASSERT(
            check_play_resources(match, expected) == TEST_PASSED, "pair changes do not start a new resource lifetime"
        );
        for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
            PlayerUnitState state = i == batter   ? PLAYER_STATE_AT_BAT
                                    : i == runner ? PLAYER_STATE_ON_BASE
                                                  : PLAYER_STATE_IDLE;
            BaseID base = i == batter ? BASE_HOME : i == runner ? BASE_THIRD : BASE_NONE;
            ASSERT_EQ(
                state, match->playerInfo[i].batting.presence.state, "only the current pair has live physical presence"
            );
            ASSERT_EQ(
                base, match->playerInfo[i].batting.presence.base_id, "other pair slots retain no active base progress"
            );
        }
        ASSERT_FLOAT_EQ(
            ctx->state->fieldPositions->thirdBaseRun.x, match->playerInfo[runner].body.location.x, 1e-6f,
            "the selected runner is physically at third"
        );
    }
    cleanup_scenario(ctx);
    return TEST_PASSED;
}
