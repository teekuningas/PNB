#ifndef SESSION_H
#define SESSION_H

// The session — StateInfo, the assembly point the program runs on: the screen it is on, the two worlds, the
// tick's channels and outputs, and the controllers' memories, which it holds only by pointer and names only
// by forward declaration: whoever needs to see inside one includes that controller's own header. A stage
// is never handed this struct; update_game_frame destructures it into each stage's honest arguments.

#include "field.h"
#include "world.h"
#include "rules_state.h"
#include "intents.h"
#include "tick_outputs.h"
#include "key_states.h"
#include "team_data.h"

// Held by pointer and defined by their owners: the cup (src/cup), and the two controllers' memories.
typedef struct _Cup Cup;
typedef struct _ClientInputState ClientInputState;
typedef struct _AIControllerState AIControllerState;

typedef struct _StateInfo {
    // Main menu or game screen active
    ScreenState screen;
    int changeScreen;
    int updated;
    int numTeams;
    int playSoundEffect;
    int stopSoundEffect;
    int soundEnabled;
    int currently_played_cup_match_index; // Used to pass match context to/from game screen
    // Whether the match is paused: SESSION control, which decides whether a tick runs at all, so it is not
    // game state and lives with whoever runs the session rather than in the World. The player pauses it
    // from the screen, the state validator pauses it on a failed check, and starting the game screen
    // clears it.
    int paused;
    // addresses to important information structures
    KeyStates* keyStates;
    TeamData* teamData;
    FieldPositions* fieldPositions;
    MatchSession* match;
    ClientInputState* clientInput; // client-local input memory — NOT synced (see ClientInputState)
    AIControllerState* aiController; // controller-private AI memory — NOT synced (see AIControllerState)
    // The per-team intent channels — held BY VALUE, because they are neither a world nor a memory:
    // they are the tick's input, alive only between the CONTROL stage and the INGEST gate of the same
    // frame. Storing them here just gives that per-tick value a place to sit on this machine; the
    // state validator holds them to being empty at every frame boundary.
    IntentChannels channels;
    // What the referee pronounced this tick — OUT, STRIKE, RUN — for the screen to show. An OUTPUT of the
    // tick, beside the channels that are its input and for the same reason held by value: the referee
    // empties it at the top of every tick and the screen consumes it, and it is in no snapshot, so whether
    // anybody displayed an announcement can never reach a future frame.
    GameEventType announcement;
    TickFacts tick_facts; // complete tick observations, read-only to their client consumers
    GameRulesState* rules;
    Cup* cup; // New dynamic tournament state
    GameConclusion* gameConclusion;
} StateInfo;

#endif /* SESSION_H */
