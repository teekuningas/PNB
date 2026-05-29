#ifndef BATTING_ORDER_MENU_H
#define BATTING_ORDER_MENU_H

#include "menu_types.h"
#include "globals.h"
#include "resource_manager.h"
#include "render.h"

void init_batting_order_state(BattingOrderState* state, int team_index, int player_control, const StateInfo* stateInfo);
MenuStage update_batting_order_menu(
    BattingOrderState* state, const KeyStates* keyStates, MenuStage currentStage, MenuMode menuMode,
    GameSetup* gameSetup
);
void draw_batting_order_menu(
    const BattingOrderState* state, MenuStage currentStage, const RenderState* rs, ResourceManager* rm
);

#endif /* BATTING_ORDER_MENU_H */
