#ifndef CUP_MENU_H
#define CUP_MENU_H

#include "menu_types.h"

void initCupMenu(CupMenuState* cupMenuState, StateInfo* stateInfo);
MenuStage updateCupMenu(CupMenuState* cupMenuState, StateInfo* stateInfo, struct MenuData* menuData, KeyStates* keyStates);
void drawCupMenu(const CupMenuState* cupMenuState, const StateInfo* stateInfo, const struct MenuData* menuData);

#endif /* CUP_MENU_H */
