#ifndef PLAYER_RENDERER_H
#define PLAYER_RENDERER_H

#include "../include/globals.h" // For StateInfo and PlayerInfo structures

// Function to initialize player rendering resources (textures, models)
int initPlayerRenderer(void);

// Function to draw players
void drawPlayerRenderer(const StateInfo* stateInfo, const PlayerInfo* playerInfo, double alpha);

// Function to clean up player rendering resources
int cleanPlayerRenderer(void);

#endif // PLAYER_RENDERER_H
