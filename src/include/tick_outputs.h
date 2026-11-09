#ifndef TICK_OUTPUTS_H
#define TICK_OUTPUTS_H

#include <stdbool.h>
#include "foundation.h"
#include "player_events.h"

typedef struct _PlayerTickFacts {
    PlayerActivityEvents activity;
    bool holds_ball;
    bool throw_recovering;
} PlayerTickFacts;

// What the tick hands out rather than keeps: the sound to play and the announcement to show, for the
// client; the screen a period's end asks for and how the match ended, for the session. None of it is World
// state, so whether anybody consumed it can never reach a future frame.

// Absence of a controller does not mean every runner from third has resolved: a live runner may
// have lost safety there.
typedef enum { THIRD_NO_CONTROLLER, THIRD_CONTROLLER_ON_BASE, THIRD_CONTROLLER_OFF_BASE } ThirdBaseControl;

// Facts that a final snapshot cannot recover: route starts and pitch release can be followed by
// arrivals or resets in the same tick. Base control is observed before ADJUDICATE; enforcement
// can start another route after game flow has advanced. No view state or rendering commands.
typedef struct _TickFacts {
    bool pitch_released;
    bool homeward_run_started;
    bool ball_home;
    ThirdBaseControl third_base_control;
    bool world_reset;
    bool enforcement_completed;
    bool homeward_run_started_in_enforcement;
    PlayerTickFacts simulated_players[PLAYERS_IN_TEAM * 2 + JOKER_COUNT];
    PlayerTickFacts enforced_players[PLAYERS_IN_TEAM * 2 + JOKER_COUNT];
} TickFacts;

typedef enum {
    EVENT_NONE = 0,
    EVENT_OUT = 1,
    EVENT_WOUNDED = 2,
    EVENT_RUN_SCORED = 3,
    EVENT_OUT_OF_BOUNDS = 4,
    EVENT_STRIKE = 5,
    EVENT_BALL = 6,
    EVENT_INNING_ENDING = 7,
    EVENT_NEXT_PAIR = 8,
    EVENT_TWO_RUNS_SCORED = 9
} GameEventType;

// soundcodes

#define SOUND_MENU 1
#define SOUND_SWING 2
#define SOUND_CATCH 3

// Which screen the program is on; the tick may ask for another (a period or the match ending).
typedef enum { SCREEN_LOADING, SCREEN_MAIN_MENU, SCREEN_GAME } ScreenState;

// How a match ended, written by ENFORCE when it ends and read by the game-over screen.
typedef struct _GameConclusion {
    int winner;
    int period0Runs[2];
    int period1Runs[2];
    int period2Runs[2];
    int period3Runs[2];
    int isCupGame;
} GameConclusion;

#endif /* TICK_OUTPUTS_H */
