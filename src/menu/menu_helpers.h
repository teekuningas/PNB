#ifndef MENU_HELPERS_H
#define MENU_HELPERS_H

#include "menu_types.h"
#include "game_setup.h"
#include "resource_manager.h"
#include "render.h"

void launchGameFromMenu(StateInfo* stateInfo, const GameSetup* gameSetup);
void resetMenuForNewGame(MenuData* menuData, StateInfo* stateInfo);
void initBattingOrderState(BattingOrderState *state, int team_index, int player_control, const StateInfo *stateInfo);
// Draws a full-screen 2D background quad for menus
// Uses the "empty_background" texture from ResourceManager
void drawMenuLayout2D(ResourceManager* rm, const RenderState* rs);

// --- 2D Text Rendering Framework ---

typedef enum {
	TEXT_ALIGN_LEFT,
	TEXT_ALIGN_CENTER
} TextAlign;

void draw_text_2d(const char* text, float x, float y, float size, TextAlign align, const RenderState* rs);
void draw_text_block_2d(const char* text, float x, float y, float width, float size, float lineSpacing, const RenderState* rs);


#endif /* MENU_HELPERS_H */
