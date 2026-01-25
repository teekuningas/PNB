#include "state_validator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "base_control.h"
#include "rules_pure/player_utils.h"

// History Buffer Settings
#define HISTORY_SIZE 100
#define SNAPSHOT_LABEL_LEN 32

typedef struct {
	char label[SNAPSHOT_LABEL_LEN];
	int frameCount; // Assuming we can get this, or just sequence ID
	MatchSession snapshot;
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
	memcpy(&g_history[idx].snapshot, state->match, sizeof(MatchSession));

	// Advance head
	g_historyHead = (g_historyHead + 1) % HISTORY_SIZE;
	if (g_historyCount < HISTORY_SIZE) {
		g_historyCount++;
	}
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
	case PLAYER_STATE_ON_BASE:
		return "ON_BASE";
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

static void print_game_json(FILE* f, MatchSession* game, Scoreboard* global, int indent)
{
	// Helper for indentation
	char sp[16];
	int i;
	for(i=0; i<indent &&i<15; i++) sp[i] = ' ';
	sp[i] = 0;

	if (global) {
		fprintf(f, "%s\"global\": {\n", sp);
		fprintf(f, "%s  \"inning\": %d,\n", sp, global->inning);
		fprintf(f, "%s  \"period\": %d,\n", sp, global->period);
		fprintf(f, "%s  \"team0_runs\": %d,\n", sp, global->teams[0].runs);
		fprintf(f, "%s  \"team1_runs\": %d\n", sp, global->teams[1].runs);
		fprintf(f, "%s},\n", sp);
	}

	fprintf(f, "%s\"halfInningState\": {\n", sp);
	fprintf(f, "%s  \"outs\": %d,\n", sp, game->halfInningState.outs);
	fprintf(f, "%s  \"runsInTheInning\": %d,\n", sp, game->halfInningState.runsInTheInning);
	fprintf(f, "%s  \"strikes\": %d,\n", sp, game->halfInningState.strikes);
	fprintf(f, "%s  \"balls\": %d,\n", sp, game->halfInningState.balls);
	fprintf(f, "%s  \"event\": %d\n", sp, game->halfInningState.event);
	fprintf(f, "%s},\n", sp);

	fprintf(f, "%s\"gameControl\": {\n", sp);
	fprintf(f, "%s  \"pause\": %d,\n", sp, game->flowControl.pause);
	fprintf(f, "%s  \"waitingForBatterDecision\": %d,\n", sp, game->flowControl.waitingForBatterDecision);
	fprintf(f, "%s  \"waitingForFreeWalkDecision\": %d,\n", sp, game->flowControl.waitingForFreeWalkDecision);
	fprintf(f, "%s  \"catchHasBeenMade\": %d,\n", sp, game->betweenPitchState.catchHasBeenMade);
	fprintf(f, "%s  \"hasBallHitGround\": %d,\n", sp, game->betweenPitchState.hasBallHitGround);
	fprintf(f, "%s  \"foulState\": %d\n", sp, (int)game->betweenPitchState.foulState);
	fprintf(f, "%s },\n", sp);


	fprintf(f, "%s\"gameEvents\": {\n", sp);
	fprintf(f, "%s  \"catchMade\": %d,\n", sp, game->gameEvents.catchMade);
	fprintf(f, "%s  \"playerArrivedAtBase\": %d,\n", sp, game->gameEvents.playerArrivedAtBase);
	fprintf(f, "%s  \"ballHitGround\": %d,\n", sp, game->gameEvents.ballHitGround);
	fprintf(f, "%s  \"ballHitByBat\": %d,\n", sp, game->gameEvents.ballHitByBat);
	fprintf(f, "%s  \"pitchReleased\": %d\n", sp, game->gameEvents.pitchReleased);
	fprintf(f, "%s},\n", sp);

	fprintf(f, "%s\"gameFlowState\": {\n", sp);
	fprintf(f, "%s  \"ballHome\": %d,\n", sp, game->gameFlowState.ballHome);
	fprintf(f, "%s  \"endOfInningCounter\": %d\n", sp, game->gameFlowState.endOfInningCounter);
	fprintf(f, "%s},\n", sp);

	fprintf(f, "%s\"pII\": {\n", sp);
	fprintf(f, "%s  \"batterIndex\": %d,\n", sp, get_active_batter_index(game));
	fprintf(f, "%s  \"batterSelectionIndex\": %d,\n", sp, game->pII.batterSelectionIndex);
	fprintf(f, "%s  \"hasBallIndex\": %d,\n", sp, game->pII.hasBallIndex);
	fprintf(f, "%s  \"controlIndex\": %d\n", sp, game->pII.controlIndex);
	fprintf(f, "%s},\n", sp);

	fprintf(f, "%s\"pRAI\": {\n", sp);
	fprintf(f, "%s  \"pitchState\": %d,\n", sp, game->pRAI.pitchState);
	fprintf(f, "%s  \"batterReady\": %d,\n", sp, game->pRAI.batterReady);
	fprintf(f, "%s  \"battingGoingOn\": %d,\n", sp, game->pRAI.battingGoingOn);
	fprintf(f, "%s  \"batHit\": %d,\n", sp, game->pRAI.batHit);
	fprintf(f, "%s  \"initBatter\": %d\n", sp, game->pRAI.initBatter);
	fprintf(f, "%s},\n", sp);

	fprintf(f, "%s\"ballInfo\": {\n", sp);
	fprintf(f, "%s  \"location\": { \"x\": %.2f, \"y\": %.2f, \"z\": %.2f },\n", sp, game->ballInfo.location.x, game->ballInfo.location.y, game->ballInfo.location.z);
	fprintf(f, "%s  \"moving\": %d,\n", sp, game->ballInfo.moving);
	fprintf(f, "%s  \"hasHitGround\": %d,\n", sp, game->ballInfo.currentFlightHasHitGround);
	fprintf(f, "%s  \"onGround\": %d\n", sp, game->ballInfo.onGround);
	fprintf(f, "%s},\n", sp);

	fprintf(f, "%s\"aiState\": {\n", sp);
	fprintf(f, "%s  \"actionKeyLock\": %d,\n", sp, game->aiState.actionKeyLock);
	fprintf(f, "%s  \"battingKeyDown\": %d,\n", sp, game->aiState.battingKeyDown);
	fprintf(f, "%s  \"planCalculated\": %d,\n", sp, game->aiState.planCalculated);
	fprintf(f, "%s  \"pitchStage\": %d\n", sp, game->aiState.pitchStage);
	fprintf(f, "%s},\n", sp);

	fprintf(f, "%s\"players\": [\n", sp);
	int activeBatterIndex = get_active_batter_index(game);
	for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		PlayerInfo* p = &game->playerInfo[i];
		// Only print relevant players (on base, active, pending wound, OR holding safety)
		int isRelevant = (p->bTPI.baseId != BASE_NONE || p->bTPI.state != PLAYER_STATE_IDLE ||
		                  game->referee.battingPlayers[i].status != PLAYER_STATUS_ACTIVE || i == activeBatterIndex ||
		                  game->referee.battingPlayers[i].currentSafetyBase != BASE_NONE);

		if (isRelevant) {
			fprintf(f, "%s  {\n", sp);
			fprintf(f, "%s    \"id\": %d,\n", sp, i);
			fprintf(f, "%s    \"baseId\": %d,\n", sp, p->bTPI.baseId);
			fprintf(f, "%s    \"baseStr\": \"%s\",\n", sp, base_to_string(p->bTPI.baseId));
			fprintf(f, "%s    \"state\": \"%s\",\n", sp, state_to_string(p->bTPI.state));
			fprintf(f, "%s    \"pos\": { \"x\": %.2f, \"z\": %.2f },\n", sp, p->tPI.location.x, p->tPI.location.z);

			// Runtime state
			fprintf(f, "%s    \"runtime\": { \"goingForward\": %d, \"arrived\": %d },\n", sp, game->playerRuntime[i].goingForward, game->playerRuntime[i].arrivedToBase);

			// Referee State
			fprintf(f, "%s    \"ref_safetyBase\": %d,\n", sp, game->referee.battingPlayers[i].currentSafetyBase);
			fprintf(f, "%s    \"ref_safetyBaseStr\": \"%s\",\n", sp, base_to_string(game->referee.battingPlayers[i].currentSafetyBase));
			fprintf(f, "%s    \"ref_baseAtPitchStart\": %d,\n", sp, game->referee.battingPlayers[i].baseAtPitchStart);
			fprintf(f, "%s    \"ref_status\": %d\n", sp, game->referee.battingPlayers[i].status);

			fprintf(f, "%s  },\n", sp);
		}
	}
	// Hack to close array validly
	fprintf(f, "%s  {\"id\": -1}\n", sp);
	fprintf(f, "%s]", sp);
}

void StateValidator_Dump(StateInfo* state, const char* reason)
{
	if (!g_dumpPath[0]) return;

	FILE* f = fopen(g_dumpPath, "w");
	if (!f) {
		fprintf(stderr, "Failed to open dump file: %s\n", g_dumpPath);
		return;
	}

	MatchSession* game = state->match;

	fprintf(f, "{\n");
	fprintf(f, "  \"failure_reason\": \"%s\",\n", reason);
	fprintf(f, "  \"timestamp\": %d,\n", g_sequenceId);

	// Current State
	fprintf(f, "  \"currentState\": {\n");
	print_game_json(f, game, &state->match->scoreboard, 4);
	fprintf(f, "\n  },\n");

	// History
	fprintf(f, "  \"history\": [\n");

	int startIdx = (g_historyCount < HISTORY_SIZE) ? 0 : g_historyHead;
	for (int i = 0; i < g_historyCount; i++) {
		int idx = (startIdx + i) % HISTORY_SIZE;
		fprintf(f, "    {\n");
		fprintf(f, "      \"seq\": %d,\n", g_history[idx].frameCount);
		fprintf(f, "      \"label\": \"%s\",\n", g_history[idx].label);
		fprintf(f, "      \"snapshot\": {\n");
		print_game_json(f, &g_history[idx].snapshot, NULL, 8); // No global info for snapshots
		fprintf(f, "\n      }\n");
		fprintf(f, "    }%s\n", (i < g_historyCount - 1) ? "," : "");
	}

	fprintf(f, "  ]\n");
	fprintf(f, "}\n");
	fclose(f);
	printf("State dumped to %s\n", g_dumpPath);
}

int StateValidator_Check(StateInfo* state)
{
	if (!g_isActive) return 1;

	MatchSession* game = state->match;

	// Skip validation if the game is already paused (avoid redundant checks/dumps)
	if (game->flowControl.pause) return 1;

	// Invariant 1: Unique Base Occupancy (Safe players)
	// Check baseControlIndex vs Player state
	for (int b = 0; b < BASE_COUNT; b++) {
		int idx = get_base_controller(game, b);
		if (idx != -1) {
			// Player at this index MUST be at this base (enforced by get_base_controller definition)
			if (game->playerInfo[idx].bTPI.baseId != (BaseID)b) {
				printf("\n[STATE ERROR] FATAL: Base %d controlled by %d, but player is at base %d\n", b, idx, game->playerInfo[idx].bTPI.baseId);
				return 0; // Invalid
			}

			// Player MUST not be in invalid states for controlling a base
			if (game->playerInfo[idx].bTPI.state == PLAYER_STATE_OUT ||
			        game->playerInfo[idx].bTPI.state == PLAYER_STATE_WOUNDED ||
			        game->playerInfo[idx].bTPI.state == PLAYER_STATE_SCORED ||
			        game->playerInfo[idx].bTPI.state == PLAYER_STATE_IDLE) {
				printf("\n[STATE ERROR] FATAL: Base %d controlled by %d, but player state is %s (Invalid for controller)\n",
				       b, idx, state_to_string(game->playerInfo[idx].bTPI.state));
				return 0; // Invalid
			}
		}
	}

	// Invariant 2: Batter Consistency
	int activeBatterCount = 0;
	for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		if (game->playerInfo[i].bTPI.state == PLAYER_STATE_AT_BAT) {
			activeBatterCount++;

			// Must be at home
			if (game->playerInfo[i].bTPI.baseId != BASE_HOME) {
				printf("\n[STATE ERROR] FATAL: Player %d is AT_BAT but at base %d (Must be BASE_HOME)\n", i, game->playerInfo[i].bTPI.baseId);
				return 0;
			}
		}
	}

	if (activeBatterCount > 1) {
		printf("\n[STATE ERROR] FATAL: Multiple active batters found (%d)\n", activeBatterCount);
		return 0;
	}

	return 1; // Valid
}
