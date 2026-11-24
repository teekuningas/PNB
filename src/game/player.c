// Player render resource setup; poses and clip clocks belong to the client.

#include "player.h"
#ifndef NO_RENDER
#include "../renderer/player_renderer.h"
#endif

int init_player(ResourceManager* rm)
{
#ifndef NO_RENDER
    if (init_player_renderer(rm) != 0) return -1;
#endif
    return 0;
}
