#ifndef CUP_H
#define CUP_H

#include "globals.h"

void updateSchedule(TournamentState* tournamentState, StateInfo* stateInfo);
void updateCupTreeAfterDay(TournamentState* tournamentState, StateInfo* stateInfo, int scheduleSlot, int winningSlot, int randomNumber);
void cup_process_finished_game(TournamentState* tournamentState, StateInfo* stateInfo, int gameWinner);

#endif /* CUP_H */
