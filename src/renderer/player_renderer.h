#ifndef PLAYER_RENDERER_H
#define PLAYER_RENDERER_H

#include "../include/globals.h" // For StateInfo and PlayerInfo structures
#include "../core/resource_manager.h"

// Function to initialize player rendering resources (textures, models)
int init_player_renderer(ResourceManager* rm);

// Function to draw players
void draw_player_renderer(const StateInfo* stateInfo, const PlayerInfo* playerInfo, double alpha, ResourceManager* rm);

// Function to clean up player rendering resources
int clean_player_renderer(void);

#endif // PLAYER_RENDERER_H
