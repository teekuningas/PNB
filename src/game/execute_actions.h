#ifndef EXECUTE_ACTIONS_H
#define EXECUTE_ACTIONS_H

#include "globals.h"

void execute_actions(
    MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions, int* playSoundEffect
);
void init_execute_actions(MatchSession* match, ClientInputState* clientInput);
void generic_sling_ball(BallInfo* ballInfo, float x, float y, float z);
void update_meters(MatchSession* match, const ClientInputState* clientInput);

// The AI half of the CONTROL stage (pipeline stage 1, at the frame top — game_frame.c). It is declared
// from *this* header only for historical reasons, and that is a lie about the include graph: the
// execution stage's header should not export a controller. Its home is src/game/ai/ai_controller.c,
// and the carve-out rides the channel slice, which rewrites this file into the INGEST gate anyway.
// (Deliberately not done earlier: a new src/*.c plus a header naming MatchSession would push two
// `make guardrails` ratchets up — globals.h includers and files awaiting the function-quality audit
// — and those floors may only fall.)
void ai_update(
    MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions,
    AIControllerState* aiController
);

#endif /* EXECUTE_ACTIONS_H */
