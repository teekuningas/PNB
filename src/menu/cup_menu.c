#include "cup_menu.h"
#include "font.h"
#include "render.h"
#include "menu_helpers.h"
#include "cup.h"
#include "save.h"
#include <string.h>



static int refreshLoadCups(StateInfo* stateInfo);
static void saveCup(StateInfo* stateInfo, int slot);
static void loadCup(StateInfo* stateInfo, int slot);

static void initCupViewTreeState(CupViewTreeState* viewTreeState)
{
	// set locations for cup tree view.
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

static void initializeNewCup(TournamentState* tournamentState, int inningsPointer, int userTeamSelection)
{
	int i;

	// Set inning count based on selection
	if(inningsPointer == 0) tournamentState->cupInfo.inningCount = 2;
	else if(inningsPointer == 1) tournamentState->cupInfo.inningCount = 4;
	else if(inningsPointer == 2) tournamentState->cupInfo.inningCount = 8;

	// Initialize tournament state
	tournamentState->cupInfo.userTeamIndexInTree = 0;
	tournamentState->cupInfo.winnerIndex = -1;
	tournamentState->cupInfo.dayCount = 0;

	// Clear cup tree
	for(i = 0; i < SLOT_COUNT; i++) {
		tournamentState->cupInfo.cupTeamIndexTree[i] = -1;
	}

	// Randomly assign 8 teams to the cup tree
	i = 0;
	while(i < 8) {
		int random = rand()%8;
		if(tournamentState->cupInfo.cupTeamIndexTree[random] == -1) {
			tournamentState->cupInfo.cupTeamIndexTree[random] = i;
			if(i == userTeamSelection) {
				tournamentState->cupInfo.userTeamIndexInTree = random;
			}
			i++;
		}
	}

	// Initialize slot wins
	for(i = 0; i < SLOT_COUNT; i++) {
		tournamentState->cupInfo.slotWins[i] = 0;
	}

	// Set up initial schedule (first round matches)
	for(i = 0; i < 4; i++) {
		tournamentState->cupInfo.schedule[i][0] = i*2;
		tournamentState->cupInfo.schedule[i][1] = i*2+1;
	}
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
		if(cupMenuState->initial.pointer == 0) {
			cupMenuState->screen = CUP_MENU_SCREEN_NEW_CUP;
			cupMenuState->new_cup.new_cup_stage = NEW_CUP_STAGE_TEAM_SELECTION;
			cupMenuState->new_cup.rem = stateInfo->numTeams;
			cupMenuState->new_cup.pointer = 0;
		} else if(cupMenuState->initial.pointer == 1) {
			cupMenuState->screen = CUP_MENU_SCREEN_LOAD_CUP;
			cupMenuState->load_save.rem = 5;
			cupMenuState->load_save.pointer = 0;
		}
	}
	if(keyStates->released[0][KEY_1]) {
		return MENU_STAGE_FRONT;
	}
	return MENU_STAGE_CUP;
}

static MenuStage updateScreen_Ongoing(CupMenuState* cupMenuState, StateInfo* stateInfo, CupMenuOutput* output)
{
	if (stateInfo->tournamentState->cupInfo.winnerIndex != -1) {
		// Cup is over, handle finished cup menu
		if (cupMenuState->ongoing.pointer == 0) { // Schedule
			cupMenuState->screen = CUP_MENU_SCREEN_VIEW_SCHEDULE;
		} else if (cupMenuState->ongoing.pointer == 1) { // Cup tree
			cupMenuState->screen = CUP_MENU_SCREEN_VIEW_TREE;
		} else if (cupMenuState->ongoing.pointer == 2) { // Quit
			return MENU_STAGE_FRONT;
		}
		return MENU_STAGE_CUP;
	}

	// Cup is in progress, handle normal menu
	if(cupMenuState->ongoing.pointer == 0) { // "Next day"
		int userTeamIndex = -1;
		int userPosition = 0;
		int opponentTeamIndex = -1;
		int i, j;
		// find out if there is a game for human player.
		for(i = 0; i < 4; i++) {
			for(j = 0; j < 2; j++) {
				if(stateInfo->tournamentState->cupInfo.schedule[i][j] == stateInfo->tournamentState->cupInfo.userTeamIndexInTree) {
					if(j == 0) {
						userTeamIndex = stateInfo->tournamentState->cupInfo.cupTeamIndexTree[stateInfo->tournamentState->cupInfo.schedule[i][0]];
						opponentTeamIndex = stateInfo->tournamentState->cupInfo.cupTeamIndexTree[stateInfo->tournamentState->cupInfo.schedule[i][1]];
						userPosition = 0;
					} else {
						userTeamIndex = stateInfo->tournamentState->cupInfo.cupTeamIndexTree[stateInfo->tournamentState->cupInfo.schedule[i][1]];
						opponentTeamIndex = stateInfo->tournamentState->cupInfo.cupTeamIndexTree[stateInfo->tournamentState->cupInfo.schedule[i][0]];
						userPosition = 1;
					}
				}
			}
		}
		stateInfo->tournamentState->cupInfo.dayCount++;
		// if there is, we proceed to the match
		if(userTeamIndex != -1) {
			if(userPosition == 0) {
				output->team1 = userTeamIndex;
				output->team2 = opponentTeamIndex;
				output->team1_control = 0;
				output->team2_control = 2;
			} else {
				output->team2 = userTeamIndex;
				output->team1 = opponentTeamIndex;
				output->team2_control = 0;
				output->team1_control = 2;
			}
			output->innings = stateInfo->tournamentState->cupInfo.inningCount;
			return MENU_STAGE_BATTING_ORDER_1;
		} else {
			// otherwise we simulate the day right away.
			updateCupTreeAfterDay(stateInfo->tournamentState, stateInfo, -1, 0);
			updateSchedule(stateInfo->tournamentState, stateInfo);
		}
	} else if(cupMenuState->ongoing.pointer == 1) {
		cupMenuState->screen = CUP_MENU_SCREEN_VIEW_SCHEDULE;
	} else if(cupMenuState->ongoing.pointer == 2) {
		cupMenuState->screen = CUP_MENU_SCREEN_VIEW_TREE;
	} else if(cupMenuState->ongoing.pointer == 3) {
		cupMenuState->screen = CUP_MENU_SCREEN_SAVE_CUP;
		cupMenuState->load_save.pointer = 0;
		cupMenuState->load_save.rem = 5;
	} else if(cupMenuState->ongoing.pointer == 4) {
		return MENU_STAGE_FRONT;
	}
	return MENU_STAGE_CUP;
}

static MenuStage updateScreen_NewCup(CupMenuState* cupMenuState, StateInfo* stateInfo, const KeyStates* keyStates)
{
	if (cupMenuState->new_cup.new_cup_stage == NEW_CUP_STAGE_TEAM_SELECTION) {
		if(keyStates->released[0][KEY_DOWN]) cupMenuState->new_cup.pointer = (cupMenuState->new_cup.pointer + 1) % cupMenuState->new_cup.rem;
		if(keyStates->released[0][KEY_UP]) cupMenuState->new_cup.pointer = (cupMenuState->new_cup.pointer - 1 + cupMenuState->new_cup.rem) % cupMenuState->new_cup.rem;
		if(keyStates->released[0][KEY_2]) {
			cupMenuState->new_cup.new_cup_stage = NEW_CUP_STAGE_WINS_TO_ADVANCE;
			cupMenuState->new_cup.team_selection = cupMenuState->new_cup.pointer;
			cupMenuState->new_cup.pointer = 0;
			cupMenuState->new_cup.rem = 2;
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
			stateInfo->tournamentState->cupInfo.gameStructure = cupMenuState->new_cup.pointer;
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
			// Initialize the new tournament
			initializeNewCup(
			    stateInfo->tournamentState,
			    cupMenuState->new_cup.pointer,
			    cupMenuState->new_cup.team_selection
			);

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
		if (stateInfo->tournamentState->saveData[cupMenuState->load_save.pointer].userTeamIndexInTree != -1) {
			loadCup(stateInfo, cupMenuState->load_save.pointer);
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

static void drawScreen_Initial(const CupInitialState* initialState, const RenderState* rs, ResourceManager* rm)
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
	draw_text_2d("New cup", center_x, menu_start_y, menu_fontsize, TEXT_ALIGN_CENTER, rs);
	draw_text_2d("Load cup", center_x, menu_start_y + menu_spacing, menu_fontsize, TEXT_ALIGN_CENTER, rs);

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

	if (stateInfo->tournamentState->cupInfo.winnerIndex != -1) {
		// Cup is over, show finished cup menu
		draw_text_2d("Schedule", center_x, menu_start_y, menu_fontsize, TEXT_ALIGN_CENTER, rs);
		draw_text_2d("Cup tree", center_x, menu_start_y + menu_spacing, menu_fontsize, TEXT_ALIGN_CENTER, rs);
		draw_text_2d("Quit", center_x, menu_start_y + 2 * menu_spacing, menu_fontsize, TEXT_ALIGN_CENTER, rs);

		char buffer[100];
		sprintf(buffer, "%s has won the cup", stateInfo->teamData[stateInfo->tournamentState->cupInfo.winnerIndex].name);
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
		draw_text_2d("Normal", center_x, menu_start_y, menu_fontsize, TEXT_ALIGN_CENTER, rs);
		draw_text_2d("One", center_x, menu_start_y + menu_spacing, menu_fontsize, TEXT_ALIGN_CENTER, rs);
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

	// --- Draw Slots ---
	for(int i = 0; i < SLOT_COUNT; i++) {
		// Convert normalized coordinates to virtual screen coordinates
		float x = (viewTreeState->treeCoordinates[i].x + 1.0f) / 2.0f * VIRTUAL_WIDTH - (slot_width / 2.0f);
		float y = (1.0f - viewTreeState->treeCoordinates[i].y) / 2.0f * VIRTUAL_HEIGHT - (slot_height / 2.0f);
		draw_texture_2d(slot_texture, x, y, slot_width, slot_height);
	}

	// --- Draw Text ---
	for(int i = 0; i < SLOT_COUNT; i++) {
		int index = stateInfo->tournamentState->cupInfo.cupTeamIndexTree[i];
		if(index != -1) {
			// Convert normalized coordinates to virtual screen coordinates for text
			float text_center_x = (viewTreeState->treeCoordinates[i].x + 1.0f) / 2.0f * VIRTUAL_WIDTH;
			float text_center_y = (1.0f - viewTreeState->treeCoordinates[i].y) / 2.0f * VIRTUAL_HEIGHT;

			// Adjust for better visual alignment
			float team_name_x = text_center_x - slot_width * 0.15f;
			float text_y = text_center_y - text_size / 2.0f; // Move text up from the bottom

			char* team_name = stateInfo->teamData[index].name;
			draw_text_2d(team_name, team_name_x, text_y, text_size, TEXT_ALIGN_CENTER, rs);

			char wins_str[2];
			sprintf(wins_str, "%d", stateInfo->tournamentState->cupInfo.slotWins[i]);
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

	int counter = 0;
	for(int i = 0; i < 4; i++) {
		int index1 = -1;
		int index2 = -1;
		if(stateInfo->tournamentState->cupInfo.schedule[i][0] != -1)
			index1 = stateInfo->tournamentState->cupInfo.cupTeamIndexTree[stateInfo->tournamentState->cupInfo.schedule[i][0]];
		if(stateInfo->tournamentState->cupInfo.schedule[i][1] != -1)
			index2 = stateInfo->tournamentState->cupInfo.cupTeamIndexTree[stateInfo->tournamentState->cupInfo.schedule[i][1]];

		if(index1 != -1 && index2 != -1) {
			float current_y = list_start_y + counter * list_spacing;
			draw_text_2d(stateInfo->teamData[index1].name, center_x - team_name_offset, current_y, text_fontsize, TEXT_ALIGN_CENTER, rs);
			draw_text_2d("-", center_x, current_y, text_fontsize, TEXT_ALIGN_CENTER, rs);
			draw_text_2d(stateInfo->teamData[index2].name, center_x + team_name_offset, current_y, text_fontsize, TEXT_ALIGN_CENTER, rs);
			counter++;
		}
	}
}

static void getScheduleForCup(const CupInfo* cup_info, int schedule[4][2])
{
	int j;
	int counter = 0;
	for(j = 0; j < SLOT_COUNT/2; j++) {
		if(cup_info->gameStructure == 0) {
			if(cup_info->slotWins[j*2] < 3 && cup_info->slotWins[j*2+1] < 3) {
				if(j < 4 || (j < 6 && cup_info->dayCount >= 1) ||
				        (j == 6 && cup_info->dayCount >= 2)) {
					schedule[counter][0] = j*2;
					schedule[counter][1] = j*2+1;
					counter++;
				}
			}
		} else {
			if(cup_info->slotWins[j*2] < 1 && cup_info->slotWins[j*2+1] < 1) {
				if(j < 4 || (j < 6 && cup_info->dayCount >= 1) ||
				        (j == 6 && cup_info->dayCount >= 2)) {
					schedule[counter][0] = j*2;
					schedule[counter][1] = j*2+1;
					counter++;
				}
			}
		}
	}
	for(j = counter; j < 4; j++) {
		schedule[j][0] = -1;
		schedule[j][1] = -1;
	}
}

static void drawScreen_LoadOrSaveCup(const CupLoadSaveState* loadSaveState, const CupMenuScreen screen, const StateInfo* stateInfo, const RenderState* rs, ResourceManager* rm)
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
		const CupInfo* saved_cup = &stateInfo->tournamentState->saveData[i];
		int user_team_tree_idx = saved_cup->userTeamIndexInTree;

		if(user_team_tree_idx != -1) {
			// Draw Team Name
			char* team_name = stateInfo->teamData[saved_cup->cupTeamIndexTree[user_team_tree_idx]].name;
			draw_text_2d(team_name, center_x, current_y, main_fontsize, TEXT_ALIGN_CENTER, rs);

			// --- Draw Details ---
			char details[100];
			char opponent_name[50] = "N/A";

			// Manually calculate winner for this save slot, as it's not stored directly
			int winnerIndex = -1;
			if (saved_cup->gameStructure == 1) { // Best of 1
				if (saved_cup->slotWins[12] >= 1) winnerIndex = saved_cup->cupTeamIndexTree[12];
				else if (saved_cup->slotWins[13] >= 1) winnerIndex = saved_cup->cupTeamIndexTree[13];
			} else { // Best of 5 (3 wins)
				if (saved_cup->slotWins[12] >= 3) winnerIndex = saved_cup->cupTeamIndexTree[12];
				else if (saved_cup->slotWins[13] >= 3) winnerIndex = saved_cup->cupTeamIndexTree[13];
			}

			if (winnerIndex != -1) {
				strcpy(opponent_name, "Cup finished");
			} else {
				int schedule[4][2];
				getScheduleForCup(saved_cup, schedule);
				int opponent_found = 0;
				for (int j = 0; j < 4 && !opponent_found; j++) {
					if (schedule[j][0] == user_team_tree_idx) {
						int opponent_tree_idx = schedule[j][1];
						if (opponent_tree_idx != -1) {
							int opponent_team_idx = saved_cup->cupTeamIndexTree[opponent_tree_idx];
							if (opponent_team_idx != -1) {
								strcpy(opponent_name, stateInfo->teamData[opponent_team_idx].name);
								opponent_found = 1;
							}
						}
					} else if (schedule[j][1] == user_team_tree_idx) {
						int opponent_tree_idx = schedule[j][0];
						if (opponent_tree_idx != -1) {
							int opponent_team_idx = saved_cup->cupTeamIndexTree[opponent_tree_idx];
							if (opponent_team_idx != -1) {
								strcpy(opponent_name, stateInfo->teamData[opponent_team_idx].name);
								opponent_found = 1;
							}
						}
					}
				}
			}

			sprintf(details, "Day: %d | Next Opponent: %s", saved_cup->dayCount + 1, opponent_name);
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

static int refreshLoadCups(StateInfo* stateInfo)
{
	// load save slots into menuData->saveData
	readSaveData(stateInfo->tournamentState->saveData, 5);

	int i, j;
	// go through the saveData-structure and figure out if its good.
	for(i = 0; i < 5; i++) {
		if(stateInfo->tournamentState->saveData[i].userTeamIndexInTree != -1) {
			int valid = 1;
			if(stateInfo->tournamentState->saveData[i].dayCount < 0) valid = 0;
			if(stateInfo->tournamentState->saveData[i].gameStructure != 0 && stateInfo->tournamentState->saveData[i].gameStructure != 1) valid = 0;
			if(stateInfo->tournamentState->saveData[i].inningCount != 2 && stateInfo->tournamentState->saveData[i].inningCount != 4 && stateInfo->tournamentState->saveData[i].inningCount != 8) valid = 0;
			if(stateInfo->tournamentState->saveData[i].winnerIndex >= stateInfo->numTeams) valid = 0;
			if(stateInfo->tournamentState->saveData[i].userTeamIndexInTree >= 14) valid = 0;
			for(j = 0; j < SLOT_COUNT; j++) {
				if(stateInfo->tournamentState->saveData[i].slotWins[j] < 0 || stateInfo->tournamentState->saveData[i].slotWins[j] > 3) valid = 0;
				if(stateInfo->tournamentState->saveData[i].cupTeamIndexTree[j] > stateInfo->numTeams) valid = 0;
			}

			if (valid == 0) {
				printf("Save slot %d is invalid or from an old version. Ignoring.\n", i);
				stateInfo->tournamentState->saveData[i].userTeamIndexInTree = -1;
			}
		}
	}
	return 0;
}

static void saveCup(StateInfo* stateInfo, int slot)
{
	// write current cup state into save slot
	writeSaveData(stateInfo->tournamentState->saveData, &stateInfo->tournamentState->cupInfo, slot, 5);

	// Refresh
	int result = refreshLoadCups(stateInfo);
	if (result != 0) {
		printf("Something wrong with the save file.\n");
	}
}

static void loadCup(StateInfo* stateInfo, int slot)
{
	int i;
	stateInfo->tournamentState->cupInfo.inningCount = stateInfo->tournamentState->saveData[slot].inningCount;
	stateInfo->tournamentState->cupInfo.gameStructure = stateInfo->tournamentState->saveData[slot].gameStructure;
	stateInfo->tournamentState->cupInfo.userTeamIndexInTree = stateInfo->tournamentState->saveData[slot].userTeamIndexInTree;
	stateInfo->tournamentState->cupInfo.dayCount = stateInfo->tournamentState->saveData[slot].dayCount;

	for(i = 0; i < SLOT_COUNT; i++) {
		stateInfo->tournamentState->cupInfo.cupTeamIndexTree[i] = stateInfo->tournamentState->saveData[slot].cupTeamIndexTree[i];
		stateInfo->tournamentState->cupInfo.slotWins[i] = stateInfo->tournamentState->saveData[slot].slotWins[i];
	}
	stateInfo->tournamentState->cupInfo.winnerIndex = -1;
	if(stateInfo->tournamentState->cupInfo.gameStructure == 1) {
		if(stateInfo->tournamentState->cupInfo.slotWins[12] == 1) {
			stateInfo->tournamentState->cupInfo.winnerIndex = stateInfo->tournamentState->cupInfo.cupTeamIndexTree[12];
		} else if(stateInfo->tournamentState->cupInfo.slotWins[13] == 1) {
			stateInfo->tournamentState->cupInfo.winnerIndex = stateInfo->tournamentState->cupInfo.cupTeamIndexTree[13];
		}
	} else if(stateInfo->tournamentState->cupInfo.gameStructure == 0) {
		if(stateInfo->tournamentState->cupInfo.slotWins[12] == 3) {
			stateInfo->tournamentState->cupInfo.winnerIndex = stateInfo->tournamentState->cupInfo.cupTeamIndexTree[12];
		} else if(stateInfo->tournamentState->cupInfo.slotWins[13] == 3) {
			stateInfo->tournamentState->cupInfo.winnerIndex = stateInfo->tournamentState->cupInfo.cupTeamIndexTree[13];
		}
	}

	// Recalculate the schedule based on the loaded cup state
	updateSchedule(stateInfo->tournamentState, stateInfo);
}

void initCupMenu(CupMenuState* cupMenuState, StateInfo* stateInfo)
{
	// Initialize all sub-states to a known default
	memset(&cupMenuState->initial, 0, sizeof(CupInitialState));
	memset(&cupMenuState->ongoing, 0, sizeof(CupOngoingState));
	memset(&cupMenuState->new_cup, 0, sizeof(CupNewState));
	memset(&cupMenuState->load_save, 0, sizeof(CupLoadSaveState));
	memset(&cupMenuState->credits_menu, 0, sizeof(CreditsMenuState));

	// Initialize the tree coordinates, as they are constant layout data
	initCupViewTreeState(&cupMenuState->view_tree);

	// Set the starting screen based on whether a cup is in progress
	if (stateInfo->tournamentState->cupInfo.userTeamIndexInTree == -1) {
		cupMenuState->screen = CUP_MENU_SCREEN_INITIAL;
		cupMenuState->initial.pointer = 0;
		cupMenuState->initial.rem = 2;
	} else {
		cupMenuState->screen = CUP_MENU_SCREEN_ONGOING;
		cupMenuState->ongoing.pointer = 0;
		if (stateInfo->tournamentState->cupInfo.winnerIndex != -1) {
			cupMenuState->ongoing.rem = 3;
		} else {
			cupMenuState->ongoing.rem = 5;
		}
	}

	// Initialize credits screen state
	cupMenuState->credits_menu.creditsScrollX = VIRTUAL_WIDTH;

	if(refreshLoadCups(stateInfo) != 0) {
		printf("Something wrong with the save file.\n");
	}
}
MenuStage updateCupMenu(
    CupMenuState* cupMenuState,
    StateInfo* stateInfo,
    const KeyStates* keyStates,
    CupMenuOutput* output
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
		if(keyStates->released[0][KEY_2]) return updateScreen_Ongoing(cupMenuState, stateInfo, output);
		return MENU_STAGE_CUP;
	}
	case CUP_MENU_SCREEN_NEW_CUP:
		return updateScreen_NewCup(cupMenuState, stateInfo, keyStates);
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
		drawScreen_Initial(&cupMenuState->initial, rs, rm);
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
		drawScreen_LoadOrSaveCup(&cupMenuState->load_save, cupMenuState->screen, stateInfo, rs, rm);
		break;
	case CUP_MENU_SCREEN_END_CREDITS:
		drawScreen_EndCredits(&cupMenuState->credits_menu, rs, rm);
		break;
	}
}
