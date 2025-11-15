#include "team_selection_menu.h"
#include "render.h"
#include "font.h"
#include "menu_types.h"
#include "menu_helpers.h"

#define DEFAULT_CONTROLLED_1 0
#define DEFAULT_CONTROLLED_2 2

void initTeamSelectionState(TeamSelectionState *state, int numTeams)
{
	state->state = TEAM_SELECTION_STAGE_TEAM_1;
	state->pointer = DEFAULT_TEAM_1;
	state->team1 = DEFAULT_TEAM_1;
	state->team2 = DEFAULT_TEAM_2;
	state->team1_controller = DEFAULT_CONTROLLED_1;
	state->team2_controller = DEFAULT_CONTROLLED_2;
	state->innings = 2; // Default innings
	state->rem = numTeams;
	state->numTeams = numTeams;
}

MenuStage updateTeamSelectionMenu(TeamSelectionState *state, const KeyStates *keyStates, GameSetup *gameSetup)
{
	switch(state->state) {
	case TEAM_SELECTION_STAGE_TEAM_1:
		if(keyStates->released[0][KEY_1]) {
			return MENU_STAGE_FRONT;
		}
		if(keyStates->released[0][KEY_2]) {
			state->state = TEAM_SELECTION_STAGE_CONTROL_1;
			state->team1 = state->pointer;
			state->pointer = DEFAULT_CONTROLLED_1;
			state->rem = 3;
		}
		if(keyStates->released[0][KEY_DOWN]) {
			state->pointer +=1;
			state->pointer = (state->pointer+state->rem)%state->rem;
		}
		if(keyStates->released[0][KEY_UP]) {
			state->pointer -=1;
			state->pointer = (state->pointer+state->rem)%state->rem;
		}
		break;
	case TEAM_SELECTION_STAGE_CONTROL_1:
		if(keyStates->released[0][KEY_1]) {
			state->state = TEAM_SELECTION_STAGE_TEAM_1;
			state->pointer = state->team1;
			state->rem = state->numTeams;

		}
		if(keyStates->released[0][KEY_2]) {
			state->state = TEAM_SELECTION_STAGE_TEAM_2;
			state->team1_controller = state->pointer;
			state->pointer = DEFAULT_TEAM_2;
			state->rem = state->numTeams;
		}
		if(keyStates->released[0][KEY_DOWN]) {
			state->pointer +=1;
			state->pointer = (state->pointer+state->rem)%state->rem;
		}
		if(keyStates->released[0][KEY_UP]) {
			state->pointer -=1;
			state->pointer = (state->pointer+state->rem)%state->rem;
		}
		break;
	case TEAM_SELECTION_STAGE_TEAM_2:
		if(keyStates->released[0][KEY_1]) {
			state->state = TEAM_SELECTION_STAGE_CONTROL_1;
			state->rem = 3;
			state->pointer = state->team1_controller;

		}
		if(keyStates->released[0][KEY_2]) {
			state->state = TEAM_SELECTION_STAGE_CONTROL_2;
			state->team2 = state->pointer;
			state->rem = 3;
			state->pointer = DEFAULT_CONTROLLED_2;
			if(state->pointer == state->team1_controller) {
				state->pointer++;
				state->pointer = (state->pointer+state->rem)%state->rem;
			}
		}
		if(keyStates->released[0][KEY_DOWN]) {
			state->pointer +=1;
			state->pointer = (state->pointer+state->rem)%state->rem;
		}
		if(keyStates->released[0][KEY_UP]) {
			state->pointer -=1;
			state->pointer = (state->pointer+state->rem)%state->rem;
		}
		break;
	case TEAM_SELECTION_STAGE_CONTROL_2:
		if(keyStates->released[0][KEY_1]) {
			state->state = TEAM_SELECTION_STAGE_TEAM_2;
			state->rem = state->numTeams;
			state->pointer = state->team2;

		}
		if(keyStates->released[0][KEY_2]) {
			state->state = TEAM_SELECTION_STAGE_INNINGS;
			state->team2_controller = state->pointer;
			state->pointer = 1;
			state->rem = 3;
		}
		if(keyStates->released[0][KEY_DOWN]) {
			state->pointer +=1;
			state->pointer = (state->pointer+state->rem)%state->rem;
			if(state->pointer == state->team1_controller) {
				state->pointer++;
				state->pointer = (state->pointer+state->rem)%state->rem;
			}
		}
		if(keyStates->released[0][KEY_UP]) {
			state->pointer -=1;
			state->pointer = (state->pointer+state->rem)%state->rem;
			if(state->pointer == state->team1_controller) {
				state->pointer--;
				state->pointer = (state->pointer+state->rem)%state->rem;
			}
		}
		break;
	case TEAM_SELECTION_STAGE_INNINGS:
		if(keyStates->released[0][KEY_1]) {
			state->state = TEAM_SELECTION_STAGE_CONTROL_2;
			state->rem = 3;
			state->pointer = state->team2_controller;
			if(state->pointer == state->team1_controller) {
				state->pointer++;
				state->pointer = (state->pointer+state->rem)%state->rem;
			}
		}
		if(keyStates->released[0][KEY_2]) {
			if(state->pointer == 0) state->innings = 2;
			else if(state->pointer == 1) state->innings = 4;
			else if(state->pointer == 2) state->innings = 8;

			gameSetup->team1 = state->team1;
			gameSetup->team2 = state->team2;
			gameSetup->team1_control = state->team1_controller;
			gameSetup->team2_control = state->team2_controller;
			gameSetup->halfInningsInPeriod = state->innings;

			return MENU_STAGE_BATTING_ORDER_1;
		}
		if(keyStates->released[0][KEY_DOWN]) {
			state->pointer +=1;
			state->pointer = (state->pointer+state->rem)%state->rem;
		}
		if(keyStates->released[0][KEY_UP]) {
			state->pointer -=1;
			state->pointer = (state->pointer+state->rem)%state->rem;
		}
		break;
	}
	return MENU_STAGE_TEAM_SELECTION; // Stay in this stage by default
}

void drawTeamSelectionMenu(const TeamSelectionState *state, const TeamData* teamData, const RenderState* rs, ResourceManager* rm)
{
	begin_2d_render(rs);
	drawMenuLayout2D(rm, rs);

	const float center_x = VIRTUAL_WIDTH / 2.0f;

	const float title_y = VIRTUAL_HEIGHT * 0.1f;
	const float team_name_y = VIRTUAL_HEIGHT * 0.25f;
	const float options_start_y = VIRTUAL_HEIGHT * 0.35f;
	const float option_spacing = VIRTUAL_HEIGHT * 0.07f;
	const float arrow_size = VIRTUAL_HEIGHT * 0.06f;

	const float column_1_x = VIRTUAL_WIDTH * 0.35f;
	const float column_2_x = VIRTUAL_WIDTH * 0.65f;

	const float title_size = VIRTUAL_HEIGHT * 0.08f;
	const float subtitle_size = VIRTUAL_HEIGHT * 0.06f;
	const float text_size = VIRTUAL_HEIGHT * 0.045f;

	// --- Title ---
	draw_text_2d("Game Setup", center_x, title_y, title_size, TEXT_ALIGN_CENTER, rs);

	// --- Team 1 Column ---
	if (state->state >= TEAM_SELECTION_STAGE_TEAM_1 && state->state < TEAM_SELECTION_STAGE_CONTROL_1) {
		draw_text_2d("Team 1", column_1_x, team_name_y, subtitle_size, TEXT_ALIGN_CENTER, rs);
		for (int i = 0; i < state->numTeams; i++) {
			const char* team_name = teamData[i].name;
			draw_text_2d(team_name, column_1_x, options_start_y + i * option_spacing, text_size, TEXT_ALIGN_CENTER, rs);
		}
	}

	// --- Team 1 Controller Column ---
	if (state->state >= TEAM_SELECTION_STAGE_CONTROL_1 && state->state < TEAM_SELECTION_STAGE_TEAM_2) {
		draw_text_2d("Controlled by", column_1_x, team_name_y, subtitle_size, TEXT_ALIGN_CENTER, rs);
		draw_text_2d("Pad 1", column_1_x, options_start_y, text_size, TEXT_ALIGN_CENTER, rs);
		draw_text_2d("Pad 2", column_1_x, options_start_y + option_spacing, text_size, TEXT_ALIGN_CENTER, rs);
		draw_text_2d("AI", column_1_x, options_start_y + 2 * option_spacing, text_size, TEXT_ALIGN_CENTER, rs);
	}

	// --- Team 2 Column ---
	if (state->state >= TEAM_SELECTION_STAGE_TEAM_2 && state->state < TEAM_SELECTION_STAGE_CONTROL_2) {
		draw_text_2d("Team 2", column_2_x, team_name_y, subtitle_size, TEXT_ALIGN_CENTER, rs);
		for (int i = 0; i < state->numTeams; i++) {
			const char* team_name = teamData[i].name;
			draw_text_2d(team_name, column_2_x, options_start_y + i * option_spacing, text_size, TEXT_ALIGN_CENTER, rs);
		}
	}

	// --- Team 2 Controller Column ---
	if (state->state >= TEAM_SELECTION_STAGE_CONTROL_2 && state->state < TEAM_SELECTION_STAGE_INNINGS) {
		draw_text_2d("Controlled by", column_2_x, team_name_y, subtitle_size, TEXT_ALIGN_CENTER, rs);
		draw_text_2d("Pad 1", column_2_x, options_start_y, text_size, TEXT_ALIGN_CENTER, rs);
		draw_text_2d("Pad 2", column_2_x, options_start_y + option_spacing, text_size, TEXT_ALIGN_CENTER, rs);
		draw_text_2d("AI", column_2_x, options_start_y + 2 * option_spacing, text_size, TEXT_ALIGN_CENTER, rs);
	}

	// --- Innings Column ---
	if (state->state == TEAM_SELECTION_STAGE_INNINGS) {
		draw_text_2d("Innings", center_x, team_name_y, subtitle_size, TEXT_ALIGN_CENTER, rs);
		draw_text_2d("1", center_x, options_start_y, text_size, TEXT_ALIGN_CENTER, rs);
		draw_text_2d("2", center_x, options_start_y + option_spacing, text_size, TEXT_ALIGN_CENTER, rs);
		draw_text_2d("4", center_x, options_start_y + 2 * option_spacing, text_size, TEXT_ALIGN_CENTER, rs);
	}


	// --- Arrow ---
	float arrow_x = 0;
	float arrow_y = options_start_y + state->pointer * option_spacing;

	switch(state->state) {
	case TEAM_SELECTION_STAGE_TEAM_1:
	case TEAM_SELECTION_STAGE_CONTROL_1:
		arrow_x = column_1_x + (VIRTUAL_WIDTH * 0.1f);
		break;
	case TEAM_SELECTION_STAGE_TEAM_2:
	case TEAM_SELECTION_STAGE_CONTROL_2:
		arrow_x = column_2_x + (VIRTUAL_WIDTH * 0.1f);
		break;
	case TEAM_SELECTION_STAGE_INNINGS:
		arrow_x = center_x + (VIRTUAL_WIDTH * 0.1f);
		break;
	}

	if (arrow_x > 0) {
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
}
