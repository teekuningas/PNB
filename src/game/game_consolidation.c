#include "game_consolidation.h"
#include "tick_outputs.h"
#include "field.h"
#include "team_data.h"
#include "rules_state.h"
#include "world.h"
#include "player_motion.h"
#include "runner_paths.h"
#include "base_logic.h"
#include "base_control.h"
#include "rules_pure/player_utils.h"
#include "rules_pure/rules_strikes.h"
#include "game_reset.h"
#include "referee.h"

// ===============================================================================================
// FORWARD DECLARATIONS
// ===============================================================================================

static FreeWalkEntitlement
determine_free_walk_entitlement(const MatchSession* match, const RefereeState* referee, int balls);
static int enforce_legal_state(
    MatchSession* match, const FieldPositions* field_positions, const RefereeState* referee,
    const BetweenPitchState* bps
);
static int handle_foul_play_reset(
    MatchSession* match, const FieldPositions* field_positions, const GameRulesState* rules, ConsolidationOutput* output
);
static void update_game_flow(
    MatchSession* match, const FieldPositions* field_positions, const GameRulesState* rules, int ball_pitch_resolved
);

static void check_next_batter_decision(MatchSession* match, const GameRulesState* rules);
static void apply_batter_becomes_runner(
    MatchSession* match, const FieldPositions* field_positions, const HalfInningState* his, const RefereeState* referee
);
static void handle_free_walk_offers(
    MatchSession* match, const HalfInningState* his, const RefereeState* referee, int ball_pitch_resolved
);
static int check_end_of_inning(
    MatchSession* match, const FieldPositions* field_positions, const TeamData* team_data,
    GameConclusion* game_conclusion, const GameRulesState* rules, ConsolidationOutput* output
);
static int check_next_pair(
    MatchSession* match, const FieldPositions* field_positions, const GameRulesState* rules, ConsolidationOutput* output
);
static void populate_game_conclusion(GameConclusion* game_conclusion, const Scoreboard* scoreboard, int winner);

// ===============================================================================================
// PUBLIC API
// ===============================================================================================

void consolidation_update(
    MatchSession* match, const FieldPositions* field_positions, const TeamData* team_data,
    GameConclusion* game_conclusion, const GameRulesState* rules, ConsolidationOutput* output
)
{
    const RefereeState* referee = &rules->referee;
    const BetweenPitchState* bps = &rules->between_pitch;

    // Initialize output
    output->request_screen_change = 0;
    output->target_screen = SCREEN_GAME;
    output->period_transition = PERIOD_TRANSITION_NONE;
    output->world_reset = 0;
    output->enforcement_completed = 0;

    // 0. Check for Physical Resets FIRST (these reset the world)
    // If any reset happens, abort all further processing for this frame

    // Check for end-of-inning reset
    if (check_end_of_inning(match, field_positions, team_data, game_conclusion, rules, output)) {
        return;
    }

    // Check for next pair reset (homerun contest)
    if (check_next_pair(match, field_positions, rules, output)) {
        return;
    }

    // Check for foul play (out of bounds) reset
    if (handle_foul_play_reset(match, field_positions, rules, output)) {
        return;
    }

    // 1. State Enforcement (The Enforcer)
    // Reconcile physical entities and pitch flight with this tick's referee verdicts FIRST, so
    // downstream flow decisions inspect an already-consistent world on the same tick.
    int ball_pitch_resolved = enforce_legal_state(match, field_positions, referee, bps);

    // 2. Game Flow Analysis (The Game Master)
    // Opens/closes batting offers and forces §18(1) runs against the reconciled world.
    update_game_flow(match, field_positions, rules, ball_pitch_resolved);
    output->enforcement_completed = 1;
}

// ===============================================================================================
// INTERNAL: PHYSICAL ENFORCEMENT (formerly reconcileLegalAndPhysicalState)
// ===============================================================================================

static int enforce_legal_state(
    MatchSession* match, const FieldPositions* field_positions, const RefereeState* referee,
    const BetweenPitchState* bps
)
{
    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        // 1. React to OUT
        if (referee->batting_players[i].status == PLAYER_STATUS_OUT) {
            if (match->players[i].batting.presence.state != PLAYER_STATE_OUT) {
                if (match->players[i].batting.presence.state == PLAYER_STATE_AT_BAT) {
                    match->at_bat.cycle = BATTING_CYCLE_CLOSED;
                }
                match->players[i].batting.presence.state = PLAYER_STATE_OUT;
                match->players[i].batting.presence.base_id = BASE_NONE;
                move_player_out(match->players, field_positions, i);
            }
        }

        // 2. React to SCORE
        if (referee->batting_players[i].has_scored && match->players[i].batting.presence.state != PLAYER_STATE_SCORED) {
            match->players[i].batting.presence.state = PLAYER_STATE_SCORED;
            match->players[i].batting.presence.base_id = BASE_NONE;
            move_player_out(match->players, field_positions, i);
        }

        // 3. React to WOUNDED
        if (referee->batting_players[i].status == PLAYER_STATUS_WOUNDED) {
            if (match->players[i].batting.presence.state != PLAYER_STATE_WOUNDED) {
                if (match->players[i].batting.presence.state == PLAYER_STATE_AT_BAT) {
                    match->at_bat.cycle = BATTING_CYCLE_CLOSED;
                }
                match->players[i].batting.presence.state = PLAYER_STATE_WOUNDED;
                match->players[i].batting.presence.base_id = BASE_NONE;
                move_player_out(match->players, field_positions, i);
            }
        }

        // 4. React to displacement (Panic Run)
        if (match->players[i].batting.presence.state == PLAYER_STATE_ON_BASE ||
            match->players[i].batting.presence.state == PLAYER_STATE_LEADING ||
            (match->players[i].batting.presence.state == PLAYER_STATE_RUNNING &&
             match->players[i].batting.presence.base_id == BASE_HOME && match->players[i].path.going_forward == 0)) {
            BaseID physBase = match->players[i].batting.presence.base_id;
            if (referee->batting_players[i].current_safety_base != physBase) {
                int isWounded = (referee->batting_players[i].status == PLAYER_STATUS_WOUNDED);

                if (!isWounded) {
                    // Player is physically at (or retreating to) a base where they legally have no safety.
                    // They must run forward.
                    run_to_next_base(match, field_positions, i, physBase);
                }
            }
        }
    }

    // 5. React to Pitch Resolution
    int ball_pitch_resolved = 0;
    if (bps->pitch_result != PITCH_RESULT_NONE && match->ball.pitch_in_flight) {
        match->ball.pitch_in_flight = 0;
        // A BALL asks §26's question afresh in update_game_flow on this same tick.
        if (bps->pitch_result == PITCH_RESULT_BALL) {
            ball_pitch_resolved = 1;
        }
    }
    return ball_pitch_resolved;
}

// ===============================================================================================
// INTERNAL: FOUL PLAY RESET
// ===============================================================================================

static int handle_foul_play_reset(
    MatchSession* match, const FieldPositions* field_positions, const GameRulesState* rules, ConsolidationOutput* output
)
{
    // Check if Referee has triggered the reset state
    if (rules->referee.foul_state == FOUL_STATE_RESETTING) {
        reset_for_foul_play(match, field_positions, rules);
        output->world_reset = 1;

        // Note: Referee will transition state to NONE in referee_finalize
        return 1; // Signal that we reset
    }
    return 0; // No reset
}

// Formerly applyFoulPlayReset in game_setup.c
// Now handled by game_reset.c: reset_for_foul_play

// ===============================================================================================
// INTERNAL: GAME FLOW (formerly game_analysis)
// ===============================================================================================

static void update_game_flow(
    MatchSession* match, const FieldPositions* field_positions, const GameRulesState* rules, int ball_pitch_resolved
)
{
    check_next_batter_decision(match, rules);

    // §18(1) holds in every mode: a batter who has received his three correct pitches is permanently a
    // runner, whatever else is being played. The homerun contest used to be exempt from this along with
    // the free walks, and that exemption was bug #8 — nothing closed the batting turn, so the lukkari
    // pitched a fourth time to a batter who had none left.
    apply_batter_becomes_runner(match, field_positions, &rules->half_inning, &rules->referee);

    // §26's free walks are normal play only. The contest answers wrong pitches its own way (§8: two wrong
    // gives the runner the free-walk right, three wrong gives the team two runs) and neither is built yet,
    // so offering the normal-play walk here would be the wrong rule rather than a missing one.
    if (rules->scoreboard.period < 4) {
        handle_free_walk_offers(match, &rules->half_inning, &rules->referee, ball_pitch_resolved);
    }
}

static void check_next_batter_decision(MatchSession* match, const GameRulesState* rules)
{
    const RefereeState* referee = &rules->referee;
    const BetweenPitchState* bps = &rules->between_pitch;
    const Scoreboard* scoreboard = &rules->scoreboard;

    // Cancel pending batter request if inning is ending or a batter is at the plate
    // (e.g. a batter who ran off home and retreated safely back to BASE_HOME after
    // offers.batter_seat had already opened).
    if (referee->end_of_inning_state != END_INNING_STATE_NONE || get_active_batter_index(match) != -1) {
        match->offers.batter_seat = 0;
        return;
    }

    // so this function's idea is to make progress in selecting a new batter if old one's gone.
    // so this will be called only once when possible.
    const HalfInningState* his = &rules->half_inning;

    if (scoreboard->period < 4 && get_active_batter_index(match) == -1 && match->offers.batter_seat == 0 &&
        referee->end_of_inning_state == END_INNING_STATE_NONE) {
        // §12: the batting order is a cycle, so there is always a next regular batter — until the
        // order comes round to the designated last batter, after which only an unused joker can
        // extend the turn. When neither is available there is nothing to ASK: the referee ends the
        // half-inning as soon as the ball is in a home fielder's hands, which is the rule's own
        // third conjunct.
        //
        // This raises the QUESTION and no longer carries an answer with it. There used to be an
        // engine-side "offer" here — a cursor a producer nudged along and the engine had to keep
        // legal, including withdrawing it when a run re-designated the last batter under a decision
        // already on the table. It is gone: a producer names the player it wants and the INGEST gate
        // asks §27/§12/§7 at the moment of seating, which is the moment the choice is actually made.
        // An answer that cannot go stale needs nobody to withdraw it.
        //
        // It asks `regular_order_spent`, NOT `turn_exhausted`. The difference is the whole point: this
        // prompt fires when nobody is standing at the plate, and the batter who just set off keeps
        // safety at home for a while yet (measured: 54 frames). `turn_exhausted` includes that, so
        // asked here it still reads 0 — which is why the question of WHO belongs at the seating and
        // not here (bug #9).
        // Asked of the candidate list itself, which is the same function the gate refuses with and
        // the client offers from. It used to be `regular_order_spent == 0 || jokers_left > 0`, which is
        // the same rule read a second way — and the two can disagree, because jokers_left and the
        // per-player joker statuses are reset by different owners at different moments (see the
        // period-boundary window the state validator tolerates). Reading the list makes a prompt
        // nobody could answer unraisable rather than merely unlikely.
        int candidates[BATTER_CANDIDATE_MAX];
        if (list_batter_candidates(match, scoreboard, his, candidates) > 0) {
            // have to check that there is only three players in the field too and that it is not a out of bounds
            // situation.
            if (count_active_batting_players(match->players) < BASE_COUNT && referee->foul_state == FOUL_STATE_NONE) {
                // also we cannot know yet if it will be out of position situation so we have to wait that the ball will
                // land in some way.
                if (bps->has_ball_hit_ground == 1 || bps->catch_has_been_made == 1) {
                    // if that happens we can now start.
                    // a controller may now name a batter, and the gate will let it through
                    match->offers.batter_seat = 1;
                }
            }
        }
    }
}

// §18(1): the batter who has spent his three correct pitches has permanently become a runner. He is no
// longer in the batting turn and no longer safe at home, so he sets off for first — where the outside
// team may burn him at once, exactly as the rule's own note says.
static void apply_batter_becomes_runner(
    MatchSession* match, const FieldPositions* field_positions, const HalfInningState* his, const RefereeState* referee
)
{
    if (batter_has_become_runner_permanently(his->strikes)) {
        // We restore automatic force running to resolve control ambiguity.
        // The batter is now "forced" to run by the rules.
        int index = get_base_controller(match, referee, BASE_HOME);

        // Only force run if player is still there and NOT already running.
        // This prevents re-triggering every frame while preserving the 3 strikes state
        // until the next batter resets it.
        if (index != -1 && match->players[index].batting.presence.state != PLAYER_STATE_RUNNING) {
            run_to_next_base(match, field_positions, index, BASE_HOME);
        }
    }
}

static void handle_free_walk_offers(
    MatchSession* match, const HalfInningState* his, const RefereeState* referee, int ball_pitch_resolved
)
{
    // §26 entitlement is evaluated once on the exact tick a BALL resolves, against the physical world
    // enforce_legal_state has already reconciled with this tick's verdicts. If the entitled player
    // later advances on their own, the recorded base stays what the walk entitled them from.
    if (ball_pitch_resolved) {
        match->free_walk = determine_free_walk_entitlement(match, referee, his->balls);
        match->offers.free_walk = (match->free_walk.runner != -1);
    } else if (match->offers.free_walk == 1) {
        // Withdraw the open offer if the entitled runner leaves play (out, wounded, or scored) before
        // answering. No -1 guard: an open offer always names a real player, asserted by state_validator.
        const PlayerUnitState state = match->players[match->free_walk.runner].batting.presence.state;
        if (state == PLAYER_STATE_WOUNDED || state == PLAYER_STATE_OUT || state == PLAYER_STATE_SCORED) {
            match->offers.free_walk = 0;
        }
    }
}

static int check_end_of_inning(
    MatchSession* match, const FieldPositions* field_positions, const TeamData* team_data,
    GameConclusion* game_conclusion, const GameRulesState* rules, ConsolidationOutput* output
)
{
    const RefereeState* referee = &rules->referee;
    const Scoreboard* scoreboard = &rules->scoreboard;

    // Scoreboard advancement and period logic handled by referee at DETECTED→RESETTING.
    // We only react when Referee signals RESETTING: perform physical reset + menu routing.

    if (referee->end_of_inning_state != END_INNING_STATE_RESETTING) {
        return 0;
    }

    PeriodTransitionType transition = referee->period_transition;

    if (transition == PERIOD_TRANSITION_NONE) {
        // Normal next-inning within same period — just reset physical world
        reset_for_new_half_inning(match, field_positions, team_data, rules);
        output->world_reset = 1;
    } else {
        // Period transition — the session routes to the menu this transition calls for
        if (transition == PERIOD_TRANSITION_GAME_OVER) {
            populate_game_conclusion(game_conclusion, scoreboard, referee->period_transition_winner);
        }
        output->period_transition = transition;
        output->request_screen_change = 1;
        output->target_screen = SCREEN_MAIN_MENU;
    }

    return 1; // Signal that we handled end-of-inning
}

static int check_next_pair(
    MatchSession* match, const FieldPositions* field_positions, const GameRulesState* rules, ConsolidationOutput* output
)
{
    const Scoreboard* scoreboard = &rules->scoreboard;

    if (scoreboard->period >= 4) {

        // Milestone 17.5: Timer and logic moved to Referee (State Machine).
        // We only react when Referee signals RESETTING (State 2).

        HomeRunPairState currentState = rules->referee.next_pair_state;

        if (currentState == HR_PAIR_STATE_RESETTING) {

            // if equality holds, ending of inning will load the settings.
            if (rules->homerun_contest.pair_index != scoreboard->pair_count) {
                // Physical Reset for Next Pair
                reset_for_next_pair(match, field_positions, scoreboard, &rules->homerun_contest);
                output->world_reset = 1;
            }
            return 1; // Signal that we reset
        }
    }
    return 0; // No reset
}

static void populate_game_conclusion(GameConclusion* game_conclusion, const Scoreboard* scoreboard, int winner)
{
    game_conclusion->winner = winner;
    game_conclusion->is_cup_game = scoreboard->is_cup_game;
    game_conclusion->period0_runs[0] = scoreboard->teams[0].period0_runs;
    game_conclusion->period0_runs[1] = scoreboard->teams[1].period0_runs;
    game_conclusion->period1_runs[0] = scoreboard->teams[0].period1_runs;
    game_conclusion->period1_runs[1] = scoreboard->teams[1].period1_runs;
    game_conclusion->period2_runs[0] = scoreboard->teams[0].period2_runs;
    game_conclusion->period2_runs[1] = scoreboard->teams[1].period2_runs;
    game_conclusion->period3_runs[0] = scoreboard->teams[0].period3_runs;
    game_conclusion->period3_runs[1] = scoreboard->teams[1].period3_runs;
}

// §26 — decide who is entitled to a free walk on this wrong pitch.
//
// Candidates are active batting-team players on a playable base (BASE_HOME..BASE_THIRD; BASE_NONE and
// BASE_HOME_SCORED are excluded). A batter who legally broke off home under §32 (PLAYER_STATE_RUNNING
// with base_id == BASE_HOME) is a valid candidate — get_active_batter_index only finds AT_BAT, while
// state_validator separately forbids a settled PLAYER_STATE_ON_BASE at BASE_HOME.
//
// Lead runner wins (highest base_id, tie-broken by base_at_pitch_start, then off-plate ahead of
// AT_BAT at home). Under §26, an empty field (lead candidate at BASE_HOME) grants a walk on 1 wrong
// pitch; any field runner (BASE_FIRST..BASE_THIRD) requires 2+.
static FreeWalkEntitlement
determine_free_walk_entitlement(const MatchSession* match, const RefereeState* referee, int balls)
{
    FreeWalkEntitlement best = {.runner = -1, .from = BASE_NONE};
    BaseID best_start = BASE_NONE;

    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        const BattingPresence presence = match->players[i].batting.presence;
        if (!base_is_index(presence.base_id)) continue;
        if (presence.state == PLAYER_STATE_WOUNDED || presence.state == PLAYER_STATE_OUT ||
            presence.state == PLAYER_STATE_SCORED || presence.state == PLAYER_STATE_IDLE) {
            continue;
        }

        const BaseID start_base = referee->batting_players[i].base_at_pitch_start;
        const int cmp = base_cmp(presence.base_id, best.from);
        int takes_lead = 0;
        if (cmp > 0) {
            takes_lead = 1;
        } else if (cmp == 0 && best.runner != -1) {
            const int start_cmp = base_cmp(start_base, best_start);
            if (start_cmp > 0) {
                takes_lead = 1;
            } else if (start_cmp == 0 && presence.base_id == BASE_HOME && presence.state != PLAYER_STATE_AT_BAT &&
                       match->players[best.runner].batting.presence.state == PLAYER_STATE_AT_BAT) {
                takes_lead = 1;
            }
        }

        if (takes_lead) {
            best.runner = i;
            best.from = presence.base_id;
            best_start = start_base;
        }
    }

    if (best.runner == -1) return best;

    const int required_balls = (best.from == BASE_HOME) ? 1 : 2;
    if (balls < required_balls) {
        return (FreeWalkEntitlement){.runner = -1, .from = BASE_NONE};
    }
    return best;
}
