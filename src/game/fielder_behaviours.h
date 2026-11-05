#ifndef FIELDER_BEHAVIOURS_H
#define FIELDER_BEHAVIOURS_H

#include "globals.h"

// Each moving fielder learns whether he is near his post.
void check_if_near_home_location(MatchSession* match);
// A baseman away from his base is covered by another fielder.
void basemen_replacements(MatchSession* match);
// Fielders with nothing to do walk back to their posts.
void move_idling_players_to_home_location(MatchSession* match);
// When a replan is asked for, rank the fielders for the loose ball and send the best placed after it.
void rank_players_and_move_them(MatchSession* match);

#endif /* FIELDER_BEHAVIOURS_H */
