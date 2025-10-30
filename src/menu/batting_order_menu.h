#ifndef BATTING_ORDER_MENU_H
#define BATTING_ORDER_MENU_H

#include "menu_types.h"
#include "resource_manager.h"
#include "render.h"

typedef struct {
	int batting_order[PLAYERS_IN_TEAM + JOKER_COUNT];
} BattingOrderMenuOutput;

MenuStage updateBattingOrderMenu(BattingOrderState *state, const KeyStates *keyStates, MenuStage currentStage, MenuMode menuMode, BattingOrderMenuOutput *output);
void drawBattingOrderMenu(const BattingOrderState *state, MenuStage currentStage, const RenderState* rs, ResourceManager* rm);

#endif /* BATTING_ORDER_MENU_H */
