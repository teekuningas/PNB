/*
    here we are just gonna render our players. draw function is the key part, it will take all the information about
   players and use that to draw them. it means that all the data about how the players look like in the scene will be in
   the playerInfo-structure.
*/

#include "globals.h"
#include "player.h"
#ifndef NO_RENDER
#include "../renderer/player_renderer.h"
#endif

int init_player(StateInfo* stateInfo, ResourceManager* rm)
{
#ifndef NO_RENDER
    if (init_player_renderer(rm) != 0) return -1;
#endif
    return 0;
}
