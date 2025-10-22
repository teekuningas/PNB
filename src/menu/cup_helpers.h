#ifndef CUP_HELPERS_H
#define CUP_HELPERS_H

#include "globals.h"
#include "menu_types.h"

void updateCupTreeAfterDay(MenuData* md, StateInfo* stateInfo, int scheduleSlot, int winningSlot);
void updateSchedule(MenuData* md, StateInfo* stateInfo);

#endif // CUP_HELPERS_H
