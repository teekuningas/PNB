#ifndef BATTING_ORDER_MENU_H
#define BATTING_ORDER_MENU_H

#include "menu_types.h"

// Function to initialize or reset the state to its default values
void initBattingOrderState(BattingOrderState *state, int team_index, int player_control);

// Update function now takes the specific state it needs and returns the next stage
MenuStage updateBattingOrderMenu(BattingOrderState *state, const KeyStates *keyStates, MenuStage currentStage, MenuMode menuMode);

// Draw function also takes the specific state, plus rendering data from MenuData
void drawBattingOrderMenu(const BattingOrderState *state, const StateInfo *stateInfo, const struct MenuData *menuData);

#endif // BATTING_ORDER_MENU_H
