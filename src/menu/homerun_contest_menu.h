#ifndef HOMERUN_CONTEST_MENU_H
#define HOMERUN_CONTEST_MENU_H

#include "menu_types.h"
#include "input.h"
#include "resource_manager.h"
#include "globals.h"
#include "render.h"

typedef struct {
	int choices[2][MAX_HOMERUN_PAIRS];
} HomerunContestMenuOutput;

// Initialize or reset a home-run contest state for one team
void initHomerunContestState(HomerunContestState *state,
                             int team_index,
                             int player_control,
                             int choiceCount);

// Update function for a home-run contest stage. Returns next MenuStage.
MenuStage updateHomerunContestMenu(HomerunContestState *state,
                                   const KeyStates *keyStates,
                                   MenuStage currentStage,
                                   HomerunContestMenuOutput *output,
                                   const TeamData* teamData);

// Draw function for a home-run contest stage
void drawHomerunContestMenu(const HomerunContestState *state,
                            const RenderState *rs,
                            ResourceManager *rm,
                            const TeamData* teams);

#endif // HOMERUN_CONTEST_MENU_H