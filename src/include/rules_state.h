#ifndef RULES_STATE_H
#define RULES_STATE_H

// The legal world: the referee's state machines and per-player verdicts, the half-inning, the pitch's
// adjudication, the scoreboard — GameRulesState. Only the referee writes it at runtime; every other stage
// receives it const. The physical World's header does not include this one, so code that holds only the
// World cannot even name the law.

#include "foundation.h"

typedef enum { PITCH_RESULT_NONE = 0, PITCH_RESULT_STRIKE = 1, PITCH_RESULT_BALL = 2 } PitchResult;

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
    BaseID baseAtPitchStart; // Where was player when pitch started

    // === CURRENT SAFETY STATUS ===
    BaseID currentSafetyBase; // Which base has their safety (-1 if none)
    int hasScored; // Logical scored status (decided by Referee)
    int runOfHonorScored; // Logical run of honor scored status (decided by Referee)

    // === PENDING RUNS (Milestone 17) ===
    int hasPendingRun; // Arrived at home while ball in air
    int hasPendingRunOfHonor; // Arrived at 3rd (homerun) while ball in air

} RefereePlayerState;

typedef struct _RefereeState {
    // Per-player tracking
    RefereePlayerState battingPlayers[PLAYERS_IN_TEAM + JOKER_COUNT];

    // Global events
    int strikesAtPitchStart; // Snapshot of strikes when pitch started

    // Wounding evaluation state (Referee monitors and decides)
    int woundingEvaluationActive; // Referee is evaluating a potential wounding catch
    int woundingEvaluationFinished; // Milestone 17: Flag set when evaluation completes (catch confirmed)
    int woundingEvaluationTimer; // Timer counting up during evaluation period

    // Flow Control State Machines (Milestone 17 consolidation)
    FoulPlayState foulState; // Out-of-bounds transition state
    int foulTimer; // Timer for foul play grace period

    HomeRunPairState nextPairTransitionState; // Next-pair transition state
    int nextPairTimer; // Timer for next-pair transition

    EndOfInningTransitionState endOfInningState; // End-of-inning transition state
    int endInningTimer; // Timer for end-of-inning transition
    PeriodTransitionType periodTransition; // Result of period logic (set at DETECTED→RESETTING)
    int periodTransitionWinner; // Winner index (valid when periodTransition == GAME_OVER)

    // Run of Honor tracking (Homerun Contest)
    int ballInThirdBaseSincePitch; // Has ball been held at 3rd base by catching team since pitch started

    int homerunPairHasPitch; // Has at least one pitch been released for current pair
} RefereeState;

typedef struct _TeamInfo {
    int value; // which team, like 1 for ankkurit, 2 for vimpeli etc.
    TeamControlMode control; // who controls, 0 player 1, 1, player 2, 2 AI
    int runs; // how many runs has this team
    int period0Runs;
    int period1Runs;
    int period2Runs;
    int period3Runs;
    int batterRunnerIndices[2][5];
    int batterOrder[12]; // this teams' batting order, used in many innings
    int batterOrderIndex; // what player is next
} TeamInfo;

// §12 Vuoronvaihto — the designation that decides when a batting turn is spent.
// Lifetime: one half-inning, which is exactly the rule's own lifetime: the designation is re-made on
// every even-numbered run and dies with the inning. Referee-owned; the decisions themselves are the
// pure predicates in rules_pure/rules_side_change.h.
typedef struct _LastBatterState {
    int designatedIndex; // the viimeinen lyöjä; -1 until the half-inning's first batter takes the bat
    int lastRegularIndex; // most recent REGULAR player to take the bat; -1 if none yet (jokers don't count)
    int hasBattedAgain; // the designated player has taken the bat since being designated

    // Two different questions, and conflating them was a real §12 defect. Both are §12(2)/(3), but
    // they are answered at different moments and by different amounts of the rule.
    //
    // WHO MAY BAT NEXT. `regularOrderSpent` is the batting-order clause on its own: the order has
    // come round to the designated last batter, so no further REGULAR may be seated and only a
    // joker can extend the turn (§27: "paloa ei tuomita, mikäli jokeripelaaja otetaan lyömään").
    // It settles the instant the previous batter enters and never wobbles afterwards, which is what
    // makes it safe to ask at the moment a new batter is chosen.
    //
    // DOES THE HALF-INNING END. `turnExhausted` adds the rule's other trigger — that the batter has
    // "muuttunut lopullisesti etenijäksi", read here as having lost safety at home. That one is
    // genuinely transient: a batter stops standing at the plate the moment he sets off but keeps
    // home until he is established elsewhere, and during a foul he is restored to the plate and
    // takes home back. It belongs to the verdict (with the ball home and no jokers left), never to
    // the choice of batter.
    int regularOrderSpent;
    int turnExhausted;
} LastBatterState;

// MILESTONE 7.5 - Focused Structs
typedef struct _HalfInningState {
    int outs;
    int balls;
    int strikes;
    int runsInTheInning;
    int endPeriod; // Rule state: period should end
    // §7: "Joukkue voi käyttää jokaisessa sisävuorossa kolmea eri jokeripelaajaa, kerran kutakin."
    // A genuinely consumable resource, and per half-inning — so it lives with the half-inning.
    int jokersLeft;
    LastBatterState lastBatter;
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
    int catchHasBeenMade; // Fly ball was caught (promoted from catchMade)
    int hasBallHitGround; // Ball has touched ground (promoted from ballHitGround)
    PitchResult pitchResult; // Referee's pitch adjudication (NONE until resolved)
    BatOutcome batOutcome; // Bat hit or missed (promoted from ballHitByBat/ballMissedByBat)
} BetweenPitchState;

typedef struct _HomeRunContestState {
    int runnerBatterPairCounter;
} HomeRunContestState;

typedef struct _Scoreboard {
    TeamInfo teams[2];
    int halfInningsInPeriod;
    int inning;
    int period;
    GamePeriodMode mode; // NEW field for Milestone 7
    int pairCount;
    int playsFirst;
    int isCupGame;
} Scoreboard;

// GameRulesState — Referee-owned state, structurally separated from physical/action state.
// Only the referee writes these at runtime. Other stages receive const pointers.
typedef struct _GameRulesState {
    RefereeState referee;
    HalfInningState halfInningState;
    BetweenPitchState betweenPitchState;
    HomeRunContestState homeRunContestState;
    Scoreboard scoreboard;
} GameRulesState;

#endif /* RULES_STATE_H */
