#include "help_menu.h"
#include "menu_types.h"
#include "display.h"
#include "key_states.h"
#include "render.h"
#include "resource_manager.h"
#include "menu_helpers.h"

void init_help_menu(HelpMenuState* state)
{
    state->page = 0;
}

MenuStage update_help_menu(HelpMenuState* state, KeyStates* keyStates)
{
    if (keyStates->released[0][KEY_1]) {
        return MENU_STAGE_FRONT;
    }

    if (keyStates->released[0][KEY_2]) {
        state->page++;
        if (state->page > 4) {
            return MENU_STAGE_FRONT;
        }
    }

    return MENU_STAGE_HELP;
}

void draw_help_menu(HelpMenuState* state, const RenderState* rs, ResourceManager* rm)
{
    begin_2d_render(rs);
    draw_menu_layout_2d(rm, rs);

    // --- Layout Constants ---
    const float left_margin = VIRTUAL_WIDTH * 0.08f;
    const float block_width = VIRTUAL_WIDTH * 0.84f;
    const float top_margin = VIRTUAL_HEIGHT * 0.1f;
    const float title_size = 48.0f;
    const float subtitle_size = 36.0f;
    const float text_size = 28.0f;
    const float line_spacing = 40.0f;
    const float title_bottom_margin = 70.0f;
    const float section_spacing = 40.0f;

    float current_y = top_margin;

    if (state->page == 0) {
        draw_text_2d("Controls", left_margin, current_y, title_size, TEXT_ALIGN_LEFT, rs);
        current_y += title_size + title_bottom_margin;

        // --- Pad 1 ---
        draw_text_2d("Pad 1", left_margin, current_y, subtitle_size, TEXT_ALIGN_LEFT, rs);
        current_y += subtitle_size + 15.0f;
        draw_text_2d("Direction keys - Arrow keys", left_margin, current_y, text_size, TEXT_ALIGN_LEFT, rs);
        current_y += line_spacing;
        draw_text_2d("Action key - Return", left_margin, current_y, text_size, TEXT_ALIGN_LEFT, rs);
        current_y += line_spacing;
        draw_text_2d("Second action key - Right shift", left_margin, current_y, text_size, TEXT_ALIGN_LEFT, rs);
        current_y += line_spacing;
        draw_text_2d("Left strafe - Right alt", left_margin, current_y, text_size, TEXT_ALIGN_LEFT, rs);
        current_y += line_spacing;
        draw_text_2d("Right strafe - Right ctrl", left_margin, current_y, text_size, TEXT_ALIGN_LEFT, rs);
        current_y += section_spacing;

        // --- Pad 2 ---
        draw_text_2d("Pad 2", left_margin, current_y, subtitle_size, TEXT_ALIGN_LEFT, rs);
        current_y += subtitle_size + 15.0f;
        draw_text_2d("Direction keys - G F H T", left_margin, current_y, text_size, TEXT_ALIGN_LEFT, rs);
        current_y += line_spacing;
        draw_text_2d("Action key - A", left_margin, current_y, text_size, TEXT_ALIGN_LEFT, rs);
        current_y += line_spacing;
        draw_text_2d("Second action key - Z", left_margin, current_y, text_size, TEXT_ALIGN_LEFT, rs);
        current_y += line_spacing;
        draw_text_2d("Left strafe - Left ctrl", left_margin, current_y, text_size, TEXT_ALIGN_LEFT, rs);
        current_y += line_spacing;
        draw_text_2d("Right strafe - Left alt", left_margin, current_y, text_size, TEXT_ALIGN_LEFT, rs);

    } else {
        const char* title = "";
        const char* content = "";

        switch (state->page) {
        case 1:
            title = "Batting";
            content = "Batting is done with the action key. The power meter sweeps once "
                      "when the pitcher begins the windup. If you do not press the action "
                      "key before the sweep ends, the batter will not swing.\n\n"
                      "To bat, press the action key once at the wanted power. Low power "
                      "bunts and higher power swings. Once the pitch is in the air, the "
                      "marker descends across the meter. Press the action key again in "
                      "the yellow area to set the vertical takeoff angle.\n\n"
                      "If you committed power on a bad pitch, press the second action key "
                      "before contact to withdraw the swing. Adjust the horizontal angle "
                      "by strafing.\n\n"
                      "When there is no batter, browse available batters with the second "
                      "action key and select one with the action key.";
            break;
        case 2:
            title = "Running";
            content = "Running is controlled by the direction keys. Every direction key "
                      "represents a base. Down is home, left is first base, right is "
                      "second base and up is third base.\n\n"
                      "Press a direction key once to synchronize running with the pitch. "
                      "Baserunners start running as soon as the pitch leaves the pitchers "
                      "hand. The batter starts running after finishing the swing or bunt. "
                      "Double click a direction key to send a runner immediately.\n\n"
                      "If a runner started running and you want him to return to the "
                      "previous base, press the same direction key once. He will return "
                      "if he is still safe on that base.\n\n"
                      "If the pitcher throws enough invalid pitches, you will be prompted "
                      "to take a free walk. Action key accepts and second action key "
                      "rejects.";
            break;
        case 3:
            title = "Pitching";
            content = "Pitching uses three presses of the action key. Press the action key "
                      "once to start the power meter sweeping, and press it again at the "
                      "wanted height to lock pitch power and begin the windup. Letting "
                      "the sweep run out cancels the pitch.\n\n"
                      "During the windup the aim marker descends from the right. Press "
                      "the action key a third time in the yellow area to pitch onto the "
                      "plate. Where the marker stops sets the horizontal direction of the "
                      "pitch.\n\n"
                      "If you do not press during the aim descent, the pitcher makes a "
                      "fake pitch and keeps the ball.\n\n"
                      "After a batter becomes a runner, earn pitch rights for the next "
                      "batter by moving the ball from home to any field base and back "
                      "to home. In the homerun contest, pitch rights are always granted.";
            break;
        case 4:
            title = "Fielding";
            content = "Move the active fielder with the direction keys. When you do not "
                      "have the ball, the action key switches to another fielder.\n\n"
                      "When you have the ball, pressing the action key alone drops the "
                      "ball. Hold a direction key together with the action key to load "
                      "throwing power, then release the action key to throw toward that "
                      "base.\n\n"
                      "The direction keys match the bases. Down is home, left is first "
                      "base, right is second base and up is third base.";
            break;
        }

        draw_text_2d(title, left_margin, current_y, title_size, TEXT_ALIGN_LEFT, rs);
        draw_text_block_2d(
            content, left_margin, current_y + title_size + title_bottom_margin, block_width, text_size, line_spacing, rs
        );
    }
}
