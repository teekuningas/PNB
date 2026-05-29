#ifndef HOMERUN_CONTEST_MENU_H
#define HOMERUN_CONTEST_MENU_H

#include "menu_types.h"
#include "input.h"
#include "resource_manager.h"
#include "globals.h"
#include "render.h"

// Initialize or reset a home-run contest state for one team
void init_homerun_contest_state(HomerunContestState* state, int team_index, int player_control, int choiceCount);

// Update function for a home-run contest stage. Returns next MenuStage.
MenuStage update_homerun_contest_menu(
    HomerunContestState* state, const KeyStates* keyStates, MenuStage currentStage, GameSetup* gameSetup,
    const TeamData* teamData
);

// Draw function for a home-run contest stage
void draw_homerun_contest_menu(
    const HomerunContestState* state, const RenderState* rs, ResourceManager* rm, const TeamData* teams
);

#endif // HOMERUN_CONTEST_MENU_H