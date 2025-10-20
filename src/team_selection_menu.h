#ifndef TEAM_SELECTION_MENU_H
#define TEAM_SELECTION_MENU_H

#include "menu_types.h"

// Forward-declare MenuData to avoid circular include with main_menu.h
struct MenuData;

// Function to initialize or reset the state to its default values
void initTeamSelectionState(TeamSelectionState *state);

// Update function now takes the specific state it needs and returns the next stage
MenuStage updateTeamSelectionMenu(TeamSelectionState *state, const StateInfo *stateInfo, const KeyStates *keyStates);

// Draw function also takes the specific state, plus rendering data from MenuData
void drawTeamSelectionMenu(const TeamSelectionState *state, const StateInfo *stateInfo, const struct MenuData *menuData);

#endif // TEAM_SELECTION_MENU_H

