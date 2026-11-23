#include "menu_helpers.h"
#include <string.h>
#include "display.h"
#include "session.h"
#include "resource_manager.h"
#include "render.h"
#include "font.h"
#include "front_menu.h"
#include "game_setup.h"
#include "cup.h"

// Draws a full-screen 2D background quad for menus
// Uses the "empty_background" texture from ResourceManager
void draw_menu_layout_2d(ResourceManager* rm, const RenderState* rs)
{
    // Bind the shared empty background texture
    GLuint tex = resource_manager_get_texture(rm, "data/textures/empty_background.tga");
    draw_texture_2d(tex, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
}

// Apply the batting orders chosen in the menu without touching the scoreboard's
// period/inning/run counters — used by the RETURN paths that resume an
// in-progress game rather than starting a fresh one.
static void apply_batting_orders(StateInfo* stateInfo, const GameSetup* gameSetup)
{
    memcpy(
        stateInfo->rules->scoreboard.teams[0].batter_order, gameSetup->team1_batting_order,
        sizeof(gameSetup->team1_batting_order)
    );
    memcpy(
        stateInfo->rules->scoreboard.teams[1].batter_order, gameSetup->team2_batting_order,
        sizeof(gameSetup->team2_batting_order)
    );
    stateInfo->rules->scoreboard.teams[0].batter_order_index = 0;
    stateInfo->rules->scoreboard.teams[1].batter_order_index = 0;
}

void launch_game_from_menu(StateInfo* stateInfo, const GameSetup* gameSetup, unsigned int* rng_seed)
{
    switch (gameSetup->launch_type) {
    case GAME_LAUNCH_NEW:
        initialize_game_from_menu(stateInfo, gameSetup, rng_seed);
        break;
    case GAME_LAUNCH_RETURN_INTER_PERIOD:
        apply_batting_orders(stateInfo, gameSetup);
        return_to_game(stateInfo);
        break;
    case GAME_LAUNCH_RETURN_SUPER_INNING:
        // Resume the game in the super inning the referee already set up (period
        // and run totals preserved). Only the new batting orders and the
        // leading-off team (decided fresh by hutunkeitto) are applied.
        apply_batting_orders(stateInfo, gameSetup);
        stateInfo->rules->scoreboard.plays_first = gameSetup->plays_first;
        return_to_game(stateInfo);
        break;
    case GAME_LAUNCH_RETURN_HOMERUN_CONTEST: {
        int pairCount = gameSetup->homerun_choice_count;
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < pairCount; j++) {
                stateInfo->rules->scoreboard.teams[0].batter_runner_indices[i][j] = gameSetup->homerun_choices1[i][j];
                stateInfo->rules->scoreboard.teams[1].batter_runner_indices[i][j] = gameSetup->homerun_choices2[i][j];
            }
        }
        stateInfo->rules->scoreboard.pair_count = pairCount;
        stateInfo->rules->homerun_contest.pair_index = 0;
        return_to_game(stateInfo);
    } break;
    }
}

void reset_menu_for_new_game(MenuData* menuData, StateInfo* stateInfo)
{
    // Reset the pending game setup to a clean state
    memset(&menuData->pendingGameSetup, 0, sizeof(GameSetup));
    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        menuData->pendingGameSetup.team1_batting_order[i] = i;
        menuData->pendingGameSetup.team2_batting_order[i] = i;
    }

    if (stateInfo->rules->scoreboard.is_cup_game != 1) {
        init_front_menu_state(&menuData->front_menu);
        menuData->stage = MENU_STAGE_FRONT;
    } else {
        menuData->stage = MENU_STAGE_CUP;
        menuData->cup_menu.screen = CUP_MENU_SCREEN_ONGOING;
        menuData->cup_menu.ongoing.pointer = 0;
        menuData->cup_menu.ongoing.rem = 5;
    }
}

void draw_text_2d(const char* text, float x, float y, float size, TextAlign align, const RenderState* rs)
{
    if (!text) return;

    float final_x = x;
    unsigned int len = strlen(text);

    if (align == TEXT_ALIGN_CENTER) {
        float text_width = get_text_width_2d(text, len, size);
        final_x = x - (text_width / 2.0f);
    }

    print_text_2d(text, len, final_x, y, size);
}

void draw_text_block_2d(
    const char* text, float x, float y, float width, float size, float lineSpacing, const RenderState* rs
)
{
    if (!text) return;

    const char* p = text;
    float current_y = y;

    while (*p != '\0') {
        // Explicit empty line (e.g., paragraph break "\n\n").
        if (*p == '\n') {
            current_y += lineSpacing;
            p++;
            continue;
        }

        const char* para_end = p;
        while (*para_end != '\0' && *para_end != '\n') {
            para_end++;
        }

        // Greedily pack whole words up to para_end that fit within width.
        const char* line_end = p;
        const char* scan = p;
        while (scan < para_end) {
            const char* word_end = scan;
            while (word_end < para_end && *word_end == ' ') {
                word_end++;
            }
            while (word_end < para_end && *word_end != ' ') {
                word_end++;
            }

            if (get_text_width_2d(p, (unsigned int)(word_end - p), size) > width) {
                break;
            }
            line_end = word_end;
            scan = word_end;
        }

        // If a single word overflows width, break by character (at least one for progress).
        if (line_end == p) {
            const char* c = p;
            while (c < para_end && get_text_width_2d(p, (unsigned int)(c - p + 1), size) <= width) {
                c++;
            }
            line_end = (c > p) ? c : p + 1;
        }

        print_text_2d(p, (unsigned int)(line_end - p), x, current_y, size);
        current_y += lineSpacing;

        p = line_end;
        while (*p == ' ') {
            p++;
        }
        if (*p == '\n') {
            p++;
        }
    }
}
