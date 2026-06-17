#ifndef THROWING_SYSTEM_H
#define THROWING_SYSTEM_H

#include "globals.h"

#define THROW_MAX 50

void init_throwing_system(MatchSession* match);

void prepare_throw(MatchSession* match, const FieldPositions* fieldPositions, BaseID base);

void throw_release(MatchSession* match);
void throw_load(MatchSession* match, BaseID base);
void fielder_move(MatchSession* match, int direction);
void fielder_stop_move(MatchSession* match, int direction);
void drop_ball(MatchSession* match);
void update_controlled_player_speed(MatchSession* match);

#endif
