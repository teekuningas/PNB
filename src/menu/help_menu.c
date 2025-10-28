#include "help_menu.h"
#include "menu_types.h"
#include "globals.h"
#include "render.h"
#include "font.h"
#include "resource_manager.h"
#include "menu_helpers.h"

void initHelpMenu(HelpMenuState *state)
{
	state->page = 0;
}

MenuStage updateHelpMenu(HelpMenuState *state, KeyStates *keyStates)
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

void drawHelpMenu(HelpMenuState *state, const RenderState* rs, ResourceManager* rm)
{
	begin_2d_render(rs);
	drawMenuLayout2D(rm, rs);

	// --- Layout Constants ---
	const float left_margin = rs->window_width * 0.08f;
	const float block_width = rs->window_width * 0.84f;
	const float top_margin = rs->window_height * 0.1f;
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
			content = "Batting is done by the action key. Movement of the batter starts "
			          "automatically when pitch is in air. If you dont press the key "
			          "before the marker reaches the end of the meter, batter will not bat. "
			          "If you want to bat, select power by pressing the key down when marker "
			          "reaches wanted height and then releasing it after marker has come down "
			          "to the yellow area. Balls vertical takeoff direction depends on the actual spot of the "
			          "marker in the yellow area. You can adjust the horizontal angle of "
			          "balls takeoff direction by strafing with batter. If there is no "
			          "batter you can browse through available batters by pressing the "
			          "second action key and select one by pressing the action key.";
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
			content = "Pitching is also done with the action key. To pitch you first hold "
			          "the action key down and the marker will rise. You select the power "
			          "by releasing action key and after that you must click the action "
			          "key when marker reaches the yellow area again. The final position "
			          "of marker in yellow area will affect the takeoff direction of ball.";
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
		draw_text_block_2d(content, left_margin, current_y + title_bottom_margin, block_width, text_size, line_spacing, rs);
	}
}
