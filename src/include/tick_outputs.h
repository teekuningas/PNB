#ifndef TICK_OUTPUTS_H
#define TICK_OUTPUTS_H

// The tick's outputs for the client alone — the sound to play, the announcement to show. Each is an
// out-parameter of the tick, emptied by its producer every tick and never World state, so whether a screen
// consumed one can never reach a future frame.

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

#endif /* TICK_OUTPUTS_H */
