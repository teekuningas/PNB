#ifndef MENU_HELPERS_H
#define MENU_HELPERS_H

#include "menu_types.h"
#include "game_setup.h"
#include "resource_manager.h"
#include "render.h"

void createGameSetup(GameSetup* gameSetup, MenuData* menuData, MenuInfo* menuInfo);
void updateSchedule(TournamentState* tournamentState, StateInfo* stateInfo);
void updateCupTreeAfterDay(TournamentState* tournamentState, StateInfo* stateInfo, int scheduleSlot, int winningSlot);
void resetMenuForNewGame(MenuData* menuData, StateInfo* stateInfo);
void initBattingOrderState(BattingOrderState *state, int team_index, int player_control);
// Draws a full-screen 2D background quad for menus
// Uses the "empty_background" texture from ResourceManager
void drawMenuLayout2D(ResourceManager* rm, const RenderState* rs);

#endif /* MENU_HELPERS_H */
