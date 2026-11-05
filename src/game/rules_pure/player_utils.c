#include "player_utils.h"
#include "rules_state.h"
#include "world.h"
#include "rules_batting_order.h"

int get_active_batter_index(const MatchSession* game)
{
    if (!game) return -1;

    // Indices 0 to 11 (PLAYERS_IN_TEAM + JOKER_COUNT) are always the current batting team.
    // Indices 12 to 20 are the fielding team.

    int limit = PLAYERS_IN_TEAM + JOKER_COUNT;

    for (int i = 0; i < limit; i++) {
        // We look for the player specifically marked as AT_BAT
        if (game->playerInfo[i].bTPI.state == PLAYER_STATE_AT_BAT) {
            return i;
        }
    }

    return -1;
}

int get_batting_team_index(const Scoreboard* sb)
{
    return (sb->inning + sb->playsFirst + sb->period) % 2;
}

void batting_team_jokers(const Scoreboard* sb, int out[JOKER_COUNT])
{
    const TeamInfo* team = &sb->teams[get_batting_team_index(sb)];
    for (int k = 0; k < JOKER_COUNT; k++) {
        out[k] = team->batterOrder[PLAYERS_IN_TEAM + k];
    }
    // Insertion sort into roster order: three elements.
    for (int k = 1; k < JOKER_COUNT; k++) {
        const int index = out[k];
        int m = k;
        while (m > 0 && out[m - 1] > index) {
            out[m] = out[m - 1];
            m--;
        }
        out[m] = index;
    }
}

int team_is_ai(TeamControlMode control)
{
    return control == CONTROL_AI;
}

int list_batter_candidates(const MatchSession* game, const Scoreboard* sb, const HalfInningState* his, int* out)
{
    const int battingTeamIndex = get_batting_team_index(sb);
    const TeamInfo* team = &sb->teams[battingTeamIndex];
    const int inTurn = team->batterOrder[team->batterOrderIndex];
    const int spent = his->lastBatter.regularOrderSpent;
    int count = 0;

    if (batter_seat_verdict(inTurn, inTurn, spent, (int)game->playerInfo[inTurn].bTPI.joker) == SEAT_ALLOWED) {
        out[count++] = inTurn;
    }
    int jokers[JOKER_COUNT];
    batting_team_jokers(sb, jokers);
    for (int i = 0; i < JOKER_COUNT; i++) {
        const int joker = jokers[i];
        if (batter_seat_verdict(joker, inTurn, spent, (int)game->playerInfo[joker].bTPI.joker) == SEAT_ALLOWED) {
            out[count++] = joker;
        }
    }
    return count;
}
