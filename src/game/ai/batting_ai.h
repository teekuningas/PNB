#ifndef BATTING_AI_H
#define BATTING_AI_H

#include "field.h"
#include "rules_state.h"
#include "intents.h"
#include "world.h"
#include "ai/ai_memory.h"

void update_batting_ai(
    const MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions,
    AIControllerState* aiController, IntentChannel* channel
);

#endif
