#include "catching_ai_strategy.h"
#include "base_logic.h"
#include <math.h>

static float distance_xz(const Vector3D* a, const Vector3D* b)
{
    float dx = a->x - b->x;
    float dz = a->z - b->z;
    return (float)sqrt(dx * dx + dz * dz);
}

Vector3D chase_point(const Vector3D* fielder, const Vector3D* predicted)
{
    return (distance_xz(predicted, fielder) <= AI_MOVE_DEAD_ZONE) ? *fielder : *predicted;
}

Vector3D carry_to_throw_point(const Vector3D* carrier, const Vector3D* base)
{
    float dx = carrier->x - base->x;
    float dz = carrier->z - base->z;
    float d = (float)sqrt(dx * dx + dz * dz);

    // Standing exactly on the base leaves no direction to step back along. Any fixed one will do as
    // long as it is the same every time — a controller that picked a direction from the world here
    // would be inventing state, and one that picked at random would break replay.
    if (d < EPSILON) {
        dx = 0.0f;
        dz = 1.0f;
        d = 1.0f;
    }

    float t = AI_THROW_STANDOFF / d;
    Vector3D out = *carrier;
    out.x = base->x + dx * t;
    out.z = base->z + dz * t;
    return out;
}

MoveDeclaration
decide_move_declaration(const Vector3D* desired, int hasDeclared, const Vector3D* lastDeclared, int framesSinceDeclared)
{
    MoveDeclaration out;
    out.point = *desired;

    if (!hasDeclared) {
        out.declare = 1;
    } else if (distance_xz(desired, lastDeclared) > AI_MOVE_DECLARE_THRESHOLD) {
        out.declare = 1;
    } else {
        out.declare = (framesSinceDeclared >= AI_MOVE_HEARTBEAT_FRAMES);
    }

    return out;
}

int should_ai_throw(
    const PlayerIndexInfo* playerIndices, int catcherNearHome, int replacerIndex, int replacerStage, int replacerBase,
    int replacerMoving, int targetBase
)
{
    int hasBallIndex = playerIndices->hasBallIndex;
    int catcherIndex = playerIndices->catcherOnBaseIndex[targetBase];

    int shouldThrow = 0;
    if (hasBallIndex != -1 && catcherIndex != -1) {
        // Only consider throwing if I am NOT the baseman for this base
        if (hasBallIndex != catcherIndex) {
            // Check normal catcher
            if (catcherNearHome == 1) {
                shouldThrow = 1;
            }

            // Check replacer
            if (hasBallIndex != replacerIndex) {
                if (replacerStage == 1 && replacerBase == targetBase && replacerMoving == 0) {
                    shouldThrow = 1;
                }
            }
        }
    }

    return shouldThrow;
}

int should_ai_drop_ball(
    const RefereeState* ref, const BetweenPitchState* betweenPitchState, BaseID runner3BaseAtPitchStart,
    int runner3IsOnBase, BaseID runner2BaseAtPitchStart, int runner2IsOnBase, int catcherHomeIndex, int hasBallIndex
)
{
    // Tactical drop for bases loaded (§30 taktinen pudotus):
    // Dropping the ball in a fly-ball situation can allow for a force play
    // when we want to get an OUT at home base or create a double play.
    // We only do this when 2nd and 3rd bases are occupied.
    if (ref->woundingEvaluationActive == 1) {
        if (runner3BaseAtPitchStart == BASE_THIRD && runner3IsOnBase == 1 && runner2BaseAtPitchStart == BASE_SECOND &&
            runner2IsOnBase == 1 && catcherHomeIndex == hasBallIndex) {
            return 1;
        }
    }
    return 0;
}
BaseID determine_lead_base(const CatchingRunnerInfo* runners, int runnerCount, int randomValue)
{
    BaseID leadBase = BASE_NONE;
    int i;
    for (i = 0; i < runnerCount; i++) {
        if (runners[i].isOnBase == 0 && runners[i].takingFreeWalk == 0) {
            if (base_cmp(runners[i].base, leadBase) > 0) {
                if (runners[i].leading == 0) {
                    leadBase = runners[i].base;
                } else {
                    if (randomValue == 0) {
                        leadBase = base_get_prev(runners[i].base);
                    }
                }
            }
        }
    }
    return leadBase;
}
