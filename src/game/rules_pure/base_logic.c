#include "base_logic.h"

BaseID base_get_next(BaseID id)
{
	if (id == BASE_HOME) return BASE_FIRST;
	if (id == BASE_FIRST) return BASE_SECOND;
	if (id == BASE_SECOND) return BASE_THIRD;
	if (id == BASE_THIRD) return BASE_HOME_SCORED;
	if (id == BASE_HOME_SCORED) return BASE_NONE;
	return BASE_NONE;
}

BaseID base_get_prev(BaseID id)
{
	if (id == BASE_FIRST) return BASE_HOME;
	if (id == BASE_SECOND) return BASE_FIRST;
	if (id == BASE_THIRD) return BASE_SECOND;
	if (id == BASE_HOME_SCORED) return BASE_THIRD;
	return BASE_NONE;
}

bool base_is_safe_haven(BaseID id)
{
	return (id >= BASE_HOME &&id <= BASE_THIRD);
}

bool base_is_index(BaseID id)
{
	return (id >= BASE_HOME &&id <= BASE_THIRD);
}

bool base_can_advance(BaseID id)
{
	// Logic from legacy code: "base >= 0 &&base < 3" -> HOME, 1st, 2nd.
	return (id >= BASE_HOME &&id <= BASE_SECOND);
}

bool base_is_at_least(BaseID id, BaseID threshold)
{
	return id >= threshold;
}

int base_cmp(BaseID a, BaseID b)
{
	if (a > b) return 1;
	if (a < b) return -1;
	return 0;
}

int base_to_int_index(BaseID id)
{
	if (base_is_index(id)) {
		return (int)id;
	}
	return -1;
}

bool player_is_protected(PlayerUnitState state)
{
	// Legacy: state == PLAYER_STATE_ON_BASE || state == PLAYER_STATE_AT_BAT
	// Also including ADVANCING_FREELY (Safe Walk) as protected?
	// In legacy `is_actually_safe` check, it included SAFE_ON_BASE and AT_BAT.
	// ADVANCING_FREELY protects from Wounds/Tags but does it protect from Force Outs?
	// Rule 40 says force play happens. A free walker is forced to advance?
	// Actually, `is_runner_forced_out` takes `is_advancing_freely` as separate param.
	// So "protected" here strictly means "Safe at current base".
	return (state == PLAYER_STATE_ON_BASE || state == PLAYER_STATE_AT_BAT);
}

bool player_is_safe_from_fly(PlayerUnitState state, BaseID current_base, BaseID original_base)
{
	// If taking free walk, always safe from fly (rule 36/31?)
	if (state == PLAYER_STATE_ADVANCING_FREELY) return true;

	// Legacy logic: "if not wound... if ball out of base... its a wound"
	// "its also wound if the player has arrived the next base already."

	// Player is safe if they are at their original base.
	// "Mikäli sisäpelaaja on irti pesästä..." -> If off base.

	// If current_base matches original_base, they are safe (haven't advanced past return point).
	// Note: This logic assumes they can return.

	// Simplified check based on game_analysis.c:
	// return false (not safe) if they have advanced.
	if (current_base != original_base) return false;

	// If they are physically safe on the original base, they are safe.
	if (state == PLAYER_STATE_ON_BASE || state == PLAYER_STATE_AT_BAT) return true;

	return false; // Off base
}

int count_active_batting_players(const PlayerInfo* players)
{
	int count = 0;
	for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		// Player is considered active on field if they have a valid base assignment.
		// This includes AT_BAT (Base 0), SAFE (Bases 1-3), RUNNING (between bases).
		// It excludes BASE_NONE (Out, Wounded/removed, Scored/removed).
		if (players[i].bTPI.baseId != BASE_NONE) {
			count++;
		}
	}
	return count;
}

// so a little function to check if ball's x and z coordinates indicate that ball is out of the
// playing field.
int checkIfBallIsOutOfBounds(BallInfo* ballInfo, FieldPositions* fieldPositions)
{
	int value = 1;
	// first, is ball behind the line at the back, or too much at right or too much at left
	// or in front of the homeline
	// if not, continue
	if(ballInfo->location.z > fieldPositions->backLeftPoint.z &&
	        ballInfo->location.x < fieldPositions->backRightPoint.x &&
	        ballInfo->location.x > fieldPositions->backLeftPoint.x &&
	        ballInfo->location.z < HOME_LINE_Z) {
		float z0 = 1.0f;
		float x = fieldPositions->rightPoint.x;
		float z = fieldPositions->rightPoint.z - z0;
		float slope1 = z/x;
		float slope2;
		x = -fieldPositions->leftPoint.x;
		z = -(fieldPositions->leftPoint.z - z0);
		slope2 = z/x;
		// then here we just have basic line equations to check if the ball is
		// out or inside the lines from pitchPlate to rightPoint and leftPoint.
		if(ballInfo->location.z - slope1*ballInfo->location.x - z0 < 0 &&
		        ballInfo->location.z - slope2*ballInfo->location.x - z0 < 0) {
			value = 0;
		}
	}
	return value;
}

int is_run_of_honor_possible(const MatchSession* match)
{
	// Simplified version: Check if ball has been at 3rd base since pitch started
	// If ball reached 3rd base, run of honor is no longer possible
	if (match->referee.ballInThirdBaseSincePitch) {
		return 0;
	}

	// Check if any batter (player who started at HOME) is still advancing
	// and hasn't already scored run of honor
	for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		const PlayerInfo* player = &match->playerInfo[i];
		if (player->bTPI.baseId == BASE_NONE) continue;

		// Check if player is a batter who could still make run of honor
		// 1. At least at 2nd base (so 2nd or 3rd)
		// 2. Started at Home (Batter)
		// 3. Hasn't already scored run of honor
		if (base_is_at_least(player->bTPI.baseId, BASE_SECOND) &&
		        match->referee.battingPlayers[i].baseAtPitchStart == BASE_HOME &&
		        !match->referee.battingPlayers[i].runOfHonorScored) {
			return 1;
		}
	}
	return 0;
}
