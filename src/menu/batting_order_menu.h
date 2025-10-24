#ifndef BATTING_ORDER_MENU_H
#define BATTING_ORDER_MENU_H

#include "menu_types.h"

MenuStage updateBattingOrderMenu(BattingOrderState *state, const KeyStates *keyStates, MenuStage currentStage, MenuMode menuMode);
void drawBattingOrderMenu(const BattingOrderState *state, const StateInfo *stateInfo, const struct MenuData *menuData);

#endif /* BATTING_ORDER_MENU_H */
