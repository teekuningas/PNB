#include "game_over_menu.h"
#include "render.h"
#include "tick_outputs.h"
#include "menu_helpers.h"
#include <stdio.h>
#include "display.h"
#include "key_states.h"
#include "team_data.h"
#include "input.h"

MenuStage updateGameOverMenu(const GameConclusion* conclusion, const KeyStates* keyStates)
{
    if (any_human_released(keyStates, KEY_2)) {
        if (conclusion->is_cup_game) {
            return MENU_STAGE_CUP;
        }
        return MENU_STAGE_FRONT;
    }

    return MENU_STAGE_GAME_OVER;
}

void draw_game_over_menu(
    const GameConclusion* conclusion, const TeamData* teamData, RenderState* rs, ResourceManager* rm
)
{
    begin_2d_render(rs);
    draw_menu_layout_2d(rm, rs);

    char buffer[128];
    const float center_x = VIRTUAL_WIDTH / 2.0f;

    sprintf(buffer, "Team %d is victorious", conclusion->winner + 1);
    draw_text_2d(buffer, center_x, 150, 60.0f, TEXT_ALIGN_CENTER, rs);

    const char* winner_name = teamData[conclusion->winner].name;
    sprintf(buffer, "Congratulations %s!", winner_name);
    draw_text_2d(buffer, center_x, 250, 50.0f, TEXT_ALIGN_CENTER, rs);

    sprintf(buffer, "First period: %d - %d", conclusion->period0_runs[0], conclusion->period0_runs[1]);
    draw_text_2d(buffer, center_x, 380, 40.0f, TEXT_ALIGN_CENTER, rs);

    sprintf(buffer, "Second period: %d - %d", conclusion->period1_runs[0], conclusion->period1_runs[1]);
    draw_text_2d(buffer, center_x, 430, 40.0f, TEXT_ALIGN_CENTER, rs);

    // Super inning: show if anyone scored OR if homerun was played (implies super-inning happened)
    if (conclusion->period2_runs[0] > 0 || conclusion->period2_runs[1] > 0 || conclusion->period3_runs[0] > 0 ||
        conclusion->period3_runs[1] > 0) {
        sprintf(buffer, "Super inning: %d - %d", conclusion->period2_runs[0], conclusion->period2_runs[1]);
        draw_text_2d(buffer, center_x, 480, 40.0f, TEXT_ALIGN_CENTER, rs);
    }

    // Homerun contest: show if anyone scored
    if (conclusion->period3_runs[0] > 0 || conclusion->period3_runs[1] > 0) {
        sprintf(buffer, "Homerun contest: %d - %d", conclusion->period3_runs[0], conclusion->period3_runs[1]);
        draw_text_2d(buffer, center_x, 530, 40.0f, TEXT_ALIGN_CENTER, rs);
    }
}
