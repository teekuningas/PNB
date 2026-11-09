#define main golden_main
#include "golden.c"
#undef main
#include "sim_observers.h"

static void print_semantic_hashes(const SimGame* game)
{
    ChecksumObserver checksum;
    checksum_observer_init(&checksum);
    checksum_observer_hook(game, &checksum);
    for (int s = 0; s < NSEC; s++)
        len[s] = 0, buf[s][0] = 0;
    HomeRunView home_run = {0};
    PlayerView players = {0};
    project(game->state, &home_run, &players);
    printf("semantic %016llx %s\n", checksum.hash, buf[8]);
}

int main(void)
{
    GameRulesState rules = {0};
    MatchSession match = {0};
    ClientInputState client = {0};
    AIControllerState ai = {0};
    FieldPositions field = {0};
    StateInfo state = {
        .rules = &rules, .match = &match, .clientInput = &client, .aiController = &ai, .fieldPositions = &field
    };
    SimGame game = {.state = &state};
    printf("width %zu layout %zu\n", sizeof(rules.referee.battingPlayers[0].hasScored), sizeof(rules));

#ifdef WIDE_FLAGS
    // On the original all-integer layout, every bit and every array slot must still
    // be observed, including negative counters and otherwise unreachable enum values.
    unsigned int seed = 0x1234ABCDu;
    for (int sample = 0; sample < 64; sample++) {
        unsigned char bytes[sizeof(rules)];
        for (size_t i = 0; i < sizeof(bytes); i++) {
            seed = seed * 1664525u + 1013904223u;
            bytes[i] = seed >> 24;
        }
        memcpy(&rules, bytes, sizeof(rules));
        if (rules_projection_hash(&rules) != fnv(&rules, sizeof(rules), 1469598103934665603ULL)) {
            fprintf(stderr, "rules projection lost or reordered a value in sample %d\n", sample);
            return 1;
        }
    }
    memset(&rules, 0, sizeof(rules));
#endif

    print_semantic_hashes(&game);
    match.ballInfo.moving = 1;
    print_semantic_hashes(&game);
    match.ballInfo.onGround = 1;
    print_semantic_hashes(&game);
    match.ballInfo.pitch_in_flight = 1;
    print_semantic_hashes(&game);
    match.at_bat.swing.powerActive = 1;
    print_semantic_hashes(&game);
    match.at_bat.swing.verticalActive = 1;
    print_semantic_hashes(&game);
    rules.halfInningState.lastBatter.hasBattedAgain = 1;
    print_semantic_hashes(&game);
    rules.halfInningState.lastBatter.turnExhausted = 1;
    print_semantic_hashes(&game);
    rules.referee.battingPlayers[0].hasScored = 1;
    rules.referee.battingPlayers[0].runOfHonorScored = 1;
    rules.referee.battingPlayers[0].hasPendingRun = 1;
    rules.referee.battingPlayers[0].hasPendingRunOfHonor = 1;
    rules.referee.woundingEvaluationActive = 1;
    rules.referee.woundingEvaluationFinished = 1;
    rules.referee.ballInThirdBaseSincePitch = 1;
    rules.referee.homerunPairHasPitch = 1;
    rules.halfInningState.endPeriod = 1;
    rules.halfInningState.lastBatter.regularOrderSpent = 1;
    rules.betweenPitchState.catchHasBeenMade = 1;
    rules.betweenPitchState.hasBallHitGround = 1;
    rules.scoreboard.isCupGame = 1;
    print_semantic_hashes(&game);
    rules.scoreboard.teams[0].runs = 3;
    print_semantic_hashes(&game);
    return 0;
}
