#ifndef MENU_HELPERS_H
#define MENU_HELPERS_H

#include "menu_types.h"
#include "game_setup.h"

void createGameSetup(GameSetup* gameSetup, MenuData* menuData, MenuInfo* menuInfo);
void updateSchedule(TournamentState* tournamentState, StateInfo* stateInfo);
void updateCupTreeAfterDay(TournamentState* tournamentState, StateInfo* stateInfo, int scheduleSlot, int winningSlot);
void resetMenuForNewGame(MenuData* menuData, StateInfo* stateInfo);
void initBattingOrderState(BattingOrderState *state, int team_index, int player_control);

#endif /* MENU_HELPERS_H */
