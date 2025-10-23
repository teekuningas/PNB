#ifndef MENU_HELPERS_H
#define MENU_HELPERS_H

#include "menu_types.h"

void updateSchedule(TournamentState* tournamentState, StateInfo* stateInfo);
void updateCupTreeAfterDay(TournamentState* tournamentState, StateInfo* stateInfo, int scheduleSlot, int winningSlot);
void resetMenuForNewGame(MenuData* menuData, StateInfo* stateInfo);

#endif /* MENU_HELPERS_H */
