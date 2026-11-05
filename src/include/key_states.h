#ifndef KEY_STATES_H
#define KEY_STATES_H

// The input devices' state as the platform layer (src/core/input.c) leaves it each frame: which keys are
// down, pressed and released, per pad. The menus read it, and the human controller turns it into intents;
// the engine never sees a key.

// keycodes

#define KEY_COUNT 11

#define KEY_A 0
#define KEY_B 1
#define KEY_PLUS 2
#define KEY_MINUS 3
#define KEY_1 4
#define KEY_2 5
#define KEY_UP 6
#define KEY_DOWN 7
#define KEY_LEFT 8
#define KEY_RIGHT 9
#define KEY_HOME 10

// Number of real human input devices (pads). Slot 0 = pad 1, slot 1 = pad 2.
#define HUMAN_PAD_COUNT 2

// The first index is a TeamControlMode value used directly as an input-slot
// index, so the array is sized to CONTROL_AI + 1 = 3 rows. Slots 0 and 1 are
// the human pads; slot 2 (CONTROL_AI) is a PHANTOM row that input.c never
// writes — an AI-controlled team reading down[CONTROL_AI][...] always sees
// zeros. Callers guard reads with `if (control != CONTROL_AI)`; the AI has no
// keys at all and declares its intent directly. Overloading TeamControlMode as an array
// index is a known minor smell (tracked as debt); prefer
// HUMAN_PAD_COUNT when iterating real devices so watcher-facing code (e.g. the
// pause key, the game-over screen) never touches the phantom slot.
//
// The three readings of a key, and they are not interchangeable — which one a gesture uses is a
// design decision about that gesture:
//   down     — held, every frame it is held. For gestures whose meaning IS the holding (a throw
//              gathers while KEY_2 is down).
//   pressed  — the one frame it went down. The EARLY-caught click: what a meter sampler wants,
//              because waiting for the release spends a frame the swing's hit window cannot spare.
//   released — the one frame after it came up. The LATE-caught click: what a discrete ANSWER wants
//              (press and think), what a hold gesture is completed by, and what the base-run
//              double-press window is measured between.
typedef struct _KeyStates {
    int released[3][KEY_COUNT];
    int pressed[3][KEY_COUNT];
    int down[3][KEY_COUNT];
} KeyStates;

#endif /* KEY_STATES_H */
