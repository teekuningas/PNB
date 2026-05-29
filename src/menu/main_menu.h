#ifndef MAIN_MENU_H
#define MAIN_MENU_H

#include "globals.h"
#include "menu_types.h"
#include "resource_manager.h"
#include "render.h"

// Initialize menuData and prepare menu
int init_main_menu(StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo, ResourceManager* rm, RenderState* rs);
// Update and draw take explicit MenuData pointer for state
// Update and draw now explicitly take MenuData pointer
void update_main_menu(
    StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo, KeyStates* keyStates, unsigned int* rng_seed
);
void draw_main_menu(
    const StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo, double alpha, ResourceManager* rm,
    RenderState* rs
);
int clean_main_menu(MenuData* menuData);

#endif /* MAIN_MENU_H */
