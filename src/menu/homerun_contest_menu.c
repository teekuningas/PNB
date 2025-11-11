#include "homerun_contest_menu.h"
#include "globals.h"
#include "render.h"
#include "font.h"
#include "menu_helpers.h"
#include <string.h>

void initHomerunContestState(HomerunContestState *state,
                             int team_index,
                             int player_control,
                             int choiceCount)
{
	state->team_index = team_index;
	state->player_control = player_control;
	state->pointer = 1; // start at first player slot
	state->rem = PLAYERS_IN_TEAM + JOKER_COUNT + 1; // slots + Continue
	state->choiceCount = choiceCount;
	state->choiceCounter = 0;
	// clear previous choices
	int pairCount = choiceCount / 2;
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < pairCount; j++) {
			state->choices[i][j] = -1;
		}
	}
}

MenuStage updateHomerunContestMenu(HomerunContestState *state,
                                   const KeyStates *keyStates,
                                   MenuStage currentStage,
                                   HomerunContestMenuOutput *output,
                                   const TeamData* teamData)
{
	int pairCount = state->choiceCount / 2;

	// Common logic for finishing a team's selection
	void finish_selection() {
		memcpy(output->choices, state->choices, sizeof(state->choices));
	}

	// AI auto-selection
	if (state->player_control == 2) {
		// first select batters by descending power
		int counter = 0, currentIndex = 0;
		// clear slots
		for (int i = 0; i < 2; i++)
			for (int j = 0; j < pairCount; j++)
				state->choices[i][j] = -1;
		// batters
		while (counter < pairCount) {
			for (int p = 0; p < PLAYERS_IN_TEAM + JOKER_COUNT; p++) {
				if (currentIndex == pairCount) break;
				if (teamData[state->team_index].players[p].power == 5 - counter) {
					state->choices[0][currentIndex++] = p;
				}
			}
			if (currentIndex == pairCount) break;
			counter++;
		}
		// runners by descending speed
		counter = 0;
		currentIndex = 0;
		while (counter < pairCount) {
			for (int p = 0; p < PLAYERS_IN_TEAM + JOKER_COUNT; p++) {
				if (currentIndex == pairCount) break;
				if (teamData[state->team_index].players[p].speed == 5 - counter) {
					int valid = 1;
					for (int k = 0; k < pairCount; k++) {
						if (state->choices[0][k] == p) {
							valid = 0;
							break;
						}
					}
					if (valid) state->choices[1][currentIndex++] = p;
				}
			}
			if (currentIndex == pairCount) break;
			counter++;
		}

		finish_selection();
		return (currentStage == MENU_STAGE_HOMERUN_CONTEST_1)
		       ? MENU_STAGE_HOMERUN_CONTEST_2
		       : MENU_STAGE_GO_TO_GAME;
	}

	// Human selection
	// undo last pick
	if (keyStates->released[state->player_control][KEY_1]) {
		if (state->choiceCounter > 0) {
			int idx = state->choiceCounter - 1;
			int row = idx / pairCount;
			int col = idx % pairCount;
			state->choices[row][col] = -1;
			state->choiceCounter--;
		}
	}
	// select or continue
	if (keyStates->released[state->player_control][KEY_2]) {
		if (state->pointer == 0) { // "Continue" selected
			if (state->choiceCounter >= state->choiceCount) {
				finish_selection();
				return (currentStage == MENU_STAGE_HOMERUN_CONTEST_1)
				       ? MENU_STAGE_HOMERUN_CONTEST_2
				       : MENU_STAGE_GO_TO_GAME;
			}
		} else if (state->choiceCounter < state->choiceCount) { // Player selected
			// validate no duplicate
			int valid = 1;
			for (int r = 0; r < 2; r++) {
				for (int c = 0; c < pairCount; c++) {
					if (state->choices[r][c] == state->pointer - 1) {
						valid = 0;
						break;
					}
				}
				if (!valid) break;
			}
			if (valid) {
				int idx = state->choiceCounter;
				int row = idx / pairCount;
				int col = idx % pairCount;
				state->choices[row][col] = state->pointer - 1;
				state->choiceCounter++;
			}
		}
	}
	// navigate cursor
	if (keyStates->released[state->player_control][KEY_DOWN]) {
		state->pointer = (state->pointer + 1) % state->rem;
	}
	if (keyStates->released[state->player_control][KEY_UP]) {
		state->pointer = (state->pointer + state->rem - 1) % state->rem;
	}

	// stay in current stage
	return currentStage;
}

void drawHomerunContestMenu(const HomerunContestState *state,
                            const RenderState *rs,
                            ResourceManager *rm,
                            const TeamData* teams)
{
	if (state->player_control == 2) return; // nothing to draw for AI

	begin_2d_render(rs);
	drawMenuLayout2D(rm, rs);

	const float center_x = VIRTUAL_WIDTH / 2.0f;
	const float title_y = VIRTUAL_HEIGHT * 0.05f;
	const float subtitle_y = VIRTUAL_HEIGHT * 0.12f;
	const float headers_y = VIRTUAL_HEIGHT * 0.22f;
	const float list_start_y = VIRTUAL_HEIGHT * 0.27f;
	const float option_spacing = VIRTUAL_HEIGHT * 0.045f;
	const float continue_y = VIRTUAL_HEIGHT * 0.9f;

	const float title_size = VIRTUAL_HEIGHT * 0.06f;
	const float subtitle_size = VIRTUAL_HEIGHT * 0.05f;
	const float text_size = VIRTUAL_HEIGHT * 0.035f;
	const float arrow_size = VIRTUAL_HEIGHT * 0.05f;

	const float col_number_x = VIRTUAL_WIDTH * 0.15f;
	const float col_name_x = VIRTUAL_WIDTH * 0.25f;
	const float col_speed_x = VIRTUAL_WIDTH * 0.55f;
	const float col_power_x = VIRTUAL_WIDTH * 0.65f;
	const float col_choice_x = VIRTUAL_WIDTH * 0.75f;


	// --- Title and Subtitle ---
	draw_text_2d("Homerun Contest Setup", center_x, title_y, title_size, TEXT_ALIGN_CENTER, rs);
	draw_text_2d(teams[state->team_index].name, center_x, subtitle_y, subtitle_size, TEXT_ALIGN_CENTER, rs);

	// --- Column Headers ---
	draw_text_2d("Name", col_name_x, headers_y, text_size, TEXT_ALIGN_LEFT, rs);
	draw_text_2d("Speed", col_speed_x, headers_y, text_size, TEXT_ALIGN_LEFT, rs);
	draw_text_2d("Power", col_power_x, headers_y, text_size, TEXT_ALIGN_LEFT, rs);
	draw_text_2d("Role", col_choice_x, headers_y, text_size, TEXT_ALIGN_LEFT, rs);


	// --- Player List ---
	int total_players = PLAYERS_IN_TEAM + JOKER_COUNT;
	for (int i = 0; i < total_players; i++) {
		float current_y = list_start_y + i * option_spacing;

		// Number/Joker
		char num_str[4];
		if (i < PLAYERS_IN_TEAM) {
			sprintf(num_str, "%d", i + 1);
		} else {
			sprintf(num_str, "J");
		}
		draw_text_2d(num_str, col_number_x, current_y, text_size, TEXT_ALIGN_LEFT, rs);

		// Name
		const char* name = teams[state->team_index].players[i].name;
		draw_text_2d(name, col_name_x, current_y, text_size, TEXT_ALIGN_LEFT, rs);

		// Speed
		char stat_str[2];
		sprintf(stat_str, "%d", teams[state->team_index].players[i].speed);
		draw_text_2d(stat_str, col_speed_x, current_y, text_size, TEXT_ALIGN_LEFT, rs);

		// Power
		sprintf(stat_str, "%d", teams[state->team_index].players[i].power);
		draw_text_2d(stat_str, col_power_x, current_y, text_size, TEXT_ALIGN_LEFT, rs);

		// Chosen Role (Batter/Runner)
		for (int r = 0; r < 2; r++) {
			for (int c = 0; c < state->choiceCount / 2; c++) {
				if (state->choices[r][c] == i) {
					char role_str[16]; // Buffer for "X. Batter" or "X. Runner"
					const char* role_type = (r == 0) ? "Batter" : "Runner";
					sprintf(role_str, "%d. %s", c + 1, role_type);
					draw_text_2d(role_str, col_choice_x, current_y, text_size, TEXT_ALIGN_LEFT, rs);
				}
			}
		}
	}

	// --- Instructions & Continue ---
	draw_text_2d("Continue", center_x, continue_y, subtitle_size, TEXT_ALIGN_CENTER, rs);

	// --- Arrow ---
	float arrow_y;
	float arrow_x;
	if (state->pointer == 0) { // Pointing at "Continue"
		arrow_y = continue_y - (arrow_size - subtitle_size) / 2.0f;
		arrow_x = center_x + (VIRTUAL_WIDTH * 0.1f);
	} else { // Pointing at a player
		arrow_y = list_start_y + (state->pointer - 1) * option_spacing - (arrow_size - text_size) / 2.0f;
		arrow_x = col_name_x + (VIRTUAL_WIDTH * 0.2f); // Position to the right of the name column
	}
	glBindTexture(GL_TEXTURE_2D, resource_manager_get_texture(rm, "data/textures/arrow.tga"));
	glBegin(GL_QUADS);
	glTexCoord2f(0, 1);
	glVertex2f(arrow_x, arrow_y);
	glTexCoord2f(0, 0);
	glVertex2f(arrow_x, arrow_y + arrow_size);
	glTexCoord2f(1, 0);
	glVertex2f(arrow_x + arrow_size, arrow_y + arrow_size);
	glTexCoord2f(1, 1);
	glVertex2f(arrow_x + arrow_size, arrow_y);
	glEnd();
}