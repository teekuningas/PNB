#ifndef INGEST_H
#define INGEST_H

#include "actions/pitching_system.h" // PitchValues — what a pitch declaration contributes
#include "field.h"
#include "rules_state.h"
#include "intents.h"
#include "world.h"
#include "actions/throwing_system.h" // ThrowValues — what a throw declaration contributes

// What ingestion leaves behind: this tick's commands, in engine terms. It is a local of the tick
// (update_game_frame hands it from INGEST to ACTUALIZE), so it cannot outlive the tick, cannot be
// snapshot, and cannot be read by a controller — the same guarantees the channel itself has, carried through the
// interpretation step. A held value with a lifetime of its own (a destination, a swing's power) is written straight
// into the engine state that owns it instead; the pitch's and the throw's values come through here because what they
// contribute to — a windup — may not exist until the actualizer begins it this very tick.
typedef struct _IngestedCommands {
    int change_player; // hand control to the next fielder
    int drop_ball; // put the held ball down
    FreeWalkAction take_free_walk; // FREE_WALK_IDLE when no answer was given this tick
    RunIntent base_run[BASE_COUNT]; // per base, RUN_NONE where nothing was commanded
    int base_runner[BASE_COUNT]; // who the command was for: the base's runner at the top of the tick
    int seat_batter_index; // who to seat as the next batter; -1 when nobody was named this tick
    int swing_angle_declared; // 0 when the batting side said nothing about its aim this tick
    float swing_angle;
    int swing_pass; // the batting side withdrew its swing this tick
    PitchValues pitch;
    ThrowValues throw;
} IngestedCommands;

// THE INGEST GATE — stage 2 of the tick. Drains both teams' channels and leaves them empty: the channel is
// an input to this tick, not somewhere intent lives. Each message is asked its permission against the
// world as it stands at the top of the tick; what survives becomes the commands ACTUALIZE reads, or is
// written straight into the engine state that holds a declared value of its own (a destination, a
// swing's power).
IngestedCommands ingest_intents(
    MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions, IntentChannels* channels
);

#endif /* INGEST_H */
