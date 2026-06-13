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

// Updates the game over menu. Any human pad can dismiss the result screen,
// independent of team control — so an AI-vs-AI game a human is watching is not
// trapped here.
MenuStage updateGameOverMenu(const GameConclusion* conclusion, const KeyStates* keyStates);

// Draws the game over menu.
void draw_game_over_menu(
    const GameConclusion* conclusion, const TeamData* teamData, RenderState* rs, ResourceManager* rm
);

#endif // GAME_OVER_MENU_H
