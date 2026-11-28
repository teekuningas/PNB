#include "base_control.h"
#include "field.h"
#include "rules_state.h"
#include "world.h"
#include "vector_math.h"

int get_base_controller(const MatchSession* game, const RefereeState* referee, BaseID base)
{
    if (base < 0 || base >= BASE_COUNT) return -1;

    int bestCandidate = -1;
    int highestBaseAtPitch = -2;

    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        if (referee->batting_players[i].current_safety_base == base &&
            game->players[i].batting.presence.base_id == base) {
            int baseAtPitch = (int)referee->batting_players[i].base_at_pitch_start;
            if (baseAtPitch > highestBaseAtPitch) {
                highestBaseAtPitch = baseAtPitch;
                bestCandidate = i;
            }
        }
    }
    return bestCandidate;
}

int get_ball_at_base_index(const MatchSession* match, const FieldPositions* field_positions)
{
    if (match->ball.holder == -1) {
        return -1; // No one has ball
    }

    // §15/§16 proximity proxy: a held ball tracks its holder's XZ position (snapped on catch and
    // moved in lockstep by update_ball_to_player), so ball.location is the single position read here.
    Vector3D ballLoc = match->ball.location;

    // Check Home Base (Index 0)
    // Special check: inside homeline-middlepoint centered disk and at homebase side
    float dx = ballLoc.x - field_positions->pitch_plate.x;
    float dz = ballLoc.z - HOME_LINE_Z;
    if (ballLoc.z > HOME_LINE_Z && vec3_is_small_enough_circle_xz(dx, dz, HOME_RADIUS)) {
        return 0; // Home Base
    }

    // Check Bases 1-3
    for (int i = 1; i < BASE_COUNT; i++) {
        Vector3D baseLoc;
        if (i == 1)
            baseLoc = field_positions->first_base;
        else if (i == 2)
            baseLoc = field_positions->second_base;
        else if (i == 3)
            baseLoc = field_positions->third_base;
        else
            continue;

        dx = ballLoc.x - baseLoc.x;
        dz = ballLoc.z - baseLoc.z;

        if (vec3_is_small_enough_circle_xz(dx, dz, BASE_RADIUS)) {
            return i;
        }
    }

    return -1;
}

int ball_is_home(const MatchSession* match, const FieldPositions* field_positions)
{
    return get_ball_at_base_index(match, field_positions) == 0;
}
