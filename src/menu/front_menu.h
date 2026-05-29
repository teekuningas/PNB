#ifndef FRONT_MENU_H
#define FRONT_MENU_H

#include "menu_types.h"
#include "resource_manager.h"
#include "render.h"

void init_front_menu_state(FrontMenuState* state);
MenuStage update_front_menu(FrontMenuState* state, KeyStates* keyStates, StateInfo* stateInfo);

// New orthographic-only front menu rendering (background, figures, arrow, text)
void draw_front_menu(const FrontMenuState* state, const RenderState* rs, ResourceManager* rm);

#endif /* FRONT_MENU_H */
