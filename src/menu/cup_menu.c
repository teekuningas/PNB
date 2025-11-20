#include "cup_menu.h"
#include "font.h"
#include "render.h"
#include "menu_helpers.h"
#include "cup.h"
#include "platform.h"
#include "rng.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

static void saveCup(StateInfo* stateInfo, int slot);
static void loadCup(StateInfo* stateInfo, int slot);
static void scanSaveSlots(CupMenuState* cupMenuState, StateInfo* stateInfo);

static void initCupViewTreeState(CupViewTreeState* viewTreeState)
{
	viewTreeState->treeCoordinates[0].x = -0.65f;
	viewTreeState->treeCoordinates[0].y = -0.6f;
	viewTreeState->treeCoordinates[1].x = -0.65f;
	viewTreeState->treeCoordinates[1].y = -0.2f;
	viewTreeState->treeCoordinates[2].x = -0.65f;
	viewTreeState->treeCoordinates[2].y =  0.2f;
	viewTreeState->treeCoordinates[3].x = -0.65f;
	viewTreeState->treeCoordinates[3].y =  0.6f;
	viewTreeState->treeCoordinates[4].x =  0.65f;
	viewTreeState->treeCoordinates[4].y = -0.6f;
	viewTreeState->treeCoordinates[5].x =  0.65f;
	viewTreeState->treeCoordinates[5].y = -0.2f;
	viewTreeState->treeCoordinates[6].x =  0.65f;
	viewTreeState->treeCoordinates[6].y =  0.2f;
	viewTreeState->treeCoordinates[7].x =  0.65f;
	viewTreeState->treeCoordinates[7].y =  0.6f;
	viewTreeState->treeCoordinates[8].x = -0.45f;
	viewTreeState->treeCoordinates[8].y = -0.4f;
	viewTreeState->treeCoordinates[9].x = -0.45f;
	viewTreeState->treeCoordinates[9].y =  0.4f;
	viewTreeState->treeCoordinates[10].x =  0.45f;
	viewTreeState->treeCoordinates[10].y = -0.4f;
	viewTreeState->treeCoordinates[11].x =  0.45f;
	viewTreeState->treeCoordinates[11].y =  0.4f;
	viewTreeState->treeCoordinates[12].x = -0.25f;
	viewTreeState->treeCoordinates[12].y = 0.0f;
	viewTreeState->treeCoordinates[13].x =  0.25f;
	viewTreeState->treeCoordinates[13].y = 0.0f;
}

// =============================================================================
// Screen-specific Update Functions
// =============================================================================

static MenuStage updateScreen_Initial(CupMenuState* cupMenuState, StateInfo* stateInfo, const KeyStates* keyStates)
{
	if(keyStates->released[0][KEY_DOWN]) {
		cupMenuState->initial.pointer = (cupMenuState->initial.pointer + 1) % cupMenuState->initial.rem;
	}
	if(keyStates->released[0][KEY_UP]) {
		cupMenuState->initial.pointer = (cupMenuState->initial.pointer - 1 + cupMenuState->initial.rem) % cupMenuState->initial.rem;
	}
	if(keyStates->released[0][KEY_2]) {
		int menu_offset = (stateInfo->cup != NULL) ? 1 : 0;  // Add "Resume" option if cup exists

		if(cupMenuState->initial.pointer == 0 && stateInfo->cup != NULL) {
			// Resume existing cup
			cupMenuState->screen = CUP_MENU_SCREEN_ONGOING;
			cupMenuState->ongoing.pointer = 0;
			if (stateInfo->cup->matches[0].winner_id != CUP_MATCH_NO_WINNER) {
				cupMenuState->ongoing.rem = 3;  // Cup finished menu
			} else {
				cupMenuState->ongoing.rem = 5;  // Cup in progress menu
			}
		} else if(cupMenuState->initial.pointer == (0 + menu_offset)) {
			// New cup
			cupMenuState->screen = CUP_MENU_SCREEN_NEW_CUP;
			cupMenuState->new_cup.new_cup_stage = NEW_CUP_STAGE_TEAM_SELECTION;
			cupMenuState->new_cup.rem = stateInfo->numTeams;
			cupMenuState->new_cup.pointer = 0;
		} else if(cupMenuState->initial.pointer == (1 + menu_offset)) {
			// Load cup
			cupMenuState->screen = CUP_MENU_SCREEN_LOAD_CUP;
			cupMenuState->load_save.rem = 5;
			cupMenuState->load_save.pointer = 0;
			scanSaveSlots(cupMenuState, stateInfo);
		}
	}
	if(keyStates->released[0][KEY_1]) {
		return MENU_STAGE_FRONT;
	}
	return MENU_STAGE_CUP;
}

static MenuStage updateScreen_Ongoing(CupMenuState* cupMenuState, StateInfo* stateInfo, CupMenuOutput* output, unsigned int* rng_seed)
{
	if (stateInfo->cup == NULL) {
		cupMenuState->screen = CUP_MENU_SCREEN_INITIAL;
		return MENU_STAGE_CUP;
	}

	if (stateInfo->cup->matches[0].winner_id != CUP_MATCH_NO_WINNER) {
		if (cupMenuState->ongoing.pointer == 0) { // Schedule
			cupMenuState->screen = CUP_MENU_SCREEN_VIEW_SCHEDULE;
		} else if (cupMenuState->ongoing.pointer == 1) { // Cup tree
			cupMenuState->screen = CUP_MENU_SCREEN_VIEW_TREE;
		} else if (cupMenuState->ongoing.pointer == 2) { // Quit
			return MENU_STAGE_FRONT;
		}
		return MENU_STAGE_CUP;
	}

	if(cupMenuState->ongoing.pointer == 0) { // "Next day"
		int match_indices[8];
		int match_count = 0;
		cup_get_matches_for_day(stateInfo->cup, stateInfo->cup->current_day, match_indices, &match_count);

		int user_match_index = -1;
		for (int i = 0; i < match_count; i++) {
			const CupMatch* match = &stateInfo->cup->matches[match_indices[i]];
			if (match->team_a_id == stateInfo->cup->user_team_id ||
			        match->team_b_id == stateInfo->cup->user_team_id) {
				user_match_index = match_indices[i];
				break;
			}
		}

		if (user_match_index != -1) {
			const CupMatch* match = &stateInfo->cup->matches[user_match_index];

			// Keep bracket structure: team_a on left (team1), team_b on right (team2)
			output->team1 = match->team_a_id;
			output->team2 = match->team_b_id;

			// User controls whichever team is theirs
			if (match->team_a_id == stateInfo->cup->user_team_id) {
				output->team1_control = 0;  // User controls team1 (left side)
				output->team2_control = 2;  // AI controls team2 (right side)
			} else {
				output->team1_control = 2;  // AI controls team1 (left side)
				output->team2_control = 0;  // User controls team2 (right side)
			}

			output->innings = stateInfo->cup->innings_per_period;
			stateInfo->currently_played_cup_match_index = user_match_index;
			return MENU_STAGE_BATTING_ORDER_1;
		} else {
			// No user match today - simulate AI matches and advance to next match day
			for (int i = 0; i < match_count; i++) {
				const CupMatch* match = &stateInfo->cup->matches[match_indices[i]];
				int team_a_wins = seeded_rand(rng_seed, 2);
				TeamID winner = team_a_wins ? match->team_a_id : match->team_b_id;
				cup_update_match_result(stateInfo->cup, match_indices[i], winner);
			}
			cup_advance_to_next_match_day(stateInfo->cup);
		}
	} else if(cupMenuState->ongoing.pointer == 1) {
		cupMenuState->screen = CUP_MENU_SCREEN_VIEW_SCHEDULE;
	} else if(cupMenuState->ongoing.pointer == 2) {
		cupMenuState->screen = CUP_MENU_SCREEN_VIEW_TREE;
	} else if(cupMenuState->ongoing.pointer == 3) {
		cupMenuState->screen = CUP_MENU_SCREEN_SAVE_CUP;
		cupMenuState->load_save.pointer = 0;
		cupMenuState->load_save.rem = 5;
		scanSaveSlots(cupMenuState, stateInfo);
	} else if(cupMenuState->ongoing.pointer == 4) {
		// When quitting an in-progress cup without saving, ask for confirmation
		// For now, we keep the cup in memory so returning to cup menu resumes it
		return MENU_STAGE_FRONT;
	}
	return MENU_STAGE_CUP;
}

static MenuStage updateScreen_NewCup(CupMenuState* cupMenuState, StateInfo* stateInfo, const KeyStates* keyStates, unsigned int* rng_seed)
{
	if (cupMenuState->new_cup.new_cup_stage == NEW_CUP_STAGE_TEAM_SELECTION) {
		if(keyStates->released[0][KEY_DOWN]) cupMenuState->new_cup.pointer = (cupMenuState->new_cup.pointer + 1) % cupMenuState->new_cup.rem;
		if(keyStates->released[0][KEY_UP]) cupMenuState->new_cup.pointer = (cupMenuState->new_cup.pointer - 1 + cupMenuState->new_cup.rem) % cupMenuState->new_cup.rem;
		if(keyStates->released[0][KEY_2]) {
			cupMenuState->new_cup.new_cup_stage = NEW_CUP_STAGE_WINS_TO_ADVANCE;
			cupMenuState->new_cup.team_selection = cupMenuState->new_cup.pointer;
			cupMenuState->new_cup.pointer = 0;
			cupMenuState->new_cup.rem = 3;  // Three options now
		}
		if(keyStates->released[0][KEY_1]) {
			cupMenuState->screen = CUP_MENU_SCREEN_INITIAL;
			cupMenuState->initial.pointer = 0;
			cupMenuState->initial.rem = 2;
		}
	} else if (cupMenuState->new_cup.new_cup_stage == NEW_CUP_STAGE_WINS_TO_ADVANCE) {
		if(keyStates->released[0][KEY_DOWN]) cupMenuState->new_cup.pointer = (cupMenuState->new_cup.pointer + 1) % cupMenuState->new_cup.rem;
		if(keyStates->released[0][KEY_UP]) cupMenuState->new_cup.pointer = (cupMenuState->new_cup.pointer - 1 + cupMenuState->new_cup.rem) % cupMenuState->new_cup.rem;
		if(keyStates->released[0][KEY_2]) {
			// Save wins_to_advance selection: 0="One"->1, 1="Two"->2, 2="Three"->3
			if (cupMenuState->new_cup.pointer == 0) {
				cupMenuState->new_cup.wins_to_advance = 1;
			} else if (cupMenuState->new_cup.pointer == 1) {
				cupMenuState->new_cup.wins_to_advance = 2;
			} else {
				cupMenuState->new_cup.wins_to_advance = 3;
			}
			cupMenuState->new_cup.new_cup_stage = NEW_CUP_STAGE_INNINGS;
			cupMenuState->new_cup.pointer = 0;
			cupMenuState->new_cup.rem = 3;
		}
		if(keyStates->released[0][KEY_1]) {
			cupMenuState->new_cup.new_cup_stage = NEW_CUP_STAGE_TEAM_SELECTION;
			cupMenuState->new_cup.pointer = 0;
			cupMenuState->new_cup.rem = stateInfo->numTeams;
		}
	} else if (cupMenuState->new_cup.new_cup_stage == NEW_CUP_STAGE_INNINGS) {
		if(keyStates->released[0][KEY_DOWN]) cupMenuState->new_cup.pointer = (cupMenuState->new_cup.pointer + 1) % cupMenuState->new_cup.rem;
		if(keyStates->released[0][KEY_UP]) cupMenuState->new_cup.pointer = (cupMenuState->new_cup.pointer - 1 + cupMenuState->new_cup.rem) % cupMenuState->new_cup.rem;
		if(keyStates->released[0][KEY_2]) {
			// Determine innings per period based on user selection
			int innings_per_period;
			if(cupMenuState->new_cup.pointer == 0) innings_per_period = 2;
			else if(cupMenuState->new_cup.pointer == 1) innings_per_period = 4;
			else innings_per_period = 8;

			// Generate initial team IDs (0 to numTeams-1)
			TeamID* initial_team_ids = (TeamID*)malloc(sizeof(TeamID) * stateInfo->numTeams);
			if (initial_team_ids == NULL) {
				perror("Failed to allocate initial_team_ids");
				return MENU_STAGE_CUP;
			}
			for (int i = 0; i < stateInfo->numTeams; ++i) {
				initial_team_ids[i] = i;
			}

			// Randomize tournament bracket with seed from parent
			cup_shuffle_teams(initial_team_ids, stateInfo->numTeams, *rng_seed);

			// Create the new Cup with saved selections
			if (stateInfo->cup != NULL) {
				cup_destroy(stateInfo->cup);
			}
			stateInfo->cup = cup_create(
			                     stateInfo->numTeams,
			                     cupMenuState->new_cup.wins_to_advance,
			                     cupMenuState->new_cup.team_selection,
			                     innings_per_period,
			                     initial_team_ids
			                 );

			free(initial_team_ids); // Free the temporary array

			if (stateInfo->cup == NULL) {
				fprintf(stderr, "Error: Failed to create new cup.\n");
				return MENU_STAGE_CUP;
			}

			// Transition to ongoing cup screen
			cupMenuState->screen = CUP_MENU_SCREEN_ONGOING;
			cupMenuState->ongoing.pointer = 0;
			cupMenuState->ongoing.rem = 5;
		}
		if(keyStates->released[0][KEY_1]) {
			cupMenuState->new_cup.new_cup_stage = NEW_CUP_STAGE_WINS_TO_ADVANCE;
			cupMenuState->new_cup.pointer = 0;
			cupMenuState->new_cup.rem = 2;
		}
	}
	return MENU_STAGE_CUP;
}

static MenuStage updateScreen_LoadCup(CupMenuState* cupMenuState, StateInfo* stateInfo, const KeyStates* keyStates)
{
	if(keyStates->released[0][KEY_DOWN]) cupMenuState->load_save.pointer = (cupMenuState->load_save.pointer + 1) % cupMenuState->load_save.rem;
	if(keyStates->released[0][KEY_UP]) cupMenuState->load_save.pointer = (cupMenuState->load_save.pointer - 1 + cupMenuState->load_save.rem) % cupMenuState->load_save.rem;
	if(keyStates->released[0][KEY_1]) {
		cupMenuState->screen = CUP_MENU_SCREEN_INITIAL;
		cupMenuState->initial.pointer = 1;
		cupMenuState->initial.rem = 2;
	}
	if (keyStates->released[0][KEY_2]) {
		loadCup(stateInfo, cupMenuState->load_save.pointer);
		if (stateInfo->cup != NULL) {
			cupMenuState->screen = CUP_MENU_SCREEN_ONGOING;
			cupMenuState->ongoing.pointer = 0;
			cupMenuState->ongoing.rem = 5;
		}
	}
	return MENU_STAGE_CUP;
}

static MenuStage updateScreen_SaveCup(CupMenuState* cupMenuState, StateInfo* stateInfo, const KeyStates* keyStates)
{
	if(keyStates->released[0][KEY_DOWN]) cupMenuState->load_save.pointer = (cupMenuState->load_save.pointer + 1) % cupMenuState->load_save.rem;
	if(keyStates->released[0][KEY_UP]) cupMenuState->load_save.pointer = (cupMenuState->load_save.pointer - 1 + cupMenuState->load_save.rem) % cupMenuState->load_save.rem;
	if(keyStates->released[0][KEY_1]) {
		cupMenuState->screen = CUP_MENU_SCREEN_ONGOING;
		cupMenuState->ongoing.pointer = 3;
		cupMenuState->ongoing.rem = 5;
	}
	if (keyStates->released[0][KEY_2]) {
		saveCup(stateInfo, cupMenuState->load_save.pointer);
		scanSaveSlots(cupMenuState, stateInfo);  // Refresh to show save succeeded
	}
	return MENU_STAGE_CUP;
}

static MenuStage updateScreen_View(CupMenuState* cupMenuState, const KeyStates* keyStates)
{
	if(keyStates->released[0][KEY_2] || keyStates->released[0][KEY_1]) {
		cupMenuState->screen = CUP_MENU_SCREEN_ONGOING;
	}
	return MENU_STAGE_CUP;
}


// =============================================================================
// Screen-specific Draw Functions
// =============================================================================

static void drawScreen_Initial(const CupInitialState* initialState, const StateInfo* stateInfo,
                               const RenderState* rs, ResourceManager* rm)
{
	const float center_x = VIRTUAL_WIDTH / 2.0f;
	const float title_y = VIRTUAL_HEIGHT * 0.1f;
	const float menu_start_y = VIRTUAL_HEIGHT * 0.4f;
	const float menu_spacing = VIRTUAL_HEIGHT * 0.1f;
	const float title_fontsize = 120.0f;
	const float menu_fontsize = 60.0f;
	const float arrow_size = 80.0f;

	// --- Draw Images ---
	// Batter
	float imgHeight = VIRTUAL_HEIGHT * 0.8f;
	float yPos = VIRTUAL_HEIGHT - imgHeight - (VIRTUAL_HEIGHT * 0.10f);
	float batterImgWidth = imgHeight * 0.5f;
	float batterX = VIRTUAL_WIDTH * 0.1f;
	draw_texture_2d(resource_manager_get_texture(rm, "data/textures/batter.tga"), batterX, yPos, batterImgWidth, imgHeight);

	// Catcher
	float catcherImgWidth = imgHeight * 0.8f;
	float catcherX = VIRTUAL_WIDTH * 1.05f - catcherImgWidth;
	draw_texture_2d(resource_manager_get_texture(rm, "data/textures/catcher.tga"), catcherX, yPos, catcherImgWidth, imgHeight);

	// --- Draw Text ---
	draw_text_2d("P N B", center_x, title_y, title_fontsize, TEXT_ALIGN_CENTER, rs);

	int menu_line = 0;
	if (stateInfo->cup != NULL) {
		draw_text_2d("Resume", center_x, menu_start_y + (menu_line++ * menu_spacing), menu_fontsize, TEXT_ALIGN_CENTER, rs);
	}
	draw_text_2d("New", center_x, menu_start_y + (menu_line++ * menu_spacing), menu_fontsize, TEXT_ALIGN_CENTER, rs);
	draw_text_2d("Load", center_x, menu_start_y + (menu_line++ * menu_spacing), menu_fontsize, TEXT_ALIGN_CENTER, rs);

	// --- Draw Arrow ---
	float arrow_x = center_x + 180.0f;
	float arrow_y = menu_start_y + (initialState->pointer * menu_spacing) - (arrow_size - menu_fontsize) / 2.0f;
	draw_texture_2d(resource_manager_get_texture(rm, "data/textures/arrow.tga"), arrow_x, arrow_y, arrow_size, arrow_size);
}

static void drawScreen_Ongoing(const CupOngoingState* ongoingState, const StateInfo* stateInfo, const RenderState* rs, ResourceManager* rm)
{
	const float center_x = VIRTUAL_WIDTH / 2.0f;
	const float title_y = VIRTUAL_HEIGHT * 0.1f;
	const float menu_start_y = VIRTUAL_HEIGHT * 0.3f;
	const float menu_spacing = VIRTUAL_HEIGHT * 0.08f;
	const float title_fontsize = 80.0f;
	const float menu_fontsize = 40.0f;
	const float arrow_size = 60.0f;

	draw_text_2d("Cup Menu", center_x, title_y, title_fontsize, TEXT_ALIGN_CENTER, rs);

	if (stateInfo->cup != NULL && stateInfo->cup->matches[0].winner_id != CUP_MATCH_NO_WINNER) {
		// Cup is over, show finished cup menu
		draw_text_2d("Schedule", center_x, menu_start_y, menu_fontsize, TEXT_ALIGN_CENTER, rs);
		draw_text_2d("Cup tree", center_x, menu_start_y + menu_spacing, menu_fontsize, TEXT_ALIGN_CENTER, rs);
		draw_text_2d("Quit", center_x, menu_start_y + 2 * menu_spacing, menu_fontsize, TEXT_ALIGN_CENTER, rs);

		char buffer[100];
		sprintf(buffer, "%s has won the cup", stateInfo->teamData[stateInfo->cup->matches[0].winner_id].name);
		draw_text_2d(buffer, center_x, menu_start_y + 4 * menu_spacing, menu_fontsize, TEXT_ALIGN_CENTER, rs);
	} else {
		// Cup is in progress, show normal menu
		draw_text_2d("Next day", center_x, menu_start_y, menu_fontsize, TEXT_ALIGN_CENTER, rs);
		draw_text_2d("Schedule", center_x, menu_start_y + menu_spacing, menu_fontsize, TEXT_ALIGN_CENTER, rs);
		draw_text_2d("Cup tree", center_x, menu_start_y + 2 * menu_spacing, menu_fontsize, TEXT_ALIGN_CENTER, rs);
		draw_text_2d("Save", center_x, menu_start_y + 3 * menu_spacing, menu_fontsize, TEXT_ALIGN_CENTER, rs);
		draw_text_2d("Quit", center_x, menu_start_y + 4 * menu_spacing, menu_fontsize, TEXT_ALIGN_CENTER, rs);
	}

	// --- Draw Arrow ---
	float arrow_x = center_x + 200.0f;
	float arrow_y = menu_start_y + (ongoingState->pointer * menu_spacing) - (arrow_size - menu_fontsize) / 2.0f;
	draw_texture_2d(resource_manager_get_texture(rm, "data/textures/arrow.tga"), arrow_x, arrow_y, arrow_size, arrow_size);
}

static void drawScreen_NewCup(const CupNewState* newCupState, const StateInfo* stateInfo, const RenderState* rs, ResourceManager* rm)
{
	const float center_x = VIRTUAL_WIDTH / 2.0f;
	const float title_y = VIRTUAL_HEIGHT * 0.1f;
	const float menu_start_y = VIRTUAL_HEIGHT * 0.25f;
	const float menu_spacing = VIRTUAL_HEIGHT * 0.07f;
	const float title_fontsize = 60.0f;
	const float menu_fontsize = 40.0f;
	const float arrow_size = 60.0f;

	if (newCupState->new_cup_stage == NEW_CUP_STAGE_TEAM_SELECTION) {
		draw_text_2d("Select team", center_x, title_y, title_fontsize, TEXT_ALIGN_CENTER, rs);
		for(int i = 0; i < stateInfo->numTeams; i++) {
			draw_text_2d(stateInfo->teamData[i].name, center_x, menu_start_y + i * menu_spacing, menu_fontsize, TEXT_ALIGN_CENTER, rs);
		}
	} else if (newCupState->new_cup_stage == NEW_CUP_STAGE_WINS_TO_ADVANCE) {
		draw_text_2d("How many wins to move forward", center_x, title_y, title_fontsize, TEXT_ALIGN_CENTER, rs);
		draw_text_2d("One", center_x, menu_start_y, menu_fontsize, TEXT_ALIGN_CENTER, rs);
		draw_text_2d("Two", center_x, menu_start_y + menu_spacing, menu_fontsize, TEXT_ALIGN_CENTER, rs);
		draw_text_2d("Three", center_x, menu_start_y + 2 * menu_spacing, menu_fontsize, TEXT_ALIGN_CENTER, rs);
	} else if (newCupState->new_cup_stage == NEW_CUP_STAGE_INNINGS) {
		draw_text_2d("How many innings in period", center_x, title_y, title_fontsize, TEXT_ALIGN_CENTER, rs);
		draw_text_2d("1", center_x, menu_start_y, menu_fontsize, TEXT_ALIGN_CENTER, rs);
		draw_text_2d("2", center_x, menu_start_y + menu_spacing, menu_fontsize, TEXT_ALIGN_CENTER, rs);
		draw_text_2d("4", center_x, menu_start_y + 2 * menu_spacing, menu_fontsize, TEXT_ALIGN_CENTER, rs);
	}

	// --- Draw Arrow ---
	float arrow_x = center_x + 250.0f;
	float arrow_y = menu_start_y + (newCupState->pointer * menu_spacing) - (arrow_size - menu_fontsize) / 2.0f;
	draw_texture_2d(resource_manager_get_texture(rm, "data/textures/arrow.tga"), arrow_x, arrow_y, arrow_size, arrow_size);
}

static void drawScreen_ViewTree(const CupViewTreeState* viewTreeState, const StateInfo* stateInfo, const RenderState* rs, ResourceManager* rm)
{
	const float text_size = 22.0f;
	const float slot_width = VIRTUAL_WIDTH * 0.22f;
	const float slot_height = VIRTUAL_HEIGHT * 0.12f;
	const GLuint slot_texture = resource_manager_get_texture(rm, "data/textures/cup_tree_slot.tga");

	if (stateInfo->cup == NULL) {
		draw_text_2d("No cup in progress.", VIRTUAL_WIDTH / 2.0f, VIRTUAL_HEIGHT / 2.0f, 40.0f, TEXT_ALIGN_CENTER, rs);
		return;
	}

	// NOTE: This visualization is designed for 8-team tournaments only.
	// The 14 display slots are hardcoded positions that map to:
	//   - Slots 0-7:  Quarter-final teams (bottom of bracket)
	//   - Slots 8-11: Semi-final teams (middle of bracket)
	//   - Slots 12-13: Final teams (top of bracket)
	// Match indices in the Cup structure follow binary heap ordering:
	//   - Match 0: Final
	//   - Matches 1-2: Semi-finals
	//   - Matches 3-6: Quarter-finals
	int num_slots_to_draw = stateInfo->cup->num_matches * 2;

	// --- Draw Slots ---
	for(int i = 0; i < num_slots_to_draw; i++) {
		// Convert normalized coordinates to virtual screen coordinates
		float x = (viewTreeState->treeCoordinates[i].x + 1.0f) / 2.0f * VIRTUAL_WIDTH - (slot_width / 2.0f);
		float y = (1.0f - viewTreeState->treeCoordinates[i].y) / 2.0f * VIRTUAL_HEIGHT - (slot_height / 2.0f);
		draw_texture_2d(slot_texture, x, y, slot_width, slot_height);
	}

	// --- Draw Text ---
	// Map Cup match indices to display slot indices for visualization
	// This hardcoded mapping is specific to 8-team tournaments
	for(int i = 0; i < stateInfo->cup->num_matches; i++) {
		const CupMatch* match = &stateInfo->cup->matches[i];

		int slot_a_idx, slot_b_idx;

		if (i == 0) { // Final match -> top bracket slots
			slot_a_idx = 12;
			slot_b_idx = 13;
		} else if (i >= 1 && i <= 2) { // Semi-finals -> middle bracket slots
			slot_a_idx = 8 + (i-1)*2;
			slot_b_idx = 9 + (i-1)*2;
		} else if (i >= 3 && i <= 6) { // Quarter-finals -> bottom bracket slots
			slot_a_idx = (i-3)*2;
			slot_b_idx = (i-3)*2 + 1;
		} else {
			continue; // Skip invalid match indices
		}


		if (match->team_a_id != -1) {
			float text_center_x = (viewTreeState->treeCoordinates[slot_a_idx].x + 1.0f) / 2.0f * VIRTUAL_WIDTH;
			float text_center_y = (1.0f - viewTreeState->treeCoordinates[slot_a_idx].y) / 2.0f * VIRTUAL_HEIGHT;
			float team_name_x = text_center_x - slot_width * 0.15f;
			float text_y = text_center_y - text_size / 2.0f;
			draw_text_2d(stateInfo->teamData[match->team_a_id].name, team_name_x, text_y, text_size, TEXT_ALIGN_CENTER, rs);
			char wins_str[8];
			sprintf(wins_str, "%d", match->wins_a);
			float wins_x = text_center_x + slot_width * 0.35f;
			draw_text_2d(wins_str, wins_x, text_y, text_size, TEXT_ALIGN_LEFT, rs);
		}
		if (match->team_b_id != -1) {
			float text_center_x = (viewTreeState->treeCoordinates[slot_b_idx].x + 1.0f) / 2.0f * VIRTUAL_WIDTH;
			float text_center_y = (1.0f - viewTreeState->treeCoordinates[slot_b_idx].y) / 2.0f * VIRTUAL_HEIGHT;
			float team_name_x = text_center_x - slot_width * 0.15f;
			float text_y = text_center_y - text_size / 2.0f;
			draw_text_2d(stateInfo->teamData[match->team_b_id].name, team_name_x, text_y, text_size, TEXT_ALIGN_CENTER, rs);
			char wins_str[8];
			sprintf(wins_str, "%d", match->wins_b);
			float wins_x = text_center_x + slot_width * 0.35f;
			draw_text_2d(wins_str, wins_x, text_y, text_size, TEXT_ALIGN_LEFT, rs);
		}
	}
}

static void drawScreen_ViewSchedule(const CupMenuState* cupMenuState, const StateInfo* stateInfo, const RenderState* rs, ResourceManager* rm)
{
	const float center_x = VIRTUAL_WIDTH / 2.0f;
	const float title_y = VIRTUAL_HEIGHT * 0.1f;
	const float list_start_y = VIRTUAL_HEIGHT * 0.3f;
	const float list_spacing = VIRTUAL_HEIGHT * 0.1f;
	const float title_fontsize = 60.0f;
	const float text_fontsize = 40.0f;
	const float team_name_offset = VIRTUAL_WIDTH * 0.2f;

	draw_text_2d("Schedule", center_x, title_y, title_fontsize, TEXT_ALIGN_CENTER, rs);

	if (stateInfo->cup == NULL) {
		draw_text_2d("No cup in progress.", center_x, VIRTUAL_HEIGHT / 2.0f, 40.0f, TEXT_ALIGN_CENTER, rs);
		return;
	}

	// Get matches scheduled for today
	int match_indices[8];
	int count = 0;
	cup_get_matches_for_day(stateInfo->cup, stateInfo->cup->current_day, match_indices, &count);

	for(int i = 0; i < count; i++) {
		const CupMatch* match = &stateInfo->cup->matches[match_indices[i]];
		if(match->team_a_id != -1 && match->team_b_id != -1) {
			float current_y = list_start_y + i * list_spacing;
			draw_text_2d(stateInfo->teamData[match->team_a_id].name, center_x - team_name_offset, current_y, text_fontsize, TEXT_ALIGN_CENTER, rs);
			draw_text_2d("-", center_x, current_y, text_fontsize, TEXT_ALIGN_CENTER, rs);
			draw_text_2d(stateInfo->teamData[match->team_b_id].name, center_x + team_name_offset, current_y, text_fontsize, TEXT_ALIGN_CENTER, rs);
		}
	}

	// Show current day
	char day_text[32];
	sprintf(day_text, "Day %d", stateInfo->cup->current_day + 1);
	draw_text_2d(day_text, center_x, title_y + 100.0f, 30.0f, TEXT_ALIGN_CENTER, rs);
}

static void drawScreen_LoadOrSaveCup(const CupLoadSaveState* loadSaveState, const CupMenuScreen screen,
                                     const CupMenuState* cupMenuState, const StateInfo* stateInfo,
                                     const RenderState* rs, ResourceManager* rm)
{
	const float center_x = VIRTUAL_WIDTH / 2.0f;
	const float title_y = VIRTUAL_HEIGHT * 0.1f;
	const float menu_start_y = VIRTUAL_HEIGHT * 0.25f;
	const float menu_spacing = VIRTUAL_HEIGHT * 0.15f;
	const float title_fontsize = 80.0f;
	const float main_fontsize = 40.0f;
	const float detail_fontsize = 30.0f;
	const float arrow_size = 60.0f;

	// --- Draw Title ---
	if(screen == CUP_MENU_SCREEN_LOAD_CUP) {
		draw_text_2d("Load Cup", center_x, title_y, title_fontsize, TEXT_ALIGN_CENTER, rs);
	} else {
		draw_text_2d("Save Cup", center_x, title_y, title_fontsize, TEXT_ALIGN_CENTER, rs);
	}

	// --- Draw Save Slots ---
	for(int i = 0; i < 5; i++) {
		float current_y = menu_start_y + i * menu_spacing;

		const SaveSlotInfo* slot_info = &cupMenuState->save_slots[i];

		if(slot_info->exists) {
			// Draw Team Name
			char* team_name = stateInfo->teamData[slot_info->user_team_id].name;
			draw_text_2d(team_name, center_x, current_y, main_fontsize, TEXT_ALIGN_CENTER, rs);

			// --- Draw Details ---
			char details[100];
			if (slot_info->is_finished) {
				sprintf(details, "Day %d - Cup finished", slot_info->current_day + 1);
			} else if (slot_info->opponent_team_id != -1) {
				sprintf(details, "Day %d - Next: %s", slot_info->current_day + 1,
				        stateInfo->teamData[slot_info->opponent_team_id].name);
			} else {
				sprintf(details, "Day %d - In progress", slot_info->current_day + 1);
			}
			draw_text_2d(details, center_x, current_y + main_fontsize * 1.2f, detail_fontsize, TEXT_ALIGN_CENTER, rs);
		} else {
			draw_text_2d("Empty slot", center_x, current_y, main_fontsize, TEXT_ALIGN_CENTER, rs);
		}
	}

	// --- Draw Arrow ---
	float arrow_x = center_x + 400.0f;
	float arrow_y = menu_start_y + (loadSaveState->pointer * menu_spacing) - (arrow_size - main_fontsize) / 2.0f;
	draw_texture_2d(resource_manager_get_texture(rm, "data/textures/arrow.tga"), arrow_x, arrow_y, arrow_size, arrow_size);
}

static void drawScreen_EndCredits(const CreditsMenuState* creditsState, const RenderState* rs, ResourceManager* rm)
{
	const float center_x = VIRTUAL_WIDTH / 2.0f;
	const float title_y = VIRTUAL_HEIGHT * 0.1f;
	const float title_fontsize = 60.0f;
	const float text_fontsize = 30.0f;

	// --- Draw Trophy ---
	const float trophy_size = VIRTUAL_WIDTH * 0.3f;
	const float trophy_x = center_x - trophy_size / 2.0f;
	const float trophy_y = VIRTUAL_HEIGHT * 0.20f;
	draw_texture_2d(resource_manager_get_texture(rm, "data/textures/menu_trophy.tga"), trophy_x, trophy_y, trophy_size, trophy_size);

	// --- Draw Text ---
	draw_text_2d("WE HAVE A CHAMPION!", center_x, title_y, title_fontsize, TEXT_ALIGN_CENTER, rs);

	const char* credits_text = "SPECIAL THANKS TO JUUSO HEINILA, PEKKA HEINILA, PETRI ANTTILA, MATTI PITKANEN, VILLE VILJANMAA, PETRI MIKOLA, TUOMAS NURMELA, AND OTHERS..";
	draw_text_2d(credits_text, creditsState->creditsScrollX, VIRTUAL_HEIGHT * 0.8f, text_fontsize, TEXT_ALIGN_LEFT, rs);
}


// =============================================================================
// Main Public Functions
// =============================================================================

static void scanSaveSlots(CupMenuState* cupMenuState, StateInfo* stateInfo)
{
	for (int i = 0; i < 5; i++) {
		cupMenuState->save_slots[i].exists = 0;
		cupMenuState->save_slots[i].user_team_id = -1;
		cupMenuState->save_slots[i].opponent_team_id = -1;
		cupMenuState->save_slots[i].is_finished = 0;
		cupMenuState->save_slots[i].current_day = 0;

		char filename[PATH_MAX];
		if (platform_get_save_path(filename, sizeof(filename), i) != 0) {
			continue;
		}

		Cup* saved_cup = cup_load(filename);
		if (saved_cup != NULL) {
			cupMenuState->save_slots[i].exists = 1;
			cupMenuState->save_slots[i].user_team_id = saved_cup->user_team_id;
			cupMenuState->save_slots[i].current_day = saved_cup->current_day;

			if (saved_cup->matches[0].winner_id != CUP_MATCH_NO_WINNER) {
				cupMenuState->save_slots[i].is_finished = 1;
			} else {
				int user_match_index = cup_get_user_match_index(saved_cup);
				if (user_match_index != -1) {
					const CupMatch* match = &saved_cup->matches[user_match_index];
					TeamID opponent_id = (match->team_a_id == saved_cup->user_team_id)
					                     ? match->team_b_id : match->team_a_id;
					cupMenuState->save_slots[i].opponent_team_id = opponent_id;
				}
			}

			cup_destroy(saved_cup);
		}
	}
}

static void saveCup(StateInfo* stateInfo, int slot)
{
	if (stateInfo->cup == NULL) {
		printf("No cup in progress to save.\n");
		return;
	}

	if (platform_ensure_save_dir() != 0) {
		fprintf(stderr, "Error: Could not create save directory.\n");
		return;
	}

	char filename[PATH_MAX];
	if (platform_get_save_path(filename, sizeof(filename), slot) != 0) {
		fprintf(stderr, "Error: Could not get save path.\n");
		return;
	}

	if (cup_save(stateInfo->cup, filename) == 0) {
		printf("Cup saved to slot %d (%s).\n", slot, filename);
	} else {
		printf("Failed to save cup to slot %d.\n", slot);
	}
}

static void loadCup(StateInfo* stateInfo, int slot)
{
	char filename[PATH_MAX];
	if (platform_get_save_path(filename, sizeof(filename), slot) != 0) {
		fprintf(stderr, "Error: Could not get save path.\n");
		return;
	}

	Cup* loaded_cup = cup_load(filename);
	if (loaded_cup != NULL) {
		if (stateInfo->cup != NULL) {
			cup_destroy(stateInfo->cup);
		}
		stateInfo->cup = loaded_cup;
		printf("Cup loaded from slot %d (%s).\n", slot, filename);
	} else {
		printf("Failed to load cup from slot %d, or slot is empty.\n", slot);
	}
}

void initCupMenu(CupMenuState* cupMenuState, StateInfo* stateInfo, unsigned int* rng_seed)
{
	// Initialize all sub-states to a known default
	memset(&cupMenuState->initial, 0, sizeof(CupInitialState));
	memset(&cupMenuState->ongoing, 0, sizeof(CupOngoingState));
	memset(&cupMenuState->new_cup, 0, sizeof(CupNewState));
	memset(&cupMenuState->load_save, 0, sizeof(CupLoadSaveState));
	memset(&cupMenuState->credits_menu, 0, sizeof(CreditsMenuState));

	// Initialize the tree coordinates, as they are constant layout data
	initCupViewTreeState(&cupMenuState->view_tree);

	// Always start at initial screen (Resume/New/Load menu)
	cupMenuState->screen = CUP_MENU_SCREEN_INITIAL;
	cupMenuState->initial.pointer = 0;
	// Set rem based on whether cup exists: 3 options (Resume/New/Load) or 2 (New/Load)
	cupMenuState->initial.rem = (stateInfo->cup != NULL) ? 3 : 2;
}
MenuStage updateCupMenu(
    CupMenuState* cupMenuState,
    StateInfo* stateInfo,
    const KeyStates* keyStates,
    CupMenuOutput* output,
    unsigned int* rng_seed
)
{
	switch (cupMenuState->screen) {
	case CUP_MENU_SCREEN_INITIAL:
		return updateScreen_Initial(cupMenuState, stateInfo, keyStates);
	case CUP_MENU_SCREEN_ONGOING: {
		// This screen is the only one that can decide to start a game,
		// so it's the only one that needs the 'output' struct.
		// We also handle the common up/down navigation here.
		if(keyStates->released[0][KEY_DOWN]) cupMenuState->ongoing.pointer = (cupMenuState->ongoing.pointer + 1) % cupMenuState->ongoing.rem;
		if(keyStates->released[0][KEY_UP]) cupMenuState->ongoing.pointer = (cupMenuState->ongoing.pointer - 1 + cupMenuState->ongoing.rem) % cupMenuState->ongoing.rem;
		if(keyStates->released[0][KEY_2]) return updateScreen_Ongoing(cupMenuState, stateInfo, output, rng_seed);
		return MENU_STAGE_CUP;
	}
	case CUP_MENU_SCREEN_NEW_CUP:
		return updateScreen_NewCup(cupMenuState, stateInfo, keyStates, rng_seed);
	case CUP_MENU_SCREEN_LOAD_CUP:
		return updateScreen_LoadCup(cupMenuState, stateInfo, keyStates);
	case CUP_MENU_SCREEN_SAVE_CUP:
		return updateScreen_SaveCup(cupMenuState, stateInfo, keyStates);
	case CUP_MENU_SCREEN_VIEW_SCHEDULE:
	case CUP_MENU_SCREEN_VIEW_TREE:
		return updateScreen_View(cupMenuState, keyStates);
	case CUP_MENU_SCREEN_END_CREDITS: {
		const char* credits_text = "SPECIAL THANKS TO JUUSO HEINILA, PEKKA HEINILA, PETRI ANTTILA, MATTI PITKANEN, VILLE VILJANMAA, PETRI MIKOLA, TUOMAS NURMELA, AND OTHERS..";
		const float text_fontsize = 30.0f;
		float text_width = getTextWidth2D(credits_text, strlen(credits_text), text_fontsize);
		cupMenuState->credits_menu.creditsScrollX -= 1.0f;
		if (cupMenuState->credits_menu.creditsScrollX < -text_width) {
			cupMenuState->credits_menu.creditsScrollX = VIRTUAL_WIDTH;
		}
		return updateScreen_View(cupMenuState, keyStates);
	}
	}
	return MENU_STAGE_CUP;
}


void drawCupMenu(const CupMenuState* cupMenuState, const StateInfo* stateInfo, const RenderState* rs, ResourceManager* rm)
{
	begin_2d_render(rs);
	drawMenuLayout2D(rm, rs);

	switch (cupMenuState->screen) {
	case CUP_MENU_SCREEN_INITIAL:
		drawScreen_Initial(&cupMenuState->initial, stateInfo, rs, rm);
		break;
	case CUP_MENU_SCREEN_ONGOING:
		drawScreen_Ongoing(&cupMenuState->ongoing, stateInfo, rs, rm);
		break;
	case CUP_MENU_SCREEN_NEW_CUP:
		drawScreen_NewCup(&cupMenuState->new_cup, stateInfo, rs, rm);
		break;
	case CUP_MENU_SCREEN_VIEW_TREE:
		drawScreen_ViewTree(&cupMenuState->view_tree, stateInfo, rs, rm);
		break;
	case CUP_MENU_SCREEN_VIEW_SCHEDULE:
		drawScreen_ViewSchedule(cupMenuState, stateInfo, rs, rm);
		break;
	case CUP_MENU_SCREEN_LOAD_CUP:
	case CUP_MENU_SCREEN_SAVE_CUP:
		drawScreen_LoadOrSaveCup(&cupMenuState->load_save, cupMenuState->screen, cupMenuState, stateInfo, rs, rm);
		break;
	case CUP_MENU_SCREEN_END_CREDITS:
		drawScreen_EndCredits(&cupMenuState->credits_menu, rs, rm);
		break;
	}
}
