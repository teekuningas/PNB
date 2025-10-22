#ifndef FRONT_MENU_H
#define FRONT_MENU_H

#include "menu_types.h"

// Function to initialize or reset the state to its default values
void initFrontMenuState(FrontMenuState *state);

// Update function now takes the specific state it needs and returns the next stage
MenuStage updateFrontMenu(FrontMenuState *state, KeyStates *keyStates, StateInfo* stateInfo);

// Draw function also takes the specific state, plus rendering data from MenuData
void drawFrontMenu(const FrontMenuState *state, const struct MenuData *menuData);

#endif // FRONT_MENU_H
