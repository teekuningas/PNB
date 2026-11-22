#ifndef PLAYER_RENDERER_H
#define PLAYER_RENDERER_H

#include "rules_state.h"
#include "world.h"
#include "player_view.h"
#include "../core/resource_manager.h"

// Function to initialize player rendering resources (textures, models)
int init_player_renderer(ResourceManager* rm);

// Draw every player: where they are, the shape they are making, their team's kit, and the marker over the
// controlled fielder.
void draw_player_renderer(
    const MatchSession* match, const Scoreboard* scoreboard, const PlayerView* players, double alpha,
    ResourceManager* rm
);

// Function to clean up player rendering resources
int clean_player_renderer(void);

#endif // PLAYER_RENDERER_H
