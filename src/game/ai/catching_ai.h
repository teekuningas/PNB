#ifndef CATCHING_AI_H
#define CATCHING_AI_H

#include "globals.h"

void init_catching_ai(AIState* aiState);
void update_catching_ai(
    MatchSession* match, const GameRulesState* rules, AIControllerState* aiController, IntentChannel* channel
);
void move_controlled_player_to_location(MatchSession* match, Vector3D* target);
void throw_ball_to_base(MatchSession* match, BaseID base, IntentChannel* channel);

#endif
