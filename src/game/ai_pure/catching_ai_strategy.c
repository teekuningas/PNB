#include "catching_ai_strategy.h"
#include "rules_state.h"
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

// Matches ReplacementState.REPLACEMENT_ACTIVE in world.h (kept out of catching_ai_strategy's includes).
#define AI_REPLACEMENT_ACTIVE 1

int should_ai_throw(
    int holder, int baseman, int catcherNearHome, int replacerIndex, int replacerStage, int replacerBase,
    int replacerMoving, int targetBase
)
{
    if (holder == -1) return 0;

    // Throw to the regular baseman when someone else holds the ball and the baseman is on his post.
    if (baseman != -1 && holder != baseman && catcherNearHome == 1) {
        return 1;
    }

    // Throw to an idle cover standing on the target base — including when the baseman himself
    // fielded the ball away from his post (catcherNearHome == 0).
    if (replacerIndex != -1 && holder != replacerIndex && !(holder == baseman && catcherNearHome == 1) &&
        replacerStage == AI_REPLACEMENT_ACTIVE && replacerBase == targetBase && replacerMoving == 0) {
        return 1;
    }

    return 0;
}

int should_ai_drop_ball(
    const RefereeState* ref, BaseID runner3BaseAtPitchStart, int runner3IsOnBase, BaseID runner2BaseAtPitchStart,
    int runner2IsOnBase, int catcherHomeIndex, int hasBallIndex
)
{
    // Tactical drop for bases loaded (§30 taktinen pudotus):
    // Dropping the ball in a fly-ball situation can allow for a force play
    // when we want to get an OUT at home base or create a double play.
    // We only do this when 2nd and 3rd bases are occupied.
    if (ref->wounding_evaluation_active == 1) {
        if (runner3BaseAtPitchStart == BASE_THIRD && runner3IsOnBase == 1 && runner2BaseAtPitchStart == BASE_SECOND &&
            runner2IsOnBase == 1 && catcherHomeIndex == hasBallIndex) {
            return 1;
        }
    }
    return 0;
}
static int target_base_priority(BaseID target, int is_scoring_play)
{
    if (is_scoring_play && target == BASE_HOME) return 4;
    if (target == BASE_THIRD) return 3;
    if (target == BASE_SECOND) return 2;
    if (target == BASE_FIRST) return 1;
    return 0;
}

BaseID determine_throw_target_base(const CatchingRunnerInfo* runners, int runnerCount, int randomValue)
{
    BaseID bestTarget = BASE_HOME;
    int bestPriority = 0;

    for (int i = 0; i < runnerCount; i++) {
        if (runners[i].is_on_base != 0 || runners[i].taking_free_walk != 0) continue;

        BaseID candidate = BASE_NONE;
        int isScoringPlay = 0;

        if (runners[i].leading != 0) {
            if (randomValue == 0 && runners[i].base != BASE_HOME) {
                candidate = runners[i].base;
            }
        } else if (runners[i].going_forward != 0) {
            BaseID next = base_get_next(runners[i].base);
            if (next == BASE_HOME_SCORED) {
                candidate = BASE_HOME;
                isScoringPlay = 1;
            } else if (next != BASE_NONE) {
                candidate = next;
            }
        } else if (runners[i].base != BASE_HOME) {
            candidate = runners[i].base;
        }

        if (candidate != BASE_NONE) {
            int priority = target_base_priority(candidate, isScoringPlay);
            if (priority > bestPriority) {
                bestPriority = priority;
                bestTarget = candidate;
            }
        }
    }

    return bestTarget;
}
