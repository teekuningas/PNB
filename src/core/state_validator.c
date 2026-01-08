#include "state_validator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "base_control.h"

// History Buffer Settings
#define HISTORY_SIZE 100
#define SNAPSHOT_LABEL_LEN 32

typedef struct {
    char label[SNAPSHOT_LABEL_LEN];
    int frameCount; // Assuming we can get this, or just sequence ID
    LocalGameInfo snapshot;
} GameSnapshot;

// Global State
static char g_dumpPath[256] = {0};
static int g_isActive = 0;

static GameSnapshot g_history[HISTORY_SIZE];
static int g_historyHead = 0;
static int g_historyCount = 0;
static int g_sequenceId = 0;

void StateValidator_Init(const char* jsonPath)
{
	if (jsonPath) {
		snprintf(g_dumpPath, sizeof(g_dumpPath), "%s", jsonPath);
		g_isActive = 1;
		printf("[StateValidator] Debug logging enabled. Dump path: %s\n", g_dumpPath);
	} else {
		g_isActive = 0;
	}
	g_historyHead = 0;
	g_historyCount = 0;
	g_sequenceId = 0;
}

void StateValidator_SetActive(int active)
{
	g_isActive = active;
}

void StateValidator_CaptureSnapshot(StateInfo* state, const char* label)
{
	if (!g_isActive) return;

	// Copy current state to history buffer
	int idx = g_historyHead;
	snprintf(g_history[idx].label, SNAPSHOT_LABEL_LEN, "%s", label);
	g_history[idx].frameCount = g_sequenceId++;
	
	// Deep copy the local game info
	// Note: Strings (names) are pointers, but they usually point to static data or managed resources.
	// As long as we don't free them, copying the pointer is fine for a snapshot.
	memcpy(&g_history[idx].snapshot, state->localGameInfo, sizeof(LocalGameInfo));

	// Advance head
	g_historyHead = (g_historyHead + 1) % HISTORY_SIZE;
	if (g_historyCount < HISTORY_SIZE) {
		g_historyCount++;
	}
}

static const char* base_to_string(BaseID id)
{
	switch(id) {
	case BASE_HOME: return "HOME";
	case BASE_FIRST: return "1ST";
	case BASE_SECOND: return "2ND";
	case BASE_THIRD: return "3RD";
	case BASE_HOME_SCORED: return "SCORED";
	case BASE_NONE: return "NONE";
	default: return "UNKNOWN";
	}
}

static const char* state_to_string(PlayerUnitState s)
{
	switch(s) {
	case PLAYER_STATE_IDLE: return "IDLE";
	case PLAYER_STATE_AT_BAT: return "AT_BAT";
	case PLAYER_STATE_SAFE_ON_BASE: return "SAFE";
	case PLAYER_STATE_RUNNING: return "RUNNING";
	case PLAYER_STATE_ADVANCING_FREELY: return "FREE_WALK";
	case PLAYER_STATE_LEADING: return "LEADING";
	case PLAYER_STATE_OUT: return "OUT";
	case PLAYER_STATE_WOUNDED: return "WOUNDED";
	case PLAYER_STATE_SCORED: return "SCORED_STATE";
	default: return "UNKNOWN";
	}
}

static void print_game_json(FILE* f, LocalGameInfo* game, int indent)
{
	// Helper for indentation
	char sp[16];
	int i;
	for(i=0; i<indent && i<15; i++) sp[i] = ' ';
	sp[i] = 0;

	fprintf(f, "%s\"gameState\": {\n", sp);
	fprintf(f, "%s  \"outs\": %d,\n", sp, game->gameState.outs);
	fprintf(f, "%s  \"runsInTheInning\": %d,\n", sp, game->gameState.runsInTheInning);
	fprintf(f, "%s  \"strikes\": %d,\n", sp, game->gameState.strikes);
	fprintf(f, "%s  \"balls\": %d\n", sp, game->gameState.balls);
	fprintf(f, "%s},\n", sp);

	fprintf(f, "%s\"batterIndex\": %d,\n", sp, game->pII.batterIndex);
	
	fprintf(f, "%s\"players\": [\n", sp);
	for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		PlayerInfo* p = &game->playerInfo[i];
		// Only print relevant players (on base, active, or pending wound)
		int isRelevant = (p->bTPI.baseId != BASE_NONE || p->bTPI.state != PLAYER_STATE_IDLE || 
		                  game->referee.battingPlayers[i].hasPendingWound);
		
		if (isRelevant) {
			fprintf(f, "%s  {\n", sp);
			fprintf(f, "%s    \"id\": %d,\n", sp, i);
			fprintf(f, "%s    \"baseId\": %d,\n", sp, p->bTPI.baseId);
			fprintf(f, "%s    \"baseStr\": \"%s\",\n", sp, base_to_string(p->bTPI.baseId));
			fprintf(f, "%s    \"state\": \"%s\",\n", sp, state_to_string(p->bTPI.state));
			
			// Referee State
			fprintf(f, "%s    \"ref_safetyBase\": %d,\n", sp, game->referee.battingPlayers[i].currentSafetyBase);
			fprintf(f, "%s    \"ref_safetyBaseStr\": \"%s\",\n", sp, base_to_string(game->referee.battingPlayers[i].currentSafetyBase));
			fprintf(f, "%s    \"ref_pendingWound\": %d\n", sp, game->referee.battingPlayers[i].hasPendingWound);
			
			fprintf(f, "%s  },\n", sp);
		}
	}
	// Hack to close array validly
	fprintf(f, "%s  {\"id\": -1}\n", sp);
	fprintf(f, "%s]", sp);
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
	fprintf(f, "  \"timestamp\": %d,\n", g_sequenceId);

	// Current State
	fprintf(f, "  \"currentState\": {\n");
	print_game_json(f, game, 4);
	fprintf(f, "\n  },\n");

	// History
	fprintf(f, "  \"history\": [\n");
	
	// Iterate from oldest to newest
	// Oldest is at head (if full) or 0 (if not full)?
	// Ring buffer: head points to NEXT write slot. So oldest is at head (if full).
	// But it's easier to just iterate sequentially and handle wrap.
	
	int startIdx = (g_historyCount < HISTORY_SIZE) ? 0 : g_historyHead;
	for (int i = 0; i < g_historyCount; i++) {
		int idx = (startIdx + i) % HISTORY_SIZE;
		fprintf(f, "    {\n");
		fprintf(f, "      \"seq\": %d,\n", g_history[idx].frameCount);
		fprintf(f, "      \"label\": \"%s\",\n", g_history[idx].label);
		fprintf(f, "      \"snapshot\": {\n");
		print_game_json(f, &g_history[idx].snapshot, 8);
		fprintf(f, "\n      }\n");
		fprintf(f, "    }%s\n", (i < g_historyCount - 1) ? "," : "");
	}
	
	fprintf(f, "  ]\n");
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
