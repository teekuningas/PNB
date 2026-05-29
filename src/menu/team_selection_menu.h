#ifndef TEAM_SELECTION_MENU_H
#define TEAM_SELECTION_MENU_H

#include "menu_types.h"
#include "resource_manager.h"
#include "render.h"

// Function to initialize or reset the state to its default values
void init_team_selection_state(TeamSelectionState* state, int numTeams);

// Update function now takes the specific state it needs and returns the next stage
MenuStage update_team_selection_menu(TeamSelectionState* state, const KeyStates* keyStates, GameSetup* gameSetup);

// Draw function also takes the specific state, plus rendering data from MenuData
void draw_team_selection_menu(
    const TeamSelectionState* state, const TeamData* teamData, const RenderState* rs, ResourceManager* rm
);

#endif // TEAM_SELECTION_MENU_H
