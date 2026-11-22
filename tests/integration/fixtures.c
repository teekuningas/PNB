#include "fixtures.h"
#include "tick_outputs.h"
#include <stdio.h>
#include "field.h"
#include "key_states.h"
#include "team_data.h"
#include "rules_state.h"
#include "world.h"
#include "session.h"
#include "client_input.h"
#include "ai/ai_memory.h"
#include "field_layout.h"
#include "referee.h"
#include "game_frame.h"
#include "ai/ai_controller.h"
#include <stdlib.h>
#include <string.h>

StateInfo* setup_test_state()
{
    StateInfo* state = malloc(sizeof(StateInfo));
    memset(state, 0, sizeof(StateInfo));

    state->team_data = malloc(sizeof(TeamData) * 2);
    memset(state->team_data, 0, sizeof(TeamData) * 2);

    for (int i = 0; i < 2; ++i) {
        // Zeroed like everything else here: speed and power are read from the roster when a player is set
        // up, and a malloc'd roster holds whatever the allocator left — zero on a fresh page, garbage under
        // a sanitizer, where every runner came out too slow to move.
        state->team_data[i].players = calloc(12, sizeof(PlayerData));
        for (int j = 0; j < 12; ++j) {
            state->team_data[i].players[j].name = malloc(sizeof(char) * 10);
            sprintf(state->team_data[i].players[j].name, "Player %d", j + 1);
        }
    }

    state->field_positions = malloc(sizeof(FieldPositions));
    field_init_positions(state->field_positions);

    state->match = malloc(sizeof(MatchSession));
    memset(state->match, 0, sizeof(MatchSession));

    // Client-local input memory — a sibling of match (NOT synced; see ClientInputState).
    state->client_input = malloc(sizeof(ClientInputState));
    memset(state->client_input, 0, sizeof(ClientInputState));

    // Controller-private AI memory — the mirror sibling (NOT synced; see AIControllerState).
    state->ai_controller = malloc(sizeof(AIControllerState));
    init_ai_controller(state->ai_controller, 0);

    state->rules = malloc(sizeof(GameRulesState));
    memset(state->rules, 0, sizeof(GameRulesState));

    // Zero is not a legal referee state, and some of its fields mean something wrong when zeroed —
    // notably §12's last-batter indices, where 0 is a real player and the "nobody yet" value is -1.
    // Rather than re-list those defaults here (two copies of one recipe drift), hand the job to the
    // owner: the referee's own new-inning reset. create_scenario already ran it; the contract tier
    // now starts from the same legal ground instead of raw zeroes.
    referee_reset_for_new_inning(&state->rules->referee, &state->rules->half_inning, &state->rules->between_pitch);

    // The two members the reduced pipeline never needed, and so never had: `simulate_frames` skips
    // action_invocations entirely, and consolidation only reaches game_conclusion at a period end. A test
    // that drives the REAL update_game_frame reaches both on frame one — a human-controlled team makes
    // action_invocations read key_states — so leaving them NULL made the production entry point
    // uncallable from this tier. Both are pointer-free structs; zeroed KeyStates means "no key touched".
    state->key_states = malloc(sizeof(KeyStates));
    memset(state->key_states, 0, sizeof(KeyStates));

    state->game_conclusion = malloc(sizeof(GameConclusion));
    memset(state->game_conclusion, 0, sizeof(GameConclusion));

    // Initialize indices to -1
    for (int i = 0; i < 4; i++) {
        state->match->fielding_posts.baseman[i] = -1;
        state->match->fielding_posts.cover[i] = -1;
    }
    state->match->ball.holder = -1;
    state->match->ball.released_by = -1;
    state->match->catching.controlled_fielder = -1;

    return state;
}

void cleanup_test_state(StateInfo* state)
{
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 12; ++j) {
            free(state->team_data[i].players[j].name);
        }
        free(state->team_data[i].players);
    }
    free(state->rules);
    free(state->key_states);
    free(state->game_conclusion);
    free(state->client_input);
    free(state->ai_controller);
    free(state->match);
    free(state->field_positions);
    free(state->team_data);
    free(state);
}

void set_test_player_state(StateInfo* state, int playerIndex, PlayerUnitState newState)
{
    BattingPresence* presence = &state->match->players[playerIndex].batting.presence;

    // Simply set the new state enum - no legacy flags needed
    presence->state = newState;
}
