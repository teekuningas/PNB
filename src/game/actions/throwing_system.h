#ifndef THROWING_SYSTEM_H
#define THROWING_SYSTEM_H

#include "globals.h"

#define THROW_MAX 50 // longest possible windup, in frames; the engine-owned animation clock's ceiling

// Default declared throw power [0,1]. The AI declares this; the human path uses it as a placeholder until
// the client-local charge widget (hold-time → power) lands in the human-path commit (PLAN §4.12 sub-step 2).
#define THROW_POWER_DEFAULT (3.0f / 4.0f)

void init_throwing_system(MatchSession* match);

void prepare_throw(MatchSession* match, const FieldPositions* fieldPositions, BaseID base);

void throw_release(MatchSession* match);
void throw_load(MatchSession* match, BaseID base, float power);
void fielder_move(MatchSession* match, int direction);
void fielder_stop_move(MatchSession* match, int direction);
void drop_ball(MatchSession* match);
void update_controlled_player_speed(MatchSession* match);

#endif
