#ifndef EXECUTE_ACTIONS_H
#define EXECUTE_ACTIONS_H

#include "globals.h"

void execute_actions(
    MatchSession* match, const ClientInputState* clientInput, const GameRulesState* rules,
    const FieldPositions* fieldPositions, int* playSoundEffect
);
void init_execute_actions(MatchSession* match, ClientInputState* clientInput);
void generic_sling_ball(BallInfo* ballInfo, float x, float y, float z);
void update_meters(MatchSession* match, const ClientInputState* clientInput);
void ai_update(
    MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions, unsigned int* rng_seed
);

#endif /* EXECUTE_ACTIONS_H */
