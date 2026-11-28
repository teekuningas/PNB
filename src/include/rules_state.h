#ifndef RULES_STATE_H
#define RULES_STATE_H

// The legal world: the referee's state machines and per-player verdicts, the half-inning, the pitch's
// adjudication, the scoreboard — GameRulesState. Only the referee writes it at runtime; every other stage
// receives it const. The physical World's header does not include this one, so code that holds only the
// World cannot even name the law.

#include "foundation.h"

typedef enum { PITCH_RESULT_NONE = 0, PITCH_RESULT_STRIKE = 1, PITCH_RESULT_BALL = 2 } PitchResult;

// §24 Syöttöoikeus (pitch rights): after the previous batter becomes permanently a runner (§18),
// the ball must visit Home → Field Base → Home in fielders' possession before the next batter may
// be pitched to. Always GRANTED at inning start and throughout the home-run contest (§8).
typedef enum {
    PITCH_RIGHTS_GRANTED = 0, // lukkari may pitch to the current/next batter
    PITCH_RIGHTS_NEED_HOME, // §18 fired: waiting for ball to be held at home
    PITCH_RIGHTS_NEED_FIELD_BASE, // ball visited home after §18: waiting for 1st, 2nd, or 3rd
    PITCH_RIGHTS_NEED_RETURN_HOME // ball visited a field base after home: waiting for return to home
} PitchRightsState;

typedef enum {
    GAME_MODE_NORMAL = 0,
    GAME_MODE_SUPER_INNING = 1, // Potential mapping for period 2/3 if distinct
    GAME_MODE_HOMERUN_CONTEST = 2 // Replaces period >= 4
} GamePeriodMode;

// Flow control state machine enums (used by RefereeState)
typedef enum {
    FOUL_STATE_NONE = 0,
    FOUL_STATE_DETECTED = 1, // Ball hit ground out of bounds, waiting for grace period
    FOUL_STATE_RESETTING = 2 // Grace period over, performing reset this frame
} FoulPlayState;

typedef enum { HR_PAIR_STATE_NONE = 0, HR_PAIR_STATE_DETECTED = 1, HR_PAIR_STATE_RESETTING = 2 } HomeRunPairState;

typedef enum {
    END_INNING_STATE_NONE = 0,
    END_INNING_STATE_DETECTED = 1,
    END_INNING_STATE_RESETTING = 2
} EndOfInningTransitionState;

// Period transition result — set by referee at DETECTED→RESETTING,
// read by consolidation to determine menu/screen routing.
typedef enum {
    PERIOD_TRANSITION_NONE = 0, // Normal next-inning (no menu, just physical reset)
    PERIOD_TRANSITION_INTER_PERIOD, // Period 0→1 transition
    PERIOD_TRANSITION_SUPER_INNING, // Period 1→2 transition (super inning)
    PERIOD_TRANSITION_HOMERUN_CONTEST, // Period 2→4 or round→round+2
    PERIOD_TRANSITION_GAME_OVER // Game ended (winner stored separately)
} PeriodTransitionType;

// Player status enum - replaces flag-based status tracking
typedef enum {
    PLAYER_STATUS_ACTIVE = 0, // Playing normally, no special status
    PLAYER_STATUS_WOUND_MARKED = 1, // Vulnerable: evaluation timer running, can cancel if ball drops (normal wound)
    PLAYER_STATUS_WOUND_MARKED_DOUBLE = 2, // Vulnerable: evaluation timer running (tuplahaava/double wound)
    PLAYER_STATUS_WOUND_PENDING = 3, // DOOMED (normal wound): must advance, will get WOUNDED or OUT
    PLAYER_STATUS_WOUND_PENDING_DOUBLE = 4, // DOOMED (tuplahaava): can retreat to avoid OUT
    PLAYER_STATUS_WOUNDED = 5, // TERMINAL: Out of play, returns to home base
    PLAYER_STATUS_OUT = 6 // TERMINAL: Out of play, returns to home base
} RefereePlayerStatus;

typedef struct _RefereePlayerState {
    // === PRIMARY STATUS ===
    RefereePlayerStatus status; // Mutually exclusive player status (wounding/out lifecycle)

    // === PITCH START SNAPSHOT (for foul play) ===
    BaseID base_at_pitch_start; // Where was player when pitch started

    // === CURRENT SAFETY STATUS ===
    BaseID current_safety_base; // Which base has their safety (-1 if none)
    int has_scored; // Logical scored status (decided by Referee)
    int run_of_honor_scored; // Logical run of honor scored status (decided by Referee)

    // === PENDING RUNS (Milestone 17) ===
    int has_pending_run; // Arrived at home while ball in air
    int has_pending_run_of_honor; // Arrived at 3rd (homerun) while ball in air

} RefereePlayerState;

typedef struct _RefereeState {
    // Per-player tracking
    RefereePlayerState batting_players[PLAYERS_IN_TEAM + JOKER_COUNT];

    // Global events
    int strikes_at_pitch_start; // Snapshot of strikes when pitch started

    // Wounding evaluation state (Referee monitors and decides)
    int wounding_evaluation_active; // Referee is evaluating a potential wounding catch
    int wounding_evaluation_finished; // Milestone 17: Flag set when evaluation completes (catch confirmed)
    int wounding_evaluation_timer; // Timer counting up during evaluation period

    // Flow Control State Machines (Milestone 17 consolidation)
    FoulPlayState foul_state; // Out-of-bounds transition state
    int foul_timer; // Timer for foul play grace period

    HomeRunPairState next_pair_state; // Next-pair transition state
    int next_pair_timer; // Timer for next-pair transition

    EndOfInningTransitionState end_of_inning_state; // End-of-inning transition state
    int end_of_inning_timer; // Timer for end-of-inning transition
    PeriodTransitionType period_transition; // Result of period logic (set at DETECTED→RESETTING)
    int period_transition_winner; // Winner index (valid when period_transition == GAME_OVER)

    // Run of Honor tracking (Homerun Contest)
    int ball_in_third_base_since_pitch; // Has ball been held at 3rd base by catching team since pitch started

    int homerun_pair_has_pitch; // Has at least one pitch been released for current pair
} RefereeState;

typedef struct _TeamInfo {
    int value; // which team, like 1 for ankkurit, 2 for vimpeli etc.
    TeamControlMode control; // who controls, 0 player 1, 1, player 2, 2 AI
    int runs; // how many runs has this team
    int period0_runs;
    int period1_runs;
    int period2_runs;
    int period3_runs;
    int batter_runner_indices[2][5];
    int batter_order[12]; // this teams' batting order, used in many innings
    int batter_order_index; // what player is next
} TeamInfo;

// §12 Vuoronvaihto — the designation that decides when a batting turn is spent.
// Lifetime: one half-inning, which is exactly the rule's own lifetime: the designation is re-made on
// every even-numbered run and dies with the inning. Referee-owned; the decisions themselves are the
// pure predicates in rules_pure/rules_side_change.h.
typedef struct _LastBatterState {
    int designated_index; // the viimeinen lyöjä; -1 until the half-inning's first batter takes the bat
    int last_regular_index; // most recent REGULAR player to take the bat; -1 if none yet (jokers don't count)
    int has_batted_again; // the designated player has taken the bat since being designated

    // Two different questions, and conflating them was a real §12 defect. Both are §12(2)/(3), but
    // they are answered at different moments and by different amounts of the rule.
    //
    // WHO MAY BAT NEXT. `regular_order_spent` is the batting-order clause on its own: the order has
    // come round to the designated last batter, so no further REGULAR may be seated and only a
    // joker can extend the turn (§27: "paloa ei tuomita, mikäli jokeripelaaja otetaan lyömään").
    // It settles the instant the previous batter enters and never wobbles afterwards, which is what
    // makes it safe to ask at the moment a new batter is chosen.
    //
    // DOES THE HALF-INNING END. `turn_exhausted` adds the rule's other trigger — that the batter has
    // "muuttunut lopullisesti etenijäksi", read here as having lost safety at home. That one is
    // genuinely transient: a batter stops standing at the plate the moment he sets off but keeps
    // home until he is established elsewhere, and during a foul he is restored to the plate and
    // takes home back. It belongs to the verdict (with the ball home and no jokers left), never to
    // the choice of batter.
    int regular_order_spent;
    int turn_exhausted;
} LastBatterState;

// MILESTONE 7.5 - Focused Structs
typedef struct _HalfInningState {
    int outs;
    int balls;
    int strikes;
    // §32: "Lyöntivuoroinen lyöjä saa edetä kotipesästä heti, kun pallo on irronnut lukkarin kädestä
    // ensimmäiseen syöttöön, riippumatta syötön laadusta." Legal at-bat lifetime: granted when the
    // turn's first pitch leaves the hand, preserved across 1st/2nd-strike foul resets for the same
    // batter, cleared when a new batter enters, on a 3rd-strike foul burn, on contest pair end, or
    // at half-inning reset.
    int batter_may_leave_home;
    // §24: pitch rights for the current/next batter. Starts GRANTED, revoked once when the active
    // batter permanently becomes a runner (§18), advances Home → Field Base → Home, preserved across
    // 1st/2nd-strike foul resets, reset to NEED_HOME on 3rd-strike foul burn, exempt in contest (§8).
    PitchRightsState pitch_rights;
    // Whether a batter currently holds an active batting turn at home (< 3 strikes), so §24 revokes
    // pitch rights on the single edge when that batter becomes permanently a runner (§18) and never
    // re-revokes while waiting for the next batter to enter.
    int batter_in_turn;
    int runs_in_inning;
    int end_period; // Rule state: period should end
    // §7: "Joukkue voi käyttää jokaisessa sisävuorossa kolmea eri jokeripelaajaa, kerran kutakin."
    // A genuinely consumable resource, and per half-inning — so it lives with the half-inning.
    int jokers_left;
    LastBatterState last_batter;
} HalfInningState;

// Batting outcome for the current pitch (promoted from GameEvents by referee)
typedef enum {
    BAT_OUTCOME_NONE, // No batting outcome yet
    BAT_OUTCOME_HIT, // Bat made contact with ball
    BAT_OUTCOME_MISSED // Batter swung and missed
} BatOutcome;

// MILESTONE 17: Between-pitch state (reset at pitch start, written by referee)
// Sticky flags that persist across frames but reset when new pitch starts
typedef struct _BetweenPitchState {
    int catch_has_been_made; // Fly ball was caught (promoted from catch_made)
    int has_ball_hit_ground; // Ball has touched ground (promoted from ball_hit_ground)
    PitchResult pitch_result; // Referee's pitch adjudication (NONE until resolved)
    BatOutcome bat_outcome; // Bat hit or missed (promoted from ball_hit_by_bat/ball_missed_by_bat)
} BetweenPitchState;

typedef struct _HomeRunContestState {
    int pair_index;
} HomeRunContestState;

typedef struct _Scoreboard {
    TeamInfo teams[2];
    int half_innings_in_period;
    int inning;
    int period;
    GamePeriodMode mode; // NEW field for Milestone 7
    int pair_count;
    int plays_first;
    int is_cup_game;
} Scoreboard;

// GameRulesState — Referee-owned state, structurally separated from physical/action state.
// Only the referee writes these at runtime. Other stages receive const pointers.
typedef struct _GameRulesState {
    RefereeState referee;
    HalfInningState half_inning;
    BetweenPitchState between_pitch;
    HomeRunContestState homerun_contest;
    Scoreboard scoreboard;
} GameRulesState;

#endif /* RULES_STATE_H */
