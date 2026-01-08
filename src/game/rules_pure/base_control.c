#include "base_control.h"
#include "vector_math.h"

int get_base_controller(const LocalGameInfo* game, BaseID base)
{
	if (base < 0 || base >= BASE_COUNT) return -1;

	// Iterate through all players to find who claims safety at this base.
	// This replaces the cached baseControlIndex array.
	for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		// We trust the Referee State as the single source of truth for safety.
		if (game->referee.battingPlayers[i].currentSafetyBase == base) {
			return i;
		}
	}
	return -1;
}

int get_ball_at_base_index(const StateInfo* stateInfo)
{
	if (stateInfo->localGameInfo->pII.hasBallIndex == -1) {
		return -1; // No one has ball
	}

	// Use catcher position directly or ball position?
	// Legacy logic uses ballInfo.location, assuming ball is with player.
	Vector3D ballLoc = stateInfo->localGameInfo->ballInfo.location;

	// Check Home Base (Index 0)
	// Special check: inside homeline-middlepoint centered disk and at homebase side
	float dx = ballLoc.x - stateInfo->fieldPositions->pitchPlate.x;
	float dz = ballLoc.z - HOME_LINE_Z;
	if (ballLoc.z > HOME_LINE_Z && vec3_is_small_enough_circle_xz(dx, dz, HOME_RADIUS)) {
		return 0; // Home Base
	}

	// Check Bases 1-3
	for (int i = 1; i < BASE_COUNT; i++) {
		Vector3D baseLoc;
		if (i == 1) baseLoc = stateInfo->fieldPositions->firstBase;
		else if (i == 2) baseLoc = stateInfo->fieldPositions->secondBase;
		else if (i == 3) baseLoc = stateInfo->fieldPositions->thirdBase;
		else continue;

		dx = ballLoc.x - baseLoc.x;
		dz = ballLoc.z - baseLoc.z;

		if (vec3_is_small_enough_circle_xz(dx, dz, BASE_RADIUS)) {
			return i;
		}
	}

	return -1;
}
