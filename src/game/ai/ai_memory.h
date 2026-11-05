#ifndef AI_MEMORY_H
#define AI_MEMORY_H

// The AI controller's private memory — AIControllerState and its per-pitch swing plan. The mirror of the
// human's ClientInputState: outside the World, never on the wire, in no snapshot, and invisible to the
// engine, which includes neither this header nor any that includes it (make guardrails holds that).

#include "foundation.h"

// The batting controller's plan for the pitch under way: what it decided to swing, and whether it
// has judged the pitch väärä. One struct because it is one lifetime — it is made during the windup,
// used through the flight, and wiped whole the first tick the controller sees no pitch being
// delivered. The controller wipes it itself, by observing the world; the engine used to reset the
// judgement at release, which was an engine write into a controller's memory.
//
// `style` is memory and not something to recompute, unlike the pre-pitch strategy it is taken from:
// the moment the ball leaves the hand, runners leave their bases and the situation the strategy
// reads changes under it. What the swing was decided AS is fixed when it is decided.
//
// `wrongPitch` is a judgement, not a predicate to recompute either: is_wrong_pitch reads the ball's
// velocity as if it had just been launched, so it is only true about the pitch near the start of
// the flight, and the decision to withdraw has to outlive that.
typedef struct _AISwingPlan {
    int decided; // the values below are this pitch's swing; nothing else here means anything until then
    int style; // the batting style it was decided as — 0 bunt, 1 normal, 2 wounding
    float power;
    float vertical;
    float angle;
    int wrongPitch; // judged väärä from the ball in the air — the swing is withdrawn, the runners called back
} AISwingPlan;

// Controller-private memory for the AI, the mirror of ClientInputState: it lives OUTSIDE
// MatchSession, never crosses the wire, and is in no snapshot — controller memory is controller-private.
// It is ALL of the AI's memory. The controller reads the World const and writes only this and its
// team's channel, and nothing in the engine writes here: a controller invalidates its own memory by
// observing durable world state. Its lifetime is the match (init_ai_controller, at match setup).
//
// Why the controller's randomness cannot live on the engine's stream: a replay from a message log
// runs no AI at all, and a networked peer never runs OUR controllers. Either way the AI's draws
// would advance the engine's stream a different number of times, so the engines re-sequence and
// diverge permanently — lockstep prevents divergence, it never heals it.
typedef struct _AIControllerState {
    unsigned int rngSeed;

    // Frames the batter has stood ready with the ball in the pitcher's hand, -1 while he is not —
    // the pause before the AI pitches, so a human batter can settle and the fielders get home.
    int batterReadyTimer;

    AISwingPlan swing;

    // What the catching controller last told the engine about where its fielder is going, and how
    // long ago. Controller memory, so it lives here and not in the World: a replay from a message
    // log runs no controller at all, and a peer never runs ours. It is also the reason the
    // controller never has to read its declaration back — it remembers what it said instead of
    // asking the engine, which is the no-read-back law made practical rather than merely obeyed.
    Vector3D lastDeclaredMoveTarget;
    int hasDeclaredMoveTarget;
    int framesSinceMoveDeclared;
    int lastSteeredFielder; // controlIndex when it last spoke; a new fielder needs telling afresh
} AIControllerState;

#endif /* AI_MEMORY_H */
