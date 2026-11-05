#ifndef TICK_OUTPUTS_H
#define TICK_OUTPUTS_H

// What the tick hands out rather than keeps: the sound to play and the announcement to show, for the
// client; the screen a period's end asks for and how the match ended, for the session. None of it is World
// state, so whether anybody consumed it can never reach a future frame.

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
