#include "player_utils.h"
#include "rules_batting_order.h"

int get_active_batter_index(const MatchSession* game)
{
    if (!game) return -1;

    // Based on game_frame.c and common_logic.c:
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
    for (int i = 0; i < JOKER_COUNT; i++) {
        const int joker = game->pII.jokerIndices[i];
        if (batter_seat_verdict(joker, inTurn, spent, (int)game->playerInfo[joker].bTPI.joker) == SEAT_ALLOWED) {
            out[count++] = joker;
        }
    }
    return count;
}
