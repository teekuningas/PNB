#include "batting_order_menu.h"
#include "render.h"
#include "font.h"
#include "globals.h"
#include "menu_types.h"
#include "menu_helpers.h"



MenuStage updateBattingOrderMenu(BattingOrderState *state, const KeyStates *keyStates, MenuStage currentStage, MenuMode menuMode, GameSetup *gameSetup)
{
	if (state->player_control == 2) { // AI control
		if (currentStage == MENU_STAGE_BATTING_ORDER_1) {
			memcpy(gameSetup->team1_batting_order, state->batting_order, sizeof(state->batting_order));
			return MENU_STAGE_BATTING_ORDER_2;
		} else { // Batting order 2
			memcpy(gameSetup->team2_batting_order, state->batting_order, sizeof(state->batting_order));
			if (menuMode == MENU_ENTRY_NORMAL || menuMode == MENU_ENTRY_SUPER_INNING) {
				return MENU_STAGE_HUTUNKEITTO;
			} else { // MENU_ENTRY_INTER_PERIOD
				return MENU_STAGE_GO_TO_GAME;
			}
		}
	}

	if (keyStates->released[state->player_control][KEY_2]) {
		if (state->pointer == 0) { // "Continue"
			if (currentStage == MENU_STAGE_BATTING_ORDER_1) {
				memcpy(gameSetup->team1_batting_order, state->batting_order, sizeof(state->batting_order));
				return MENU_STAGE_BATTING_ORDER_2;
			} else { // Batting order 2
				memcpy(gameSetup->team2_batting_order, state->batting_order, sizeof(state->batting_order));
				if (menuMode == MENU_ENTRY_NORMAL || menuMode == MENU_ENTRY_SUPER_INNING) {
					return MENU_STAGE_HUTUNKEITTO;
				} else { // MENU_ENTRY_INTER_PERIOD
					return MENU_STAGE_GO_TO_GAME;
				}
			}
		} else {
			if (state->mark == 0) {
				state->mark = state->pointer;
			} else {
				int temp = state->batting_order[state->pointer - 1];
				state->batting_order[state->pointer - 1] = state->batting_order[state->mark - 1];
				state->batting_order[state->mark - 1] = temp;
				state->mark = 0;
			}
		}
	}

	if (keyStates->released[state->player_control][KEY_DOWN]) {
		state->pointer = (state->pointer + 1) % state->rem;
	}
	if (keyStates->released[state->player_control][KEY_UP]) {
		state->pointer = (state->pointer + state->rem - 1) % state->rem;
	}

	return currentStage; // Stay in the current stage by default
}

void drawBattingOrderMenu(const BattingOrderState *state, MenuStage currentStage, const RenderState* rs, ResourceManager* rm)

{
	if (state->player_control == 2) { // AI, so nothing is drawn
		return;
	}

	begin_2d_render(rs);
	drawMenuLayout2D(rm, rs);

	const float center_x = VIRTUAL_WIDTH / 2.0f;
	const float title_y = VIRTUAL_HEIGHT * 0.05f;
	const float subtitle_y = VIRTUAL_HEIGHT * 0.15f;
	const float headers_y = VIRTUAL_HEIGHT * 0.25f;
	const float list_start_y = VIRTUAL_HEIGHT * 0.3f;
	const float option_spacing = VIRTUAL_HEIGHT * 0.045f;
	const float continue_y = VIRTUAL_HEIGHT * 0.9f;
	const float title_size = VIRTUAL_HEIGHT * 0.07f;
	const float subtitle_size = VIRTUAL_HEIGHT * 0.05f;
	const float text_size = VIRTUAL_HEIGHT * 0.035f;
	const float arrow_size = VIRTUAL_HEIGHT * 0.06f;
	const float col_number_x = VIRTUAL_WIDTH * 0.25f;
	const float col_name_x = VIRTUAL_WIDTH * 0.35f;
	const float col_speed_x = VIRTUAL_WIDTH * 0.65f;
	const float col_power_x = VIRTUAL_WIDTH * 0.75f;

	// --- Title and Subtitle ---
	draw_text_2d("Change Batting Order", center_x, title_y, title_size, TEXT_ALIGN_CENTER, rs);
	if (currentStage == MENU_STAGE_BATTING_ORDER_1) {
		draw_text_2d("Team 1", center_x, subtitle_y, subtitle_size, TEXT_ALIGN_CENTER, rs);
	} else {
		draw_text_2d("Team 2", center_x, subtitle_y, subtitle_size, TEXT_ALIGN_CENTER, rs);
	}

	// --- Column Headers ---
	draw_text_2d("Name", col_name_x, headers_y, text_size, TEXT_ALIGN_LEFT, rs);
	draw_text_2d("Speed", col_speed_x, headers_y, text_size, TEXT_ALIGN_LEFT, rs);
	draw_text_2d("Power", col_power_x, headers_y, text_size, TEXT_ALIGN_LEFT, rs);

	// --- Player List ---
	for(int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		char num_str[4];
		if(i < PLAYERS_IN_TEAM) {
			sprintf(num_str, "%d", i + 1);
		} else {
			sprintf(num_str, "J");
		}
		int player_index = state->batting_order[i];
		draw_text_2d(num_str, col_number_x, list_start_y + i * option_spacing, text_size, TEXT_ALIGN_LEFT, rs);
		draw_text_2d(state->players[player_index].name, col_name_x, list_start_y + i * option_spacing, text_size, TEXT_ALIGN_LEFT, rs);
		char stat_str[2];
		sprintf(stat_str, "%d", state->players[player_index].speed);
		draw_text_2d(stat_str, col_speed_x, list_start_y + i * option_spacing, text_size, TEXT_ALIGN_LEFT, rs);
		sprintf(stat_str, "%d", state->players[player_index].power);
		draw_text_2d(stat_str, col_power_x, list_start_y + i * option_spacing, text_size, TEXT_ALIGN_LEFT, rs);
	}

	// --- Continue Option ---
	draw_text_2d("Continue", center_x, continue_y, subtitle_size, TEXT_ALIGN_CENTER, rs);

	// --- Arrow ---
	float arrow_y;
	float current_arrow_x;
	if (state->pointer == 0) { // Pointing at "Continue"
		arrow_y = continue_y - (arrow_size - subtitle_size) / 2.0f;
		current_arrow_x = center_x + (VIRTUAL_WIDTH * 0.1f);
	} else { // Pointing at a player
		arrow_y = list_start_y + (state->pointer - 1) * option_spacing - (arrow_size - text_size) / 2.0f;
		current_arrow_x = col_power_x + (VIRTUAL_WIDTH * 0.03f);
	}
	draw_texture_2d(resource_manager_get_texture(rm, "data/textures/arrow.tga"), current_arrow_x, arrow_y, arrow_size, arrow_size);

	// --- Marked Player Arrow ---
	if (state->mark != 0) {
		float marked_arrow_y = list_start_y + (state->mark - 1) * option_spacing - (arrow_size - text_size) / 2.0f;
		float marked_arrow_x = col_power_x + (VIRTUAL_WIDTH * 0.03f);
		draw_texture_2d(resource_manager_get_texture(rm, "data/textures/arrow.tga"), marked_arrow_x, marked_arrow_y, arrow_size, arrow_size);
	}
}
