#ifndef CUP_H
#define CUP_H

#include "globals.h"

typedef int TeamID;

#define CUP_MATCH_NO_WINNER ((TeamID)-1)

typedef struct _CupMatch {
	TeamID team_a_id;
	TeamID team_b_id;
	int wins_a;
	int wins_b;
	TeamID winner_id; // ID of the winning team, or CUP_MATCH_NO_WINNER if undecided.
	int is_user_match;
} CupMatch;

typedef struct _Cup {
	int num_teams;
	int wins_to_advance;
	TeamID user_team_id;
	int num_rounds;
	int num_matches;
	int innings_per_period;
	int current_day;
	CupMatch* matches;
} Cup;

// New API function prototypes
Cup* cup_create(int num_teams, int wins_to_advance, TeamID user_team_id, int innings_per_period, const TeamID* initial_team_ids);
void cup_destroy(Cup* cup);
int cup_save(const Cup* cup, const char* filename);
Cup* cup_load(const char* filename);
void cup_update_match_result(Cup* cup, int match_index, TeamID winner_team_id);
void cup_simulate_round(Cup* cup, int round);
int cup_get_user_match_index(const Cup* cup);
void cup_get_schedule_for_round(const Cup* cup, int round, int* out_match_indices, int* out_count);

// Day/schedule helpers
int cup_get_current_round(const Cup* cup);
void cup_get_matches_for_day(const Cup* cup, int day, int* out_match_indices, int* out_count);
void cup_advance_day(Cup* cup);

#endif /* CUP_H */
