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
                      "key before the sweep ends, the batter will not swing. To bat, press "
                      "the action key once at the wanted power (low power bunts, higher "
                      "power swings). Once the pitch is in the air, the marker descends "
                      "across the meter; press the action key again in the yellow area to "
                      "set the vertical takeoff angle. If you committed power on a bad "
                      "pitch, press the second action key before contact to withdraw the "
                      "swing. Adjust the horizontal angle by strafing. When there is no "
                      "batter, browse available batters with the second action key and "
                      "select one with the action key.";
            break;
        case 2:
            title = "Running";
            content = "Running is controlled by the direction keys. Every direction key "
                      "represents a base. Down is the home, left is the first base, right "
                      "is the second base and up is the third base. If you just suddenly "
                      "feel the urge to run, you can double click one of these keys to "
                      "make runner at that base to run. Normally you probably should "
                      "synchronize running with pitching and batting though. This is done "
                      "by pressing the key once. That will make baserunners start running "
                      "immediately after pitch leaves pitchers hand. Batter will start "
                      "running after he has finished his swing or bunt. If you ended up "
                      "running but arent sure why and would like to return to the previous "
                      "base, you can press the corresponding direction key once and the "
                      "runner will return if he still is safe on that base. "
                      "If pitcher pitches enough invalid pitches you will be prompted if "
                      "you want to take a free walk or not. Action key accepts and second "
                      "action key rejects.";
            break;
        case 3:
            title = "Pitching";
            content = "Pitching uses three presses of the action key. Press the action key "
                      "once to start the power meter sweeping, and press it again at the "
                      "wanted height to lock pitch power and begin the windup (letting the "
                      "sweep run out cancels). During the windup the aim marker descends "
                      "from the right; press the action key a third time in the yellow "
                      "area to pitch onto the plate. Where the marker stops sets the "
                      "horizontal direction of the pitch. If you do not press during the "
                      "aim descent, the pitcher makes a fake pitch and keeps the ball.";
            break;
        case 4:
            title = "Fielding";
            content = "You move with a fielder by pressing direction keys. If you dont "
                      "have the ball, the action key will change the player. If you have "
                      "the ball, pressing action key alone will make player drop the ball. "
                      "If you hold one direction key when holding down the action key, "
                      "the player will load some throwing power. Releasing the key will "
                      "make player to throw the ball towards a base. The base corresponds "
                      "to direction key that was pressed. That is, down will be the home "
                      "base, left will be the first base, right will be the second base "
                      "and up will be the third base.";
            break;
        }

        draw_text_2d(title, left_margin, current_y, title_size, TEXT_ALIGN_LEFT, rs);
        draw_text_block_2d(
            content, left_margin, current_y + title_bottom_margin, block_width, text_size, line_spacing, rs
        );
    }
}
