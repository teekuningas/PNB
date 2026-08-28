#ifndef BATTING_SYSTEM_H
#define BATTING_SYSTEM_H

#include "globals.h"

#define BAT_LOAD_MAX (4 * 9)
#define BAT_SWING_MAX (4 * 13)

void init_batting_system(MatchSession* match);

void seat_batter(MatchSession* match, const RefereeState* referee, const FieldPositions* fieldPositions, int index);
void start_increase_batter_angle(MatchSession* match);
void stop_increase_batter_angle(MatchSession* match);
void start_decrease_batter_angle(MatchSession* match);
void stop_decrease_batter_angle(MatchSession* match);
void select_power(MatchSession* match);
void select_angle(MatchSession* match);
void update_batting(
    MatchSession* match, const RefereeState* referee, const BetweenPitchState* betweenPitchState,
    const FieldPositions* fieldPositions, int* playSoundEffect
);
void update_batting_meter(MatchSession* match);

#endif
