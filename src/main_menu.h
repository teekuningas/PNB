#ifndef MAIN_MENU_H
#define MAIN_MENU_H

#include "globals.h"

int initMainMenu(StateInfo* stateInfo);
void updateMainMenu(StateInfo* stateInfo);
void drawMainMenu(StateInfo* stateInfo, double alpha);
int cleanMainMenu(StateInfo* stateInfo);
void drawLoadingScreen(StateInfo* stateInfo);

#endif /* MAIN_MENU_H */
