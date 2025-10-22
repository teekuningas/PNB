#ifndef HOMERUN_CONTEST_MENU_H
#define HOMERUN_CONTEST_MENU_H

#include "menu_types.h"

// Initialize or reset a home-run contest state for one team
// team_index: index into StateInfo->teamData
// player_control: 0=Pad1, 1=Pad2, 2=AI
// choiceCount: total picks (batters + runners) for this team
void initHomerunContestState(HomerunContestState *state,
                             int team_index,
                             int player_control,
                             int choiceCount);

// Update function for a home-run contest stage. Returns next MenuStage.
// currentStage should be either MENU_STAGE_HOMERUN_CONTEST_1 or _2
MenuStage updateHomerunContestMenu(HomerunContestState *state,
                                   const KeyStates *keyStates,
                                   const StateInfo *stateInfo,
                                   MenuStage currentStage);

// Draw function for a home-run contest stage
void drawHomerunContestMenu(const HomerunContestState *state,
                            const StateInfo *stateInfo,
                            const struct MenuData *menuData);

#endif // HOMERUN_CONTEST_MENU_H
