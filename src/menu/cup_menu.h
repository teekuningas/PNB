#ifndef CUP_MENU_H
#define CUP_MENU_H

#include "menu_types.h"
#include "input.h"   // For KeyStates
#include "render.h"  // For GLuint

typedef struct {
	// Data needed to start a game
	int team1;
	int team2;
	int team1_control;
	int team2_control;
	int innings;
} CupMenuOutput;


void initCupMenu(CupMenuState* cupMenuState, StateInfo* stateInfo);

MenuStage updateCupMenu(
    CupMenuState* cupMenuState,
    StateInfo* stateInfo, // Kept non-const for now to manage tournament state
    const KeyStates* keyStates,
    CupMenuOutput* output
);

void drawCupMenu(
    const CupMenuState* cupMenuState,
    const StateInfo* stateInfo,
    const struct MenuData* menuData
);

#endif /* CUP_MENU_H */
