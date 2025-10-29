#ifndef TEAM_SELECTION_MENU_H
#define TEAM_SELECTION_MENU_H

#include "menu_types.h"
#include "resource_manager.h"
#include "render.h"

typedef struct {
	int team1;
	int team2;
	int team1_controller;
	int team2_controller;
	int innings;
} TeamSelectionMenuOutput;

// Function to initialize or reset the state to its default values
void initTeamSelectionState(TeamSelectionState *state, int numTeams);

// Update function now takes the specific state it needs and returns the next stage
MenuStage updateTeamSelectionMenu(TeamSelectionState *state, const KeyStates *keyStates, TeamSelectionMenuOutput *output);

// Draw function also takes the specific state, plus rendering data from MenuData
void drawTeamSelectionMenu(const TeamSelectionState *state, const TeamData* teamData, const RenderState* rs, ResourceManager* rm);

#endif // TEAM_SELECTION_MENU_H

