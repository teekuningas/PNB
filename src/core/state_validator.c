#include "state_validator.h"
#include "field.h"
#include "rules_state.h"
#include "world.h"
#include "session.h"
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
    int frame_count; // Assuming we can get this, or just sequence ID
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
    g_history[idx].frame_count = g_sequenceId++;

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

static void
print_game_json(FILE* f, MatchSession* game, GameRulesState* rules, const FieldPositions* fieldPositions, int indent)
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
        fprintf(f, "%s  \"pair_count\": %d,\n", sp, scoreboard->pair_count);
        fprintf(f, "%s  \"team0_runs\": %d,\n", sp, scoreboard->teams[0].runs);
        fprintf(f, "%s  \"team1_runs\": %d,\n", sp, scoreboard->teams[1].runs);

        // Detailed Team Info (Batter Orders)
        for (int t = 0; t < 2; t++) {
            fprintf(f, "%s  \"team%d_batter_order_index\": %d,\n", sp, t, scoreboard->teams[t].batter_order_index);
            fprintf(f, "%s  \"team%d_batter_order\": [", sp, t);
            for (int bo = 0; bo < 12; bo++) {
                fprintf(f, "%d%s", scoreboard->teams[t].batter_order[bo], (bo < 11) ? ", " : "");
            }
            fprintf(f, "],\n");
        }
        fprintf(
            f, "%s  \"runs\": [%d, %d]\n", sp, scoreboard->teams[0].runs, scoreboard->teams[1].runs
        ); // Redundant but convenient
        fprintf(f, "%s},\n", sp);
    }

    if (rules) {
        fprintf(f, "%s\"half_inning\": {\n", sp);
        fprintf(f, "%s  \"outs\": %d,\n", sp, rules->half_inning.outs);
        fprintf(f, "%s  \"runs_in_inning\": %d,\n", sp, rules->half_inning.runs_in_inning);
        fprintf(f, "%s  \"strikes\": %d,\n", sp, rules->half_inning.strikes);
        fprintf(f, "%s  \"balls\": %d,\n", sp, rules->half_inning.balls);
        fprintf(f, "%s  \"batter_may_leave_home\": %d\n", sp, rules->half_inning.batter_may_leave_home);
        fprintf(f, "%s},\n", sp);

        fprintf(f, "%s\"last_batter\": {\n", sp);
        fprintf(f, "%s  \"jokers_left\": %d,\n", sp, rules->half_inning.jokers_left);
        fprintf(f, "%s  \"designated_index\": %d,\n", sp, rules->half_inning.last_batter.designated_index);
        fprintf(f, "%s  \"last_regular_index\": %d,\n", sp, rules->half_inning.last_batter.last_regular_index);
        fprintf(f, "%s  \"has_batted_again\": %d,\n", sp, rules->half_inning.last_batter.has_batted_again);
        fprintf(f, "%s  \"turn_exhausted\": %d\n", sp, rules->half_inning.last_batter.turn_exhausted);
        fprintf(f, "%s},\n", sp);

        fprintf(f, "%s\"gameControl\": {\n", sp);
        fprintf(f, "%s  \"batter_seat_offered\": %d,\n", sp, game->offers.batter_seat);
        fprintf(f, "%s  \"free_walk_offered\": %d,\n", sp, game->offers.free_walk);
        fprintf(f, "%s  \"catch_has_been_made\": %d,\n", sp, rules->between_pitch.catch_has_been_made);
        fprintf(f, "%s  \"has_ball_hit_ground\": %d,\n", sp, rules->between_pitch.has_ball_hit_ground);
        fprintf(f, "%s  \"foul_state\": %d\n", sp, (int)rules->referee.foul_state);
        fprintf(f, "%s },\n", sp);
    }

    fprintf(f, "%s\"events\": {\n", sp);
    fprintf(f, "%s  \"catch_made\": %d,\n", sp, game->events.catch_made);
    fprintf(f, "%s  \"player_arrived_at_base\": %d,\n", sp, game->events.player_arrived_at_base);
    fprintf(f, "%s  \"ball_hit_ground\": %d,\n", sp, game->events.ball_hit_ground);
    fprintf(f, "%s  \"ball_hit_by_bat\": %d,\n", sp, game->events.ball_hit_by_bat);
    fprintf(f, "%s  \"pitch_released\": %d,\n", sp, game->events.pitch_released);
    fprintf(f, "%s  \"batter_entered\": %d\n", sp, game->events.batter_entered);
    fprintf(f, "%s},\n", sp);

    fprintf(f, "%s\"flow\": {\n", sp);
    fprintf(f, "%s  \"ballHome\": %d", sp, ball_is_home(game, fieldPositions));
    if (rules) {
        fprintf(f, ",\n%s  \"end_of_inning_state\": %d\n", sp, (int)rules->referee.end_of_inning_state);
    } else {
        fprintf(f, "\n");
    }
    fprintf(f, "%s},\n", sp);

    fprintf(f, "%s\"possession\": {\n", sp);
    fprintf(f, "%s  \"batterIndex\": %d,\n", sp, get_active_batter_index(game));
    fprintf(f, "%s  \"holder\": %d,\n", sp, game->ball.holder);
    fprintf(f, "%s  \"controlled_fielder\": %d,\n", sp, game->catching.controlled_fielder);
    fprintf(f, "%s  \"home_baseman\": %d,\n", sp, game->fielding_posts.baseman[0]);
    fprintf(
        f, "%s  \"isNearHome_pitcher\": %d\n", sp, game->players[game->fielding_posts.baseman[0]].fielding.near_home
    );
    fprintf(f, "%s},\n", sp);

    fprintf(f, "%s\"flight\": {\n", sp);
    fprintf(f, "%s  \"pitch_in_flight\": %d,\n", sp, game->ball.pitch_in_flight);
    fprintf(f, "%s  \"thrown_to\": %d\n", sp, (int)game->ball.thrown_to);
    fprintf(f, "%s},\n", sp);

    fprintf(f, "%s\"atBat\": {\n", sp);
    fprintf(f, "%s  \"cycle\": %d\n", sp, (int)game->at_bat.cycle);
    fprintf(f, "%s},\n", sp);

    fprintf(f, "%s\"actionState\": {\n", sp);
    fprintf(f, "%s  \"catching_action\": %d,\n", sp, (int)game->catching_action.kind);
    fprintf(f, "%s  \"catching_action_timer\": %d,\n", sp, game->catching_action.timer);
    // The drop / change-player commands used to be dumped here as persistent flags. They are messages
    // now: declared and consumed inside one tick, so at the frame boundary this dump is taken there is
    // nothing left of them to print — what a reader wants instead is the world they produced.
    // The swing, as the engine holds it: what has been declared and the frame it will be used on.
    // No phase and no meter — there is nothing here a producer drives forward.
    fprintf(f, "%s  \"swing_contact_frame\": %d,\n", sp, game->at_bat.swing.contact_frame);
    fprintf(f, "%s  \"swing_power\": %s,\n", sp, game->at_bat.swing.power_active ? "true" : "false");
    fprintf(f, "%s  \"swing_vertical\": %s\n", sp, game->at_bat.swing.vertical_active ? "true" : "false");
    fprintf(f, "%s},\n", sp);

    if (rules) {
        fprintf(f, "%s\"between_pitch\": {\n", sp);
        fprintf(f, "%s  \"catch_has_been_made\": %d,\n", sp, rules->between_pitch.catch_has_been_made);
        fprintf(f, "%s  \"has_ball_hit_ground\": %d,\n", sp, rules->between_pitch.has_ball_hit_ground);
        fprintf(f, "%s  \"pitch_result\": %d,\n", sp, rules->between_pitch.pitch_result);
        fprintf(f, "%s  \"bat_outcome\": %d\n", sp, rules->between_pitch.bat_outcome);
        fprintf(f, "%s},\n", sp);
    }

    fprintf(f, "%s\"ball\": {\n", sp);
    fprintf(
        f, "%s  \"location\": { \"x\": %.2f, \"y\": %.2f, \"z\": %.2f },\n", sp, game->ball.location.x,
        game->ball.location.y, game->ball.location.z
    );
    fprintf(f, "%s  \"moving\": %d,\n", sp, game->ball.moving);
    fprintf(f, "%s  \"flight_has_hit_ground\": %d,\n", sp, game->ball.flight_has_hit_ground);
    fprintf(f, "%s  \"on_ground\": %d\n", sp, game->ball.on_ground);
    fprintf(f, "%s},\n", sp);

    fprintf(f, "%s\"rng_seed\": %u,\n", sp, game->rng_seed);

    fprintf(f, "%s\"players\": [\n", sp);
    int totalPlayers = 2 * PLAYERS_IN_TEAM + JOKER_COUNT; // 21
    for (int i = 0; i < totalPlayers; i++) {
        PlayerInfo* p = &game->players[i];

        fprintf(f, "%s  {\n", sp);
        fprintf(f, "%s    \"id\": %d,\n", sp, i);
        fprintf(f, "%s    \"team\": %d,\n", sp, p->side);
        fprintf(f, "%s    \"baseId\": %d,\n", sp, p->batting.presence.base_id);
        fprintf(f, "%s    \"baseStr\": \"%s\",\n", sp, base_to_string(p->batting.presence.base_id));
        fprintf(f, "%s    \"state\": \"%s\",\n", sp, state_to_string(p->batting.presence.state));
        fprintf(f, "%s    \"pos\": { \"x\": %.2f, \"z\": %.2f },\n", sp, p->body.location.x, p->body.location.z);
        // Runtime state
        fprintf(
            f, "%s    \"runtime\": { \"goingForward\": %d, \"arrived\": %d }", sp, game->players[i].path.going_forward,
            game->players[i].path.arrived_at_base
        );

        // Referee State - Only valid for batting team (indices 0-11)
        // Assuming 0-11 are ALWAYS the batting team in the current frame
        if (rules && i < PLAYERS_IN_TEAM + JOKER_COUNT) {
            fprintf(f, ",\n");
            fprintf(f, "%s    \"ref_safetyBase\": %d,\n", sp, rules->referee.batting_players[i].current_safety_base);
            fprintf(
                f, "%s    \"ref_safetyBaseStr\": \"%s\",\n", sp,
                base_to_string(rules->referee.batting_players[i].current_safety_base)
            );
            fprintf(
                f, "%s    \"ref_baseAtPitchStart\": %d,\n", sp, rules->referee.batting_players[i].base_at_pitch_start
            );
            fprintf(f, "%s    \"ref_status\": %d\n", sp, rules->referee.batting_players[i].status);
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
    print_game_json(f, game, rules, state->field_positions, 4);
    fprintf(f, "\n  },\n");

    // History
    fprintf(f, "  \"history\": [\n");

    int startIdx = (g_historyCount < HISTORY_SIZE) ? 0 : g_historyHead;
    for (int i = 0; i < g_historyCount; i++) {
        int idx = (startIdx + i) % HISTORY_SIZE;
        fprintf(f, "    {\n");
        fprintf(f, "      \"seq\": %d,\n", g_history[idx].frame_count);
        fprintf(f, "      \"label\": \"%s\",\n", g_history[idx].label);
        fprintf(f, "      \"snapshot\": {\n");
        print_game_json(f, &g_history[idx].snapshot, NULL, state->field_positions, 8); // No global info for snapshots
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
    if (state->paused) return 1;

    // Invariant 1: Unique Base Occupancy (Safe players)
    // Check baseControlIndex vs Player state
    for (int b = 0; b < BASE_COUNT; b++) {
        int idx = get_base_controller(game, &rules->referee, b);
        if (idx != -1) {
            // Player at this index MUST be at this base (enforced by get_base_controller definition)
            if (game->players[idx].batting.presence.base_id != (BaseID)b) {
                printf(
                    "\n[STATE ERROR] FATAL: Base %d controlled by %d, but player is at base %d\n", b, idx,
                    game->players[idx].batting.presence.base_id
                );
                return 0; // Invalid
            }

            // Player MUST not be in invalid states for controlling a base
            if (game->players[idx].batting.presence.state == PLAYER_STATE_OUT ||
                game->players[idx].batting.presence.state == PLAYER_STATE_WOUNDED ||
                game->players[idx].batting.presence.state == PLAYER_STATE_SCORED ||
                game->players[idx].batting.presence.state == PLAYER_STATE_IDLE) {
                printf(
                    "\n[STATE ERROR] FATAL: Base %d controlled by %d, but player state is %s (Invalid for "
                    "controller)\n",
                    b, idx, state_to_string(game->players[idx].batting.presence.state)
                );
                return 0; // Invalid
            }
        }
    }

    // Invariant 2: Batter Consistency
    int activeBatterCount = 0;
    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        if (game->players[i].batting.presence.state == PLAYER_STATE_AT_BAT) {
            activeBatterCount++;

            // Must be at home
            if (game->players[i].batting.presence.base_id != BASE_HOME) {
                printf(
                    "\n[STATE ERROR] FATAL: Player %d is AT_BAT but at base %d (Must be BASE_HOME)\n", i,
                    game->players[i].batting.presence.base_id
                );
                return 0;
            }
        }
        // A settled player at BASE_HOME must be AT_BAT, never ON_BASE: get_active_batter_index only
        // finds AT_BAT, so ON_BASE at home leaves the plate occupied yet unpitchable and unseatable.
        if (game->players[i].batting.presence.state == PLAYER_STATE_ON_BASE &&
            game->players[i].batting.presence.base_id == BASE_HOME) {
            printf(
                "\n[STATE ERROR] FATAL: Player %d is ON_BASE at BASE_HOME (settled home presence must be AT_BAT)\n", i
            );
            return 0;
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
    // `half_inning.jokers_left`, which the referee decrements on the batter_entered event. Two
    // writers, one rule, in two different stages of the frame.
    //
    // The direction is the load-bearing part. More AVAILABLE jokers than the count admits is the
    // failure that matters, and it is §12's third conjunct going wrong: the half-inning ends when
    // `jokers_left` hits zero, but the candidate list a producer chooses from is built from the
    // per-player statuses — so a joker still marked available when the count has forgotten it means
    // a turn that ends with a batter the rules say the team may still use. Seating an already-spent
    // joker produces exactly that, because the status has nothing left to change while the referee
    // decrements regardless.
    //
    // The other direction is deliberately tolerated, because it is a real state and not a defect: at
    // a PERIOD boundary the referee clears jokers_left to JOKER_COUNT, but the physical reset that
    // re-marks the players never runs — consolidation routes to a menu instead. So between periods
    // the count leads the statuses. Nothing reads either while that lasts. Recorded rather than
    // asserted away: it is the same two-representations hazard from the harmless side.
    //
    // This replaces the invariant that used to live here, which asked whether the engine's OFFER of
    // a next batter was legal. There is no offer any more: a producer names a player and the INGEST
    // gate refuses an illegal one (§27/§12/§7), which is a stronger place to hold the rule than an
    // end-of-frame assertion. Re-pointing that assertion at "no regular is SEATED while the order is
    // spent" was considered and rejected — it would repeat bug #9's mistake exactly, because seating
    // a regular advances the batting order, which clears `regular_order_spent` in the same frame. The
    // assertion would be looking at a state that erases itself at the moment of the violation.
    // Ablated at the gate instead, in the contract tier, where the refusal actually happens.
    if (rules->scoreboard.period < 4) {
        int unusedJokers = 0;
        int jokers[JOKER_COUNT];
        batting_team_jokers(&rules->scoreboard, jokers);
        for (int i = 0; i < JOKER_COUNT; i++) {
            if (game->players[jokers[i]].batting.joker == JOKER_AVAILABLE) unusedJokers++;
        }
        if (unusedJokers > rules->half_inning.jokers_left) {
            printf(
                "\n[STATE ERROR] FATAL: §7 — %d jokers are still marked available, but "
                "half_inning.jokers_left says only %d remain: a half-inning could end with a joker "
                "the team may still use\n",
                unusedJokers, rules->half_inning.jokers_left
            );
            return 0;
        }
    }

    // Invariant 4: an open free-walk offer names a real player.
    //
    // §26 gives the walk to the lead runner, so the offer and the player it belongs to are one fact
    // and must be set together. They were not: the offer was raised whenever the ball count reached
    // the threshold, while determine_free_walk_entitlement leaves free_walk.runner at -1 when nobody
    // is active on the field. That put a prompt on screen belonging to nobody, made the withdrawal
    // beside it read players[-1] every frame, and — because the lukkari may not pitch while an offer
    // is open — left the game waiting for an answer to a question about no one. Found by the owner in
    // play, 2026-08-28.
    //
    // Asserted here rather than defended at the read, because this is a resting state: it is true
    // between frames or it is not, and an assertion on it fires at the frame a future change breaks
    // the pairing rather than quietly papering over it.
    if (game->offers.free_walk == 1 &&
        (game->free_walk.runner < 0 || game->free_walk.runner >= PLAYERS_IN_TEAM + JOKER_COUNT)) {
        printf(
            "\n[STATE ERROR] FATAL: a free walk is being offered, but free_walk.runner is %d — §26's walk "
            "belongs to a player, so an offer that names nobody is not an offer\n",
            game->free_walk.runner
        );
        return 0;
    }

    // Invariant 4b: no free walk is on offer while a pitch is being wound up.
    //
    // The lukkari may not begin a windup while an offer is open (pitch_may_begin), so the only way the
    // two can meet is an offer raised AFTER the windup began — which is what the windup state the
    // pitch flag used to carry did: starting a windup with the previous BALL still standing made consolidation react
    // to that BALL a second time, re-run the free-walk calculation and, with two balls standing, ask the
    // batting side a question it had already answered, in the middle of the pitch.
    if (game->offers.free_walk == 1 && game->catching_action.kind == CATCHING_ACTION_PITCHING) {
        printf("\n[STATE ERROR] FATAL: a free walk is on offer while a pitch is being wound up\n");
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

    // Invariant 6: no chase re-plan is pending at the frame boundary. A change of flight asks for one and
    // the ranking answers it later in the same tick; a request still standing here was raised after the
    // ranking ran, by a stage that has no business changing the ball's flight.
    if (game->chase_replan != CHASE_REPLAN_NONE) {
        printf("\n[STATE ERROR] FATAL: a chase re-plan was requested after the ranking had run this tick\n");
        return 0;
    }

    return 1; // Valid
}
