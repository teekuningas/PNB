#include "state_validator.h"
#include <stdio.h>
#include "base_control.h"

// Global flag to prevent recursive/redundant validation loops if we're already handling a crash

#include <stdlib.h>

static char g_dumpPath[256] = {0};
static int g_isActive = 0;

void StateValidator_Init(const char* jsonPath)
{
	if (jsonPath) {
		snprintf(g_dumpPath, sizeof(g_dumpPath), "%s", jsonPath);
		g_isActive = 1;
	} else {
		g_isActive = 0;
	}
}

void StateValidator_SetActive(int active)
{
	g_isActive = active;
}

static const char* base_to_string(BaseID id)
{
	switch(id) {
	case BASE_HOME:
		return "HOME";
	case BASE_FIRST:
		return "1ST";
	case BASE_SECOND:
		return "2ND";
	case BASE_THIRD:
		return "3RD";
	case BASE_HOME_SCORED:
		return "SCORED";
	case BASE_NONE:
		return "NONE";
	default:
		return "UNKNOWN";
	}
}

static const char* state_to_string(PlayerUnitState s)
{
	switch(s) {
	case PLAYER_STATE_IDLE:
		return "IDLE";
	case PLAYER_STATE_AT_BAT:
		return "AT_BAT";
	case PLAYER_STATE_SAFE_ON_BASE:
		return "SAFE";
	case PLAYER_STATE_RUNNING:
		return "RUNNING";
	case PLAYER_STATE_ADVANCING_FREELY:
		return "FREE_WALK";
	case PLAYER_STATE_LEADING:
		return "LEADING";
	case PLAYER_STATE_OUT:
		return "OUT";
	case PLAYER_STATE_WOUNDED:
		return "WOUNDED";
	case PLAYER_STATE_SCORED:
		return "SCORED_STATE";
	default:
		return "UNKNOWN";
	}
}

static void dump_state(StateInfo* state, const char* reason)
{
	if (!g_dumpPath[0]) return;

	FILE* f = fopen(g_dumpPath, "w");
	if (!f) {
		fprintf(stderr, "Failed to open dump file: %s\n", g_dumpPath);
		return;
	}

	LocalGameInfo* game = state->localGameInfo;

	fprintf(f, "{\n");
	fprintf(f, "  \"failure_reason\": \"%s\",\n", reason);

	// Game Global State
	fprintf(f, "  \"gameState\": {\n");
	fprintf(f, "    \"outs\": %d,\n", game->gameState.outs);
	fprintf(f, "    \"balls\": %d,\n", game->gameState.balls);
	fprintf(f, "    \"strikes\": %d,\n", game->gameState.strikes);
	fprintf(f, "    \"runsInTheInning\": %d,\n", game->gameState.runsInTheInning);
	fprintf(f, "    \"ballHome\": %s,\n", game->gameState.ballHome ? "true" : "false");
	fprintf(f, "    \"outOfBounds\": %s\n", game->gameState.outOfBounds ? "true" : "false");
	fprintf(f, "  },\n");

	// Ball Info
	fprintf(f, "  \"ball\": {\n");
	fprintf(f, "    \"x\": %.2f, \"y\": %.2f, \"z\": %.2f,\n", game->ballInfo.location.x, game->ballInfo.location.y, game->ballInfo.location.z);
	fprintf(f, "    \"hasHitGround\": %s,\n", game->ballInfo.hasHitGround ? "true" : "false");
	fprintf(f, "    \"carrierIndex\": %d\n", game->pII.hasBallIndex);
	fprintf(f, "  },\n");

	// Players
	fprintf(f, "  \"players\": [\n");
	for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		PlayerInfo* p = &game->playerInfo[i];
		// Only print relevant players
		if (p->bTPI.baseId != BASE_NONE || p->bTPI.state != PLAYER_STATE_IDLE) {
			fprintf(f, "    {\n");
			fprintf(f, "      \"id\": %d,\n", i);
			fprintf(f, "      \"name\": \"%s\",\n", p->bTPI.name ? p->bTPI.name : "Unknown");
			fprintf(f, "      \"baseId\": %d,\n", p->bTPI.baseId);
			fprintf(f, "      \"baseStr\": \"%s\",\n", base_to_string(p->bTPI.baseId));
			fprintf(f, "      \"state\": %d,\n", p->bTPI.state);
			fprintf(f, "      \"stateStr\": \"%s\",\n", state_to_string(p->bTPI.state));
			fprintf(f, "      \"x\": %.2f, \"z\": %.2f\n", p->tPI.location.x, p->tPI.location.z);
			fprintf(f, "    },\n");
		}
	}
	// Remove last comma hack
	fprintf(f, "    {\"id\": -1}\n");
	fprintf(f, "  ],\n");

	// Control Indices (The Truth)
	fprintf(f, "  \"batterIndex\": %d,\n", game->pII.batterIndex);
	// Referee State (Logic Internals)
	fprintf(f, "  \"referee\": {\n");
	fprintf(f, "    \"woundingCatchActive\": %s,\n", game->referee.woundingCatchActive ? "true" : "false");
	fprintf(f, "    \"pendingWounds\": [\n");
	for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		if (game->referee.battingPlayers[i].hasPendingWound) {
			fprintf(f, "      {\"id\": %d, \"type\": %d, \"sourceBase\": %d},\n",
			        i, game->referee.battingPlayers[i].woundingType, game->referee.battingPlayers[i].woundingSourceBase);
		}
	}
	fprintf(f, "      {}\n");
	fprintf(f, "    ]\n");
	fprintf(f, "  }\n");

	fprintf(f, "}\n");
	fclose(f);
	printf("State dumped to %s\n", g_dumpPath);
}

void StateValidator_Check(StateInfo* state)
{
	if (!g_isActive) return;

	LocalGameInfo* game = state->localGameInfo;

	// Skip validation if the game is already paused (avoid redundant checks/dumps)
	if (game->gameControl.pause) return;

	// Invariant 1: Unique Base Occupancy (Safe players)
	// Check baseControlIndex vs Player state
	for (int b = 0; b < BASE_COUNT; b++) {
		int idx = get_base_controller(game, b);
		if (idx != -1) {
			// Player at this index MUST be at this base
			if (game->playerInfo[idx].bTPI.baseId != (BaseID)b) {
				printf("\n[STATE ERROR] FATAL: Base %d controlled by %d, but player is at base %d\n", b, idx, game->playerInfo[idx].bTPI.baseId);
				dump_state(state, "Base Controller Mismatch");
				game->gameControl.pause = 1; // Pause game
				return;
			}

			// Player MUST be safe or at bat (if base 0)
			// UPDATED JAN 2026: Player can also be RUNNING/LEADING/FREE_WALK from this base while still being controlled by it
			// as long as they haven't arrived at the next base.
			if (game->playerInfo[idx].bTPI.state == PLAYER_STATE_OUT ||
			        game->playerInfo[idx].bTPI.state == PLAYER_STATE_WOUNDED ||
			        game->playerInfo[idx].bTPI.state == PLAYER_STATE_SCORED ||
			        game->playerInfo[idx].bTPI.state == PLAYER_STATE_IDLE) {
				printf("\n[STATE ERROR] FATAL: Base %d controlled by %d, but player state is %s (Invalid for controller)\n",
				       b, idx, state_to_string(game->playerInfo[idx].bTPI.state));
				dump_state(state, "Base Controller State Invalid");
				game->gameControl.pause = 1;
				return;
			}
		}
	}

	// Invariant 2: Reverse check (Players vs Control Index)
	for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		if (game->playerInfo[i].bTPI.state == PLAYER_STATE_SAFE_ON_BASE) {
			int b = game->playerInfo[i].bTPI.baseId;
			if (b >= 0 && b < BASE_COUNT) {
				if (get_base_controller(game, b) != i) {
					printf("\n[STATE ERROR] FATAL: Player %d is SAFE at base %d, but base controller is %d\n", i, b, get_base_controller(game, b));
					dump_state(state, "Safe Player Not Controlling Base");
					game->gameControl.pause = 1;
					return;
				}
			}
		}
	}
}