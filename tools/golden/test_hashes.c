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
        .rules = &rules, .match = &match, .client_input = &client, .ai_controller = &ai, .field_positions = &field
    };
    SimGame game = {.state = &state};
    printf("width %zu layout %zu\n", sizeof(rules.referee.batting_players[0].has_scored), sizeof(rules));

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
    match.ball.moving = 1;
    print_semantic_hashes(&game);
    match.ball.on_ground = 1;
    print_semantic_hashes(&game);
    match.ball.pitch_in_flight = 1;
    print_semantic_hashes(&game);
    match.at_bat.swing.power_active = 1;
    print_semantic_hashes(&game);
    match.at_bat.swing.vertical_active = 1;
    print_semantic_hashes(&game);
    rules.half_inning.last_batter.has_batted_again = 1;
    print_semantic_hashes(&game);
    rules.half_inning.last_batter.turn_exhausted = 1;
    print_semantic_hashes(&game);
    rules.referee.batting_players[0].has_scored = 1;
    rules.referee.batting_players[0].run_of_honor_scored = 1;
    rules.referee.batting_players[0].has_pending_run = 1;
    rules.referee.batting_players[0].has_pending_run_of_honor = 1;
    rules.referee.wounding_evaluation_active = 1;
    rules.referee.wounding_evaluation_finished = 1;
    rules.referee.ball_in_third_base_since_pitch = 1;
    rules.referee.homerun_pair_has_pitch = 1;
    rules.half_inning.end_period = 1;
    rules.half_inning.batter_may_leave_home = 1;
    rules.half_inning.batter_in_turn = 1;
    rules.half_inning.last_batter.regular_order_spent = 1;
    rules.between_pitch.catch_has_been_made = 1;
    rules.between_pitch.has_ball_hit_ground = 1;
    rules.scoreboard.is_cup_game = 1;
    print_semantic_hashes(&game);
    rules.scoreboard.teams[0].runs = 3;
    print_semantic_hashes(&game);
    return 0;
}
