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

void state_validator_init(const char* jsonPath)
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

void state_validator_set_active(int active)
{
    g_isActive = active;
}

void state_validator_capture_snapshot(StateInfo* state, const char* label)
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
    switch (id) {
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
    switch (s) {
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

static void print_game_json(FILE* f, MatchSession* game, GameRulesState* rules, int indent)
{
    // Helper for indentation
    char sp[16];
    int i;
    for (i = 0; i < indent && i < 15; i++)
        sp[i] = ' ';
    sp[i] = 0;

    if (rules) {
        const Scoreboard* scoreboard = &rules->scoreboard;
        fprintf(f, "%s\"global\": {\n", sp);
        fprintf(f, "%s  \"inning\": %d,\n", sp, scoreboard->inning);
        fprintf(f, "%s  \"period\": %d,\n", sp, scoreboard->period);
        fprintf(f, "%s  \"pairCount\": %d,\n", sp, scoreboard->pairCount);
        fprintf(f, "%s  \"team0_runs\": %d,\n", sp, scoreboard->teams[0].runs);
        fprintf(f, "%s  \"team1_runs\": %d,\n", sp, scoreboard->teams[1].runs);

        // Detailed Team Info (Batter Orders)
        for (int t = 0; t < 2; t++) {
            fprintf(f, "%s  \"team%d_batterOrderIndex\": %d,\n", sp, t, scoreboard->teams[t].batterOrderIndex);
            fprintf(f, "%s  \"team%d_batterOrder\": [", sp, t);
            for (int bo = 0; bo < 12; bo++) {
                fprintf(f, "%d%s", scoreboard->teams[t].batterOrder[bo], (bo < 11) ? ", " : "");
            }
            fprintf(f, "],\n");
        }
        fprintf(
            f, "%s  \"runs\": [%d, %d]\n", sp, scoreboard->teams[0].runs, scoreboard->teams[1].runs
        ); // Redundant but convenient
        fprintf(f, "%s},\n", sp);
    }

    if (rules) {
        fprintf(f, "%s\"halfInningState\": {\n", sp);
        fprintf(f, "%s  \"outs\": %d,\n", sp, rules->halfInningState.outs);
        fprintf(f, "%s  \"runsInTheInning\": %d,\n", sp, rules->halfInningState.runsInTheInning);
        fprintf(f, "%s  \"strikes\": %d,\n", sp, rules->halfInningState.strikes);
        fprintf(f, "%s  \"balls\": %d,\n", sp, rules->halfInningState.balls);
        fprintf(f, "%s  \"event\": %d\n", sp, rules->halfInningState.event);
        fprintf(f, "%s},\n", sp);

        fprintf(f, "%s\"lastBatter\": {\n", sp);
        fprintf(f, "%s  \"jokersLeft\": %d,\n", sp, rules->halfInningState.jokersLeft);
        fprintf(f, "%s  \"designatedIndex\": %d,\n", sp, rules->halfInningState.lastBatter.designatedIndex);
        fprintf(f, "%s  \"lastRegularIndex\": %d,\n", sp, rules->halfInningState.lastBatter.lastRegularIndex);
        fprintf(f, "%s  \"hasBattedAgain\": %d,\n", sp, rules->halfInningState.lastBatter.hasBattedAgain);
        fprintf(f, "%s  \"turnExhausted\": %d\n", sp, rules->halfInningState.lastBatter.turnExhausted);
        fprintf(f, "%s},\n", sp);

        fprintf(f, "%s\"gameControl\": {\n", sp);
        fprintf(f, "%s  \"pause\": %d,\n", sp, game->flowControl.pause);
        fprintf(f, "%s  \"waitingForBatterDecision\": %d,\n", sp, game->flowControl.waitingForBatterDecision);
        fprintf(f, "%s  \"waitingForFreeWalkDecision\": %d,\n", sp, game->flowControl.waitingForFreeWalkDecision);
        fprintf(f, "%s  \"catchHasBeenMade\": %d,\n", sp, rules->betweenPitchState.catchHasBeenMade);
        fprintf(f, "%s  \"hasBallHitGround\": %d,\n", sp, rules->betweenPitchState.hasBallHitGround);
        fprintf(f, "%s  \"foulState\": %d\n", sp, (int)rules->referee.foulState);
        fprintf(f, "%s },\n", sp);
    }

    fprintf(f, "%s\"gameEvents\": {\n", sp);
    fprintf(f, "%s  \"catchMade\": %d,\n", sp, game->gameEvents.catchMade);
    fprintf(f, "%s  \"playerArrivedAtBase\": %d,\n", sp, game->gameEvents.playerArrivedAtBase);
    fprintf(f, "%s  \"ballHitGround\": %d,\n", sp, game->gameEvents.ballHitGround);
    fprintf(f, "%s  \"ballHitByBat\": %d,\n", sp, game->gameEvents.ballHitByBat);
    fprintf(f, "%s  \"pitchReleased\": %d,\n", sp, game->gameEvents.pitchReleased);
    fprintf(f, "%s  \"batterEntered\": %d\n", sp, game->gameEvents.batterEntered);
    fprintf(f, "%s},\n", sp);

    fprintf(f, "%s\"gameFlowState\": {\n", sp);
    fprintf(f, "%s  \"ballHome\": %d", sp, game->gameFlowState.ballHome);
    if (rules) {
        fprintf(f, ",\n%s  \"endOfInningState\": %d\n", sp, (int)rules->referee.endOfInningState);
    } else {
        fprintf(f, "\n");
    }
    fprintf(f, "%s},\n", sp);

    fprintf(f, "%s\"pII\": {\n", sp);
    fprintf(f, "%s  \"batterIndex\": %d,\n", sp, get_active_batter_index(game));
    fprintf(f, "%s  \"hasBallIndex\": %d,\n", sp, game->pII.hasBallIndex);
    fprintf(f, "%s  \"controlIndex\": %d,\n", sp, game->pII.controlIndex);
    fprintf(f, "%s  \"catcherOnBaseIndex0\": %d,\n", sp, game->pII.catcherOnBaseIndex[0]);
    fprintf(
        f, "%s  \"isNearHome_pitcher\": %d\n", sp,
        game->playerInfo[game->pII.catcherOnBaseIndex[0]].cTPI.isNearHomeLocation
    );
    fprintf(f, "%s},\n", sp);

    fprintf(f, "%s\"pRAI\": {\n", sp);
    fprintf(f, "%s  \"pitch_state\": %d,\n", sp, game->pRAI.pitch_state);
    fprintf(f, "%s  \"batter_ready\": %d,\n", sp, game->pRAI.batter_ready);
    fprintf(f, "%s  \"batting_going_on\": %d,\n", sp, game->pRAI.batting_going_on);
    fprintf(f, "%s  \"init_batter\": %d,\n", sp, game->pRAI.init_batter);
    fprintf(f, "%s  \"throw_going_to_base\": %d\n", sp, game->pRAI.throw_going_to_base);
    fprintf(f, "%s},\n", sp);

    fprintf(f, "%s\"actionState\": {\n", sp);
    fprintf(f, "%s  \"current_catching_action\": %d,\n", sp, (int)game->pendingActionState.current_catching_action);
    fprintf(f, "%s  \"pitch_decl_phase\": %d,\n", sp, (int)game->pendingActionState.pitchDeclaration.phase);
    fprintf(f, "%s  \"pitch_timer\": %d,\n", sp, game->pendingActionState.pitchActualization.timer);
    // The drop / change-player commands used to be dumped here as persistent flags. They are messages
    // now: declared and consumed inside one tick, so at the frame boundary this dump is taken there is
    // nothing left of them to print — what a reader wants instead is the world they produced.
    // The swing, as the engine holds it: what has been declared and the frame it will be used on.
    // No phase and no meter — there is nothing here a producer drives forward.
    fprintf(f, "%s  \"swing_contact_frame\": %d,\n", sp, game->pendingActionState.swing.contactFrame);
    fprintf(f, "%s  \"swing_power\": %s,\n", sp, game->pendingActionState.swing.powerActive ? "true" : "false");
    fprintf(f, "%s  \"swing_vertical\": %s\n", sp, game->pendingActionState.swing.verticalActive ? "true" : "false");
    fprintf(f, "%s},\n", sp);

    if (rules) {
        fprintf(f, "%s\"betweenPitchState\": {\n", sp);
        fprintf(f, "%s  \"catchHasBeenMade\": %d,\n", sp, rules->betweenPitchState.catchHasBeenMade);
        fprintf(f, "%s  \"hasBallHitGround\": %d,\n", sp, rules->betweenPitchState.hasBallHitGround);
        fprintf(f, "%s  \"pitchResult\": %d,\n", sp, rules->betweenPitchState.pitchResult);
        fprintf(f, "%s  \"batOutcome\": %d\n", sp, rules->betweenPitchState.batOutcome);
        fprintf(f, "%s},\n", sp);
    }

    fprintf(f, "%s\"ballInfo\": {\n", sp);
    fprintf(
        f, "%s  \"location\": { \"x\": %.2f, \"y\": %.2f, \"z\": %.2f },\n", sp, game->ballInfo.location.x,
        game->ballInfo.location.y, game->ballInfo.location.z
    );
    fprintf(f, "%s  \"moving\": %d,\n", sp, game->ballInfo.moving);
    fprintf(f, "%s  \"hasHitGround\": %d,\n", sp, game->ballInfo.currentFlightHasHitGround);
    fprintf(f, "%s  \"onGround\": %d\n", sp, game->ballInfo.onGround);
    fprintf(f, "%s},\n", sp);

    fprintf(f, "%s\"aiState\": {\n", sp);
    fprintf(f, "%s  \"swingDecided\": %d,\n", sp, game->aiState.swingDecided);
    fprintf(f, "%s  \"planCalculated\": %d\n", sp, game->aiState.planCalculated);
    fprintf(f, "%s},\n", sp);

    fprintf(f, "%s\"rngSeed\": %u,\n", sp, game->rngSeed);

    fprintf(f, "%s\"players\": [\n", sp);
    int totalPlayers = 2 * PLAYERS_IN_TEAM + JOKER_COUNT; // 21
    for (int i = 0; i < totalPlayers; i++) {
        PlayerInfo* p = &game->playerInfo[i];

        fprintf(f, "%s  {\n", sp);
        fprintf(f, "%s    \"id\": %d,\n", sp, i);
        fprintf(f, "%s    \"team\": %d,\n", sp, p->cPI.team);
        fprintf(f, "%s    \"baseId\": %d,\n", sp, p->bTPI.baseId);
        fprintf(f, "%s    \"baseStr\": \"%s\",\n", sp, base_to_string(p->bTPI.baseId));
        fprintf(f, "%s    \"state\": \"%s\",\n", sp, state_to_string(p->bTPI.state));
        fprintf(f, "%s    \"pos\": { \"x\": %.2f, \"z\": %.2f },\n", sp, p->tPI.location.x, p->tPI.location.z);
        // Runtime state
        fprintf(
            f, "%s    \"runtime\": { \"goingForward\": %d, \"arrived\": %d }", sp, game->playerRuntime[i].goingForward,
            game->playerRuntime[i].arrivedToBase
        );

        // Referee State - Only valid for batting team (indices 0-11)
        // Assuming 0-11 are ALWAYS the batting team in the current frame
        if (rules && i < PLAYERS_IN_TEAM + JOKER_COUNT) {
            fprintf(f, ",\n");
            fprintf(f, "%s    \"ref_safetyBase\": %d,\n", sp, rules->referee.battingPlayers[i].currentSafetyBase);
            fprintf(
                f, "%s    \"ref_safetyBaseStr\": \"%s\",\n", sp,
                base_to_string(rules->referee.battingPlayers[i].currentSafetyBase)
            );
            fprintf(f, "%s    \"ref_baseAtPitchStart\": %d,\n", sp, rules->referee.battingPlayers[i].baseAtPitchStart);
            fprintf(f, "%s    \"ref_status\": %d\n", sp, rules->referee.battingPlayers[i].status);
        } else {
            fprintf(f, "\n");
        }

        fprintf(f, "%s  }%s\n", sp, (i < totalPlayers - 1) ? "," : "");
    }
    fprintf(f, "%s]", sp);
}

void state_validator_dump(StateInfo* state, const char* reason)
{
    if (!g_dumpPath[0]) return;

    FILE* f = fopen(g_dumpPath, "w");
    if (!f) {
        fprintf(stderr, "Failed to open dump file: %s\n", g_dumpPath);
        return;
    }

    MatchSession* game = state->match;
    GameRulesState* rules = state->rules;

    fprintf(f, "{\n");
    fprintf(f, "  \"failure_reason\": \"%s\",\n", reason);
    fprintf(f, "  \"timestamp\": %d,\n", g_sequenceId);

    // Current State
    fprintf(f, "  \"currentState\": {\n");
    print_game_json(f, game, rules, 4);
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

int state_validator_check(StateInfo* state)
{
    if (!g_isActive) return 1;

    MatchSession* game = state->match;
    GameRulesState* rules = state->rules;

    // Skip validation if the game is already paused (avoid redundant checks/dumps)
    if (game->flowControl.pause) return 1;

    // Invariant 1: Unique Base Occupancy (Safe players)
    // Check baseControlIndex vs Player state
    for (int b = 0; b < BASE_COUNT; b++) {
        int idx = get_base_controller(game, &rules->referee, b);
        if (idx != -1) {
            // Player at this index MUST be at this base (enforced by get_base_controller definition)
            if (game->playerInfo[idx].bTPI.baseId != (BaseID)b) {
                printf(
                    "\n[STATE ERROR] FATAL: Base %d controlled by %d, but player is at base %d\n", b, idx,
                    game->playerInfo[idx].bTPI.baseId
                );
                return 0; // Invalid
            }

            // Player MUST not be in invalid states for controlling a base
            if (game->playerInfo[idx].bTPI.state == PLAYER_STATE_OUT ||
                game->playerInfo[idx].bTPI.state == PLAYER_STATE_WOUNDED ||
                game->playerInfo[idx].bTPI.state == PLAYER_STATE_SCORED ||
                game->playerInfo[idx].bTPI.state == PLAYER_STATE_IDLE) {
                printf(
                    "\n[STATE ERROR] FATAL: Base %d controlled by %d, but player state is %s (Invalid for "
                    "controller)\n",
                    b, idx, state_to_string(game->playerInfo[idx].bTPI.state)
                );
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
                printf(
                    "\n[STATE ERROR] FATAL: Player %d is AT_BAT but at base %d (Must be BASE_HOME)\n", i,
                    game->playerInfo[i].bTPI.baseId
                );
                return 0;
            }
        }
    }

    if (activeBatterCount > 1) {
        printf("\n[STATE ERROR] FATAL: Multiple active batters found (%d)\n", activeBatterCount);
        return 0;
    }

    // Invariant 3: §7 — a joker the engine will still let you seat must be one the count still has
    // room for.
    //
    // "Joukkue voi käyttää jokaisessa sisävuorossa kolmea eri jokeripelaajaa, kerran kutakin" is held
    // in two places: each joker's own JOKER_USED, which the seating sets, and
    // `halfInningState.jokersLeft`, which the referee decrements on the batterEntered event. Two
    // writers, one rule, in two different stages of the frame.
    //
    // The direction is the load-bearing part. More AVAILABLE jokers than the count admits is the
    // failure that matters, and it is §12's third conjunct going wrong: the half-inning ends when
    // `jokersLeft` hits zero, but the candidate list a producer chooses from is built from the
    // per-player statuses — so a joker still marked available when the count has forgotten it means
    // a turn that ends with a batter the rules say the team may still use. Seating an already-spent
    // joker produces exactly that, because the status has nothing left to change while the referee
    // decrements regardless.
    //
    // The other direction is deliberately tolerated, because it is a real state and not a defect: at
    // a PERIOD boundary the referee clears jokersLeft to JOKER_COUNT, but the physical reset that
    // re-marks the players never runs — consolidation routes to a menu instead. So between periods
    // the count leads the statuses. Nothing reads either while that lasts. Recorded rather than
    // asserted away: it is the same two-representations hazard from the harmless side.
    //
    // This replaces the invariant that used to live here, which asked whether the engine's OFFER of
    // a next batter was legal. There is no offer any more: a producer names a player and the INGEST
    // gate refuses an illegal one (§27/§12/§7), which is a stronger place to hold the rule than an
    // end-of-frame assertion. Re-pointing that assertion at "no regular is SEATED while the order is
    // spent" was considered and rejected — it would repeat bug #9's mistake exactly, because seating
    // a regular advances the batting order, which clears `regularOrderSpent` in the same frame. The
    // assertion would be looking at a state that erases itself at the moment of the violation.
    // Ablated at the gate instead, in the contract tier, where the refusal actually happens.
    if (rules->scoreboard.period < 4) {
        int unusedJokers = 0;
        for (int i = 0; i < JOKER_COUNT; i++) {
            if (game->playerInfo[game->pII.jokerIndices[i]].bTPI.joker == JOKER_AVAILABLE) unusedJokers++;
        }
        if (unusedJokers > rules->halfInningState.jokersLeft) {
            printf(
                "\n[STATE ERROR] FATAL: §7 — %d jokers are still marked available, but "
                "halfInningState.jokersLeft says only %d remain: a half-inning could end with a joker "
                "the team may still use\n",
                unusedJokers, rules->halfInningState.jokersLeft
            );
            return 0;
        }
    }

    // Invariant 4: an open free-walk offer names a real player.
    //
    // §26 gives the walk to the lead runner, so the offer and the player it belongs to are one fact
    // and must be set together. They were not: the offer was raised whenever the ball count reached
    // the threshold, while calculate_free_walk leaves freeWalkIndex at -1 when the whole inner side
    // is off the field. That put a prompt on screen belonging to nobody, made the withdrawal beside
    // it read playerInfo[-1] every frame, and — because the lukkari may not pitch while an offer is
    // open — left the game waiting for an answer to a question about no one. Found by the owner in
    // play, 2026-08-28.
    //
    // Asserted here rather than defended at the read, because this is a resting state: it is true
    // between frames or it is not, and an assertion on it fires at the frame a future change breaks
    // the pairing rather than quietly papering over it.
    if (game->flowControl.waitingForFreeWalkDecision == 1 &&
        (game->flowControl.freeWalkIndex < 0 || game->flowControl.freeWalkIndex >= PLAYERS_IN_TEAM + JOKER_COUNT)) {
        printf(
            "\n[STATE ERROR] FATAL: a free walk is being offered, but freeWalkIndex is %d — §26's walk "
            "belongs to a player, so an offer that names nobody is not an offer\n",
            game->flowControl.freeWalkIndex
        );
        return 0;
    }

    // Invariant 5: the intent channels are empty at the frame boundary.
    //
    // This is what makes "the channel is a parameter of the tick, not state" a fact instead of an
    // intention. Every message a controller declares is drained by the INGEST gate of the same tick;
    // if any survives to here, intent has started to accumulate somewhere it can outlive the frame
    // that produced it, be snapshot, and make two machines that agreed on everything else disagree.
    // Overflow counts as the same failure from the other end: a message that never made it into the
    // channel is one the gate never judged.
    if (state->channels.batting.count != 0 || state->channels.catching.count != 0) {
        printf(
            "\n[STATE ERROR] FATAL: intent left in a channel at the frame boundary (batting=%d, catching=%d)\n",
            state->channels.batting.count, state->channels.catching.count
        );
        return 0;
    }
    if (state->channels.batting.overflowed || state->channels.catching.overflowed) {
        printf("\n[STATE ERROR] FATAL: an intent channel overflowed — a declared intent was dropped unjudged\n");
        return 0;
    }
    if (state->channels.batting.malformed || state->channels.catching.malformed) {
        printf("\n[STATE ERROR] FATAL: a malformed intent reached a channel — no kind, or the other team's\n");
        return 0;
    }

    return 1; // Valid
}
