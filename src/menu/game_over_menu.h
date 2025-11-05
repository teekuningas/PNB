/*
 * game_over_menu.h
 *
 * Handles drawing the Game-Over menu screen.
 */
#ifndef GAME_OVER_MENU_H
#define GAME_OVER_MENU_H

#include "globals.h"
#include "render.h"
#include "resource_manager.h"
#include "menu_types.h"

// Updates the game over menu.
MenuStage updateGameOverMenu(const GameConclusion* conclusion, const KeyStates* keyStates, int team1_control, int team2_control);

// Draws the game over menu.
void drawGameOverMenu(const GameConclusion* conclusion, const TeamData* teamData, RenderState* rs, ResourceManager* rm);

#endif // GAME_OVER_MENU_H
