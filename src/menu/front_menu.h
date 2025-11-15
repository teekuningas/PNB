#ifndef FRONT_MENU_H
#define FRONT_MENU_H

#include "menu_types.h"
#include "resource_manager.h"
#include "render.h"

void initFrontMenuState(FrontMenuState *state);
MenuStage updateFrontMenu(FrontMenuState *state, KeyStates *keyStates, StateInfo* stateInfo);

// New orthographic-only front menu rendering (background, figures, arrow, text)
void drawFrontMenu(const FrontMenuState *state, const RenderState* rs, ResourceManager* rm);

#endif /* FRONT_MENU_H */
