#ifndef GAME_RESET_H
#define GAME_RESET_H

#include "globals.h"

void reset_for_new_half_inning(
    MatchSession* match, const FieldPositions* field_positions, const TeamData* team_data, const GameRulesState* rules
);
void reset_for_foul_play(MatchSession* match, const FieldPositions* field_positions, const GameRulesState* rules);
void reset_for_next_pair(
    MatchSession* match, const FieldPositions* field_positions, const Scoreboard* scoreboard,
    const HomeRunContestState* hrcs
);

void initialize_action_info(MatchSession* match);

#endif // GAME_RESET_H
