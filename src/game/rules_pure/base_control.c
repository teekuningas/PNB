#include "base_control.h"
#include "vector_math.h"

int get_base_controller(const MatchSession* game, const RefereeState* referee, BaseID base)
{
    if (base < 0 || base >= BASE_COUNT) return -1;

    int bestCandidate = -1;
    int highestBaseAtPitch = -2;

    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        if (referee->battingPlayers[i].currentSafetyBase == base && game->playerInfo[i].bTPI.baseId == base) {
            int baseAtPitch = (int)referee->battingPlayers[i].baseAtPitchStart;
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
    if (match->pII.hasBallIndex == -1) {
        return -1; // No one has ball
    }

    // Use catcher position directly or ball position?
    // Legacy logic uses ballInfo.location, assuming ball is with player.
    Vector3D ballLoc = match->ballInfo.location;

    // Check Home Base (Index 0)
    // Special check: inside homeline-middlepoint centered disk and at homebase side
    float dx = ballLoc.x - field_positions->pitchPlate.x;
    float dz = ballLoc.z - HOME_LINE_Z;
    if (ballLoc.z > HOME_LINE_Z && vec3_is_small_enough_circle_xz(dx, dz, HOME_RADIUS)) {
        return 0; // Home Base
    }

    // Check Bases 1-3
    for (int i = 1; i < BASE_COUNT; i++) {
        Vector3D baseLoc;
        if (i == 1)
            baseLoc = field_positions->firstBase;
        else if (i == 2)
            baseLoc = field_positions->secondBase;
        else if (i == 3)
            baseLoc = field_positions->thirdBase;
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
