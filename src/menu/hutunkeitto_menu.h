#ifndef HUTUNKEITTO_MENU_H
#define HUTUNKEITTO_MENU_H

#include "menu_types.h"

// Function to initialize or reset the state to its default values
void initHutunkeittoState(HutunkeittoState *state);

// Update function now takes the specific state it needs and returns the next stage
MenuStage updateHutunkeittoMenu(HutunkeittoState *state, const KeyStates *keyStates, int team1_control, int team2_control);

// Draw function also takes the specific state, plus rendering data from MenuData
void drawHutunkeittoMenu(const HutunkeittoState *state, const struct MenuData *menuData);

#endif // HUTUNKEITTO_MENU_H
