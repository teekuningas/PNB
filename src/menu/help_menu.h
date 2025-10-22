#ifndef HELP_MENU_H
#define HELP_MENU_H

#include "menu_types.h"
#include "input.h"

void initHelpMenu(HelpMenuState *state);
MenuStage updateHelpMenu(HelpMenuState *state, KeyStates *keyStates);
void drawHelpMenu(HelpMenuState *state);

#endif // HELP_MENU_H
