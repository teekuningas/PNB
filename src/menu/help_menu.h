#ifndef HELP_MENU_H
#define HELP_MENU_H

#include "menu_types.h"
#include "render.h"
#include "key_states.h"
#include "resource_manager.h"

void init_help_menu(HelpMenuState* state);
MenuStage update_help_menu(HelpMenuState* state, KeyStates* keyStates);
void draw_help_menu(HelpMenuState* state, const RenderState* rs, ResourceManager* rm);

#endif // HELP_MENU_H
