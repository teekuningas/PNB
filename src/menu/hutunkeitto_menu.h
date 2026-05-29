#ifndef HUTUNKEITTO_MENU_H
#define HUTUNKEITTO_MENU_H

#include "menu_types.h"
#include "resource_manager.h"
#include "render.h"

// Function to initialize or reset the state to its default values
void init_hutunkeitto_state(HutunkeittoState* state);

// Update function now takes the specific state it needs and returns the next stage
MenuStage update_hutunkeitto_menu(
    HutunkeittoState* state, const KeyStates* keyStates, int team1_control, int team2_control, GameSetup* gameSetup,
    unsigned int* rng_seed
);

// Draw function also takes the specific state, plus rendering data
void draw_hutunkeitto_menu(
    const HutunkeittoState* state, const RenderState* rs, ResourceManager* rm, int team1_idx, int team2_idx
);

#endif // HUTUNKEITTO_MENU_H