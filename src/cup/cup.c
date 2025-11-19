#include "cup.h"
#include <stdlib.h>
#include <math.h>   // For log2
#include <string.h> // For memset
#include <mxml.h>

// =============================================================================
// NEW DYNAMIC CUP STRUCTURE (WORK IN PROGRESS)
// =============================================================================

// Helper to get an integer attribute from an mxml node
static int mxmlElementGetAttrAsInteger(mxml_node_t *node, const char *name, int default_value)
{
	const char* value = mxmlElementGetAttr(node, name);
	if (value == NULL) {
		return default_value;
	}
	return atoi(value);
}

// Helper to check if a value is a power of two
static int is_power_of_two(int n)
{
	return (n > 0) && ((n & (n - 1)) == 0);
}

Cup* cup_create(int num_teams, int wins_to_advance, TeamID user_team_id, int innings_per_period, const TeamID* initial_team_ids)
{
	if (!is_power_of_two(num_teams) || num_teams < 2) {
		fprintf(stderr, "Error: num_teams must be a power of two and at least 2.\n");
		return NULL;
	}

	Cup* cup = (Cup*)malloc(sizeof(Cup));
	if (cup == NULL) {
		perror("Failed to allocate Cup");
		return NULL;
	}

	cup->num_teams = num_teams;
	cup->wins_to_advance = wins_to_advance;
	cup->user_team_id = user_team_id;
	cup->innings_per_period = innings_per_period;
	cup->num_rounds = (int)log2(num_teams);
	cup->num_matches = num_teams - 1;
	cup->current_day = 0;

	cup->matches = (CupMatch*)malloc(sizeof(CupMatch) * cup->num_matches);
	if (cup->matches == NULL) {
		perror("Failed to allocate CupMatches");
		free(cup);
		return NULL;
	}

	// Initialize all matches
	for (int i = 0; i < cup->num_matches; ++i) {
		cup->matches[i].team_a_id = -1;
		cup->matches[i].team_b_id = -1;
		cup->matches[i].wins_a = 0;
		cup->matches[i].wins_b = 0;
		cup->matches[i].winner_id = CUP_MATCH_NO_WINNER;
		cup->matches[i].is_user_match = 0;
	}

	// Populate the initial round matches (last num_teams / 2 matches in the array)
	// For 8 teams, matches are indexed 0..6. Initial matches are 3,4,5,6.
	// Start index for initial round: num_matches - (num_teams / 2)
	// Skip this if initial_team_ids is NULL (used when loading from file)
	if (initial_team_ids != NULL) {
		int initial_round_start_idx = cup->num_matches - (cup->num_teams / 2);
		for (int i = 0; i < cup->num_teams / 2; ++i) {
			cup->matches[initial_round_start_idx + i].team_a_id = initial_team_ids[i * 2];
			cup->matches[initial_round_start_idx + i].team_b_id = initial_team_ids[i * 2 + 1];
		}
	}

	return cup;
}

void cup_destroy(Cup* cup)
{
	if (cup == NULL) {
		return;
	}
	free(cup->matches);
	free(cup);
}

#include <mxml.h>

// ... (previous code) ...

int cup_save(const Cup* cup, const char* filename)
{
	if (cup == NULL || filename == NULL) {
		return -1;
	}

	FILE* fp = fopen(filename, "w");
	if (fp == NULL) {
		perror("Failed to open save file for writing");
		return -1;
	}

	mxml_node_t* xml = mxmlNewXML("1.0");
	mxml_node_t* tournament_node = mxmlNewElement(xml, "tournament");

	char str_buffer[32];
	sprintf(str_buffer, "%d", cup->num_teams);
	mxmlElementSetAttr(tournament_node, "num_teams", str_buffer);
	sprintf(str_buffer, "%d", cup->wins_to_advance);
	mxmlElementSetAttr(tournament_node, "wins_to_advance", str_buffer);
	sprintf(str_buffer, "%d", cup->user_team_id);
	mxmlElementSetAttr(tournament_node, "user_team_id", str_buffer);
	sprintf(str_buffer, "%d", cup->innings_per_period);
	mxmlElementSetAttr(tournament_node, "innings_per_period", str_buffer);
	sprintf(str_buffer, "%d", cup->current_day);
	mxmlElementSetAttr(tournament_node, "current_day", str_buffer);

	mxml_node_t* matches_node = mxmlNewElement(tournament_node, "matches");

	for (int i = 0; i < cup->num_matches; ++i) {
		mxml_node_t* match_node = mxmlNewElement(matches_node, "match");
		const CupMatch* match = &cup->matches[i];

		sprintf(str_buffer, "%d", i);
		mxmlElementSetAttr(match_node, "index", str_buffer);
		sprintf(str_buffer, "%d", match->team_a_id);
		mxmlElementSetAttr(match_node, "team_a_id", str_buffer);
		sprintf(str_buffer, "%d", match->team_b_id);
		mxmlElementSetAttr(match_node, "team_b_id", str_buffer);
		sprintf(str_buffer, "%d", match->wins_a);
		mxmlElementSetAttr(match_node, "wins_a", str_buffer);
		sprintf(str_buffer, "%d", match->wins_b);
		mxmlElementSetAttr(match_node, "wins_b", str_buffer);
		sprintf(str_buffer, "%d", match->winner_id);
		mxmlElementSetAttr(match_node, "winner_id", str_buffer);
	}

	mxmlSaveFile(xml, fp, MXML_NO_CALLBACK);
	fclose(fp);
	mxmlDelete(xml);

	return 0;
}

Cup* cup_load(const char* filename)
{
	FILE* fp = fopen(filename, "r");
	if (fp == NULL) {
		// This is not an error, it just means the save file doesn't exist yet.
		return NULL;
	}

	mxml_node_t* xml = mxmlLoadFile(NULL, fp, MXML_OPAQUE_CALLBACK);
	fclose(fp);

	if (xml == NULL) {
		fprintf(stderr, "Error: Failed to load XML from %s\n", filename);
		return NULL;
	}

	mxml_node_t* tournament_node = mxmlFindElement(xml, xml, "tournament", NULL, NULL, MXML_DESCEND);
	if (tournament_node == NULL) {
		fprintf(stderr, "Error: Could not find <tournament> node in %s\n", filename);
		mxmlDelete(xml);
		return NULL;
	}

	int num_teams = mxmlElementGetAttrAsInteger(tournament_node, "num_teams", 0);
	int wins_to_advance = mxmlElementGetAttrAsInteger(tournament_node, "wins_to_advance", 0);
	int user_team_id = mxmlElementGetAttrAsInteger(tournament_node, "user_team_id", -1);
	int innings_per_period = mxmlElementGetAttrAsInteger(tournament_node, "innings_per_period", 4);
	int current_day = mxmlElementGetAttrAsInteger(tournament_node, "current_day", 0);

	if (num_teams == 0 || wins_to_advance == 0) {
		fprintf(stderr, "Error: Invalid tournament attributes in %s\n", filename);
		mxmlDelete(xml);
		return NULL;
	}

	Cup* cup = cup_create(num_teams, wins_to_advance, user_team_id, innings_per_period, NULL);
	if (cup == NULL) {
		mxmlDelete(xml);
		return NULL;
	}

	cup->current_day = current_day;

	mxml_node_t* matches_node = mxmlFindElement(tournament_node, tournament_node, "matches", NULL, NULL, MXML_DESCEND);
	if (matches_node == NULL) {
		fprintf(stderr, "Error: Could not find <matches> node in %s\n", filename);
		cup_destroy(cup);
		mxmlDelete(xml);
		return NULL;
	}

	for (mxml_node_t* match_node = mxmlFindElement(matches_node, matches_node, "match", NULL, NULL, MXML_DESCEND);
	        match_node != NULL;
	        match_node = mxmlFindElement(match_node, matches_node, "match", NULL, NULL, MXML_NO_DESCEND)) {
		int index = mxmlElementGetAttrAsInteger(match_node, "index", -1);
		if (index >= 0 && index < cup->num_matches) {
			CupMatch* match = &cup->matches[index];
			match->team_a_id = mxmlElementGetAttrAsInteger(match_node, "team_a_id", -1);
			match->team_b_id = mxmlElementGetAttrAsInteger(match_node, "team_b_id", -1);
			match->wins_a = mxmlElementGetAttrAsInteger(match_node, "wins_a", 0);
			match->wins_b = mxmlElementGetAttrAsInteger(match_node, "wins_b", 0);
			match->winner_id = mxmlElementGetAttrAsInteger(match_node, "winner_id", CUP_MATCH_NO_WINNER);
		}
	}

	mxmlDelete(xml);
	return cup;
}

void cup_simulate_round(Cup* cup, int round)
{
	if (cup == NULL) return;

	// Determine the range of match indices for the given round
	// Fixed formula: round 0 = Final, round 1 = Semis, round 2 = Quarters, etc.
	// For round R: matches from (2^R - 1) to (2^(R+1) - 2)
	int round_start_index = (1 << round) - 1;
	int round_end_index = (1 << (round + 1)) - 2;

	for (int i = round_start_index; i <= round_end_index; ++i) {
		CupMatch* match = &cup->matches[i];
		if (match->winner_id == CUP_MATCH_NO_WINNER && match->team_a_id != -1 && match->team_b_id != -1) {
			if (match->team_a_id != cup->user_team_id && match->team_b_id != cup->user_team_id) {
				// Simulate the match by randomly picking a winner
				TeamID winner = (rand() % 2 == 0) ? match->team_a_id : match->team_b_id;
				cup_update_match_result(cup, i, winner);
			}
		}
	}
}

int cup_get_user_match_index(const Cup* cup)
{
	if (cup == NULL) return -1;

	for (int i = 0; i < cup->num_matches; ++i) {
		const CupMatch* match = &cup->matches[i];
		if (match->winner_id == CUP_MATCH_NO_WINNER) { // Match is not decided
			if (match->team_a_id == cup->user_team_id || match->team_b_id == cup->user_team_id) {
				return i; // Found the user's next match
			}
		}
	}
	return -1; // No playable match for the user found
}

void cup_get_schedule_for_round(const Cup* cup, int round, int* out_match_indices, int* out_count)
{
	if (cup == NULL || round < 0 || round >= cup->num_rounds) {
		*out_count = 0;
		return;
	}

	// Fixed formula: round 0 = Final, round 1 = Semis, round 2 = Quarters, etc.
	// For round R: matches from (2^R - 1) to (2^(R+1) - 2)
	int round_start_index = (1 << round) - 1;
	int round_end_index = (1 << (round + 1)) - 2;

	*out_count = 0;
	for (int i = round_start_index; i <= round_end_index; ++i) {
		out_match_indices[*out_count] = i;
		(*out_count)++;
	}
}

// Helper to get the parent match index
// In a binary tree stored as an array, the parent of node i is at floor((i-1)/2)
static int get_parent_match_index(int child_match_index)
{
	if (child_match_index == 0) { // Final match has no parent
		return -1;
	}
	return (child_match_index - 1) / 2;
}

void cup_update_match_result(Cup* cup, int match_index, TeamID winner_team_id)
{
	if (cup == NULL || match_index < 0 || match_index >= cup->num_matches) {
		fprintf(stderr, "Error: Invalid cup or match_index in cup_update_match_result.\n");
		return;
	}

	CupMatch* match = &cup->matches[match_index];

	// Ensure the match is not already decided
	if (match->winner_id != CUP_MATCH_NO_WINNER) {
		fprintf(stderr, "Warning: Match %d already has a winner (%d). Ignoring result.\n", match_index, match->winner_id);
		return;
	}

	// Determine which team won the game and increment their wins
	if (match->team_a_id == winner_team_id) {
		match->wins_a++;
	} else if (match->team_b_id == winner_team_id) {
		match->wins_b++;
	} else {
		fprintf(stderr, "Error: Winner team ID %d does not match teams in match %d (%d vs %d).\n",
		        winner_team_id, match_index, match->team_a_id, match->team_b_id);
		return;
	}

	// Check if a team has won the entire match (reached wins_to_advance)
	TeamID match_winner_id = CUP_MATCH_NO_WINNER;
	if (match->wins_a >= cup->wins_to_advance) {
		match_winner_id = match->team_a_id;
	} else if (match->wins_b >= cup->wins_to_advance) {
		match_winner_id = match->team_b_id;
	}

	if (match_winner_id != CUP_MATCH_NO_WINNER) {
		match->winner_id = match_winner_id; // Set the match winner

		// Promote the winner to the parent match
		int parent_match_index = get_parent_match_index(match_index);
		if (parent_match_index != -1) {
			CupMatch* parent_match = &cup->matches[parent_match_index];

			// Determine if this match was the left or right child of the parent
			if (match_index == (2 * parent_match_index + 1)) { // Left child
				parent_match->team_a_id = match_winner_id;
			} else if (match_index == (2 * parent_match_index + 2)) { // Right child
				parent_match->team_b_id = match_winner_id;
			} else {
				fprintf(stderr, "Error: Match %d is not a child of parent %d. Logic error.\n", match_index, parent_match_index);
			}
		}
	}
}


// =============================================================================
// Day/Schedule System Helpers
// =============================================================================

// Get which round a match belongs to
static int get_match_round(const Cup* cup, int match_index)
{
	// Round 0: match 0
	// Round 1: matches 1-2
	// Round 2: matches 3-6
	// Round R: matches from (2^R - 1) to (2^(R+1) - 2)
	for (int round = 0; round < cup->num_rounds; round++) {
		int round_start = (1 << round) - 1;
		int round_end = (1 << (round + 1)) - 2;
		if (match_index >= round_start && match_index <= round_end) {
			return round;
		}
	}
	return -1;
}

// Get the slot index within a round (0, 1, 2, ...)

// Calculate the day range for a match based on its round and slot
// Returns start_day for the match
static int get_match_start_day(const Cup* cup, int match_index)
{
	int round = get_match_round(cup, match_index);
	if (round < 0) return -1;

	// Calculate days per match (best-of-N needs 2*N-1 days)
	int days_per_match = (cup->wins_to_advance * 2) - 1;

	// Each round starts after the previous round finishes
	// Round R starts at: sum of (matches_in_previous_rounds * days_per_match)
	int round_start_day = 0;
	for (int r = cup->num_rounds - 1; r > round; r--) {
		int matches_in_round = (1 << r);  // 2^r matches in round r
		round_start_day += matches_in_round * days_per_match;
	}

	// All matches in a round happen simultaneously (same days)
	return round_start_day;
}

int cup_get_current_round(const Cup* cup)
{
	if (cup == NULL) return -1;

	// Find the earliest round with undecided matches
	// Start from last round (quarters) and work backwards to final
	for (int round = cup->num_rounds - 1; round >= 0; round--) {
		int round_start = (1 << round) - 1;
		int round_end = (1 << (round + 1)) - 2;

		for (int i = round_start; i <= round_end; i++) {
			if (cup->matches[i].winner_id == -1 &&
			        cup->matches[i].team_a_id != -1 &&
			        cup->matches[i].team_b_id != -1) {
				return round;
			}
		}
	}

	return -1;  // Tournament complete
}

void cup_get_matches_for_day(const Cup* cup, int day, int* out_match_indices, int* out_count)
{
	if (cup == NULL) {
		*out_count = 0;
		return;
	}

	*out_count = 0;
	int days_per_match = (cup->wins_to_advance * 2) - 1;

	// Check each match to see if it's scheduled for this day
	for (int i = 0; i < cup->num_matches; i++) {
		CupMatch* match = &cup->matches[i];

		// Skip if match doesn't have both teams yet
		if (match->team_a_id == -1 || match->team_b_id == -1) {
			continue;
		}

		// Skip if match is already decided
		if (match->winner_id != -1) {
			continue;
		}

		int match_start_day = get_match_start_day(cup, i);
		int match_end_day = match_start_day + days_per_match - 1;

		// Check if current day is in this match's scheduled range
		if (day >= match_start_day && day <= match_end_day) {
			// Also check if we haven't exceeded wins needed
			int games_played = match->wins_a + match->wins_b;
			int day_offset = day - match_start_day;

			// Only schedule a game if we haven't played too many already
			if (day_offset == games_played) {
				out_match_indices[*out_count] = i;
				(*out_count)++;
			}
		}
	}
}

void cup_advance_day(Cup* cup)
{
	if (cup == NULL) return;
	cup->current_day++;
}
