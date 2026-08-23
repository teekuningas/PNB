#ifndef GAME_RESET_H
#define GAME_RESET_H

#include "globals.h"

void reset_for_new_half_inning(
    MatchSession* match, const FieldPositions* field_positions, const TeamData* team_data, GameRulesState* rules
);
void reset_for_foul_play(MatchSession* match, const FieldPositions* field_positions, GameRulesState* rules);
void reset_for_next_pair(
    MatchSession* match, const FieldPositions* field_positions, const Scoreboard* scoreboard,
    const HomeRunContestState* hrcs, PlayerCounters* player_counters
);

#endif // GAME_RESET_H
