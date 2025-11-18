#ifndef BATTING_ORDER_MENU_H
#define BATTING_ORDER_MENU_H

#include "menu_types.h"
#include "globals.h"
#include "resource_manager.h"
#include "render.h"

void initBattingOrderState(BattingOrderState *state, int team_index, int player_control, const StateInfo *stateInfo);
MenuStage updateBattingOrderMenu(
    BattingOrderState *state,
    const KeyStates *keyStates,
    MenuStage currentStage,
    MenuMode menuMode,
    GameSetup *gameSetup
);
void drawBattingOrderMenu(const BattingOrderState *state, MenuStage currentStage, const RenderState* rs, ResourceManager* rm);

#endif /* BATTING_ORDER_MENU_H */
