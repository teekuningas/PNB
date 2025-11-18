#ifndef CUP_H
#define CUP_H

#include "globals.h"
#include "menu_types.h"

void updateSchedule(TournamentState* tournamentState, StateInfo* stateInfo);
void updateCupTreeAfterDay(TournamentState* tournamentState, StateInfo* stateInfo, int scheduleSlot, int winningSlot);

#endif /* CUP_H */
