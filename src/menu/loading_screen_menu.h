#ifndef LOADING_SCREEN_MENU_H
#define LOADING_SCREEN_MENU_H

#include "resource_manager.h"
#include "render.h"

// Draw the initial loading screen before main menu startup.
void draw_loading_screen(ResourceManager* rm, const RenderState* rs);

#endif // LOADING_SCREEN_MENU_H
