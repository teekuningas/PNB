#ifndef MENU_HELPERS_H
#define MENU_HELPERS_H

#include "menu_types.h"

void updateSchedule(MenuData* menuData, StateInfo* stateInfo);
void updateCupTreeAfterDay(MenuData* menuData, StateInfo* stateInfo, int scheduleSlot, int winningSlot);
void resetMenuForNewGame(MenuData* menuData);

#endif /* MENU_HELPERS_H */
