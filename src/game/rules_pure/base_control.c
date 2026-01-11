#include "base_control.h"
#include "vector_math.h"

int get_base_controller(const MatchSession* game, BaseID base)
{
	if (base < 0 || base >= BASE_COUNT) return -1;

	// Iterate through all players to find who claims safety at this base.
	// Player must BOTH have safety at the base AND be physically at the base.
	// This handles vapaataival (free walk) where player has safety immediately
	// but doesn't "control" the base until arrival.
	// If multiple players qualify (e.g. Tuplahaava pending), prioritize the lead runner.
	int bestCandidate = -1;
	int highestBaseAtPitch = -2;

	for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		if (game->referee.battingPlayers[i].currentSafetyBase == base &&
		        game->playerInfo[i].bTPI.baseId == base) {
			int baseAtPitch = (int)game->referee.battingPlayers[i].baseAtPitchStart;
			if (baseAtPitch > highestBaseAtPitch) {
				highestBaseAtPitch = baseAtPitch;
				bestCandidate = i;
			}
		}
	}
	return bestCandidate;
}

int get_ball_at_base_index(const StateInfo* stateInfo)
{
	if (stateInfo->match->pII.hasBallIndex == -1) {
		return -1; // No one has ball
	}

	// Use catcher position directly or ball position?
	// Legacy logic uses ballInfo.location, assuming ball is with player.
	Vector3D ballLoc = stateInfo->match->ballInfo.location;

	// Check Home Base (Index 0)
	// Special check: inside homeline-middlepoint centered disk and at homebase side
	float dx = ballLoc.x - stateInfo->fieldPositions->pitchPlate.x;
	float dz = ballLoc.z - HOME_LINE_Z;
	if (ballLoc.z > HOME_LINE_Z &&vec3_is_small_enough_circle_xz(dx, dz, HOME_RADIUS)) {
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
