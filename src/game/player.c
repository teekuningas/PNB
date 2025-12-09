/*
	here we are just gonna render our players. draw function is the key part, it will take all the information about players and use that to draw
	them. it means that all the data about how the players look like in the scene will be in the playerInfo-structure.
*/

#include "globals.h"
#include "player.h"
#include "../renderer/player_renderer.h" // Include the new renderer header

#define PLAYER_SCALE 0.24f
#define SELECTION_BALL_SCALE 0.2f

int initPlayer(StateInfo* stateInfo)
{
	return initPlayerRenderer();
}
