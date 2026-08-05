#include "menu_helpers.h"
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
        stateInfo->rules->scoreboard.teams[0].batterOrder, gameSetup->team1_batting_order,
        sizeof(gameSetup->team1_batting_order)
    );
    memcpy(
        stateInfo->rules->scoreboard.teams[1].batterOrder, gameSetup->team2_batting_order,
        sizeof(gameSetup->team2_batting_order)
    );
    stateInfo->rules->scoreboard.teams[0].batterOrderIndex = 0;
    stateInfo->rules->scoreboard.teams[1].batterOrderIndex = 0;
}

void launch_game_from_menu(StateInfo* stateInfo, const GameSetup* gameSetup, unsigned int* rng_seed)
{
    switch (gameSetup->launchType) {
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
        stateInfo->rules->scoreboard.playsFirst = gameSetup->playsFirst;
        return_to_game(stateInfo);
        break;
    case GAME_LAUNCH_RETURN_HOMERUN_CONTEST: {
        int pairCount = gameSetup->homerun_choice_count;
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < pairCount; j++) {
                stateInfo->rules->scoreboard.teams[0].batterRunnerIndices[i][j] = gameSetup->homerun_choices1[i][j];
                stateInfo->rules->scoreboard.teams[1].batterRunnerIndices[i][j] = gameSetup->homerun_choices2[i][j];
            }
        }
        stateInfo->rules->scoreboard.pairCount = pairCount;
        stateInfo->rules->homeRunContestState.runnerBatterPairCounter = 0;
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

    if (stateInfo->rules->scoreboard.isCupGame != 1) {
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

    const char* start = text;
    const char* end = text + strlen(text);
    float current_y = y;

    while (start < end) {
        const char* line_end = start;
        const char* last_space = NULL;

        // Find the longest possible line that fits within the width
        while (line_end < end) {
            const char* next_space = strchr(line_end + 1, ' ');
            const char* word_end = next_space ? next_space : end;

            int line_len = (word_end - start);
            if (get_text_width_2d(start, line_len, size) > width) {
                break; // Word doesn't fit, break the line here
            }

            last_space = line_end;
            line_end = word_end;

            if (!next_space) {
                break; // Reached end of string
            }
        }

        // If we couldn't fit even a single word, we must break it
        if (line_end == start) {
            // Find the exact character that overflows
            int char_count = 0;
            while (start + char_count < end) {
                if (get_text_width_2d(start, char_count + 1, size) > width) {
                    break;
                }
                char_count++;
            }
            line_end = start + char_count;
        } else if (last_space && line_end < end) {
            // Prefer to break at the last space to keep words whole
            line_end = last_space;
        }

        // Draw the line
        int line_len = line_end - start;
        char line_buffer[line_len + 1];
        memcpy(line_buffer, start, line_len);
        line_buffer[line_len] = '\0';

        print_text_2d(line_buffer, line_len, x, current_y, size);

        // Move to the next line
        current_y += lineSpacing;
        start = line_end;
        // Skip leading space on the next line
        if (*start == ' ') {
            start++;
        }
    }
}
