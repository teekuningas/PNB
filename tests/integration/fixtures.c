#include "tests/integration/fixtures.h"
#include "core/field_layout.h"
#include "core/fill_player_data.h"
#include "game/game_setup.h"
#include "game/mutable_world.h"
#include <stdlib.h>
#include <string.h>

StateInfo* setup_test_state() {
    StateInfo* state = malloc(sizeof(StateInfo));
    memset(state, 0, sizeof(StateInfo));

    state->team_data = malloc(sizeof(TeamData) * 2);
    memset(state->team_data, 0, sizeof(TeamData) * 2);

    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 12; ++j) {
            sprintf(state->team_data[i].players[j].name, "Player %d", j + 1);
            state->team_data[i].players[j].batting_hand = 1;
            state->team_data[i].players[j].throwing_hand = 1;
        }
    }

    state->field_positions = malloc(sizeof(FieldPositions));
    initialize_field_layout(state->field_positions);
    
    state->game = malloc(sizeof(Game));
    memset(state->game, 0, sizeof(Game));

    state->world = malloc(sizeof(World));
    memset(state->world, 0, sizeof(World));

    return state;
}

void setup_runner_at_first_base(StateInfo* state) {
    initializeGameFromMenu(state, 0, 1);
    state->game->runner_on_base[0] = 1;
}
