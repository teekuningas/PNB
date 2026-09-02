#include "test_helpers.h"
#include "sim_harness.h"
#include "sim_observers.h"

// Run one AI-vs-AI half-inning at a fixed seed and return the rolling MatchSession
// fingerprint. Determinism means two such runs are byte-identical every frame.
static unsigned long long run_once(unsigned int seed)
{
    GameSetup setup;
    sim_make_normal_setup(&setup, 0, 1, CONTROL_AI, CONTROL_AI);

    SimGame* g = sim_create(&setup, seed);

    ChecksumObserver cs;
    checksum_observer_init(&cs);
    sim_attach(g, checksum_observer_hook, &cs);

    sim_run_until(g, sim_pred_half_inning_ended, 60000L);

    unsigned long long h = cs.hash;
    long frames = g->frame;
    sim_destroy(g);

    // Stash frame count alongside hash via the printf for diagnostics.
    printf("    seed=%u frames=%ld hash=%016llx\n", seed, frames, h);
    return h;
}

// The recorded behaviour baseline for the AI-vs-AI half-inning at seed 0x1234ABCD. This is a
// FINGERPRINT OF BEHAVIOUR, not of the struct layout: the checksum observer folds curated
// game-meaningful fields (positions, base ids, player states, ball, the engine seed, the legal
// counters), so adding or removing an unrelated field cannot move it — only the game playing out
// differently can.
//
// What it is for: a slice that claims to be behaviour-preserving can say so as a fact instead of a
// hope. A moved hash is then a leak to investigate, never something to re-baseline over. When a
// slice changes timing or the RNG on purpose, this constant is re-recorded DELIBERATELY, in that
// slice's own commit, alongside the argument for why the new behaviour is the intended one.
// Re-recorded 2026-08-27 by the fielder-movement slice, deliberately. The catching AI stopped
// puppeteering arrow keys and now declares a destination, so every fielder path in the game is
// walked by the engine instead of quantised to eight directions and re-aimed every tenth frame.
// Timing moves everywhere as a result; the argument that the new behaviour is the intended one is
// the fielding bands in test_ai_offense_breakdown, which were recorded on the OLD behaviour before
// any of this landed and all fifteen of which hold — with the defence measurably better
// (mean recovery 102.4 -> 101.1 frames, worst case 800 -> 558).
// Moved again within the same slice, deliberately, when the destination became bound to the fielder
// it was declared for: control passing mid-walk no longer hands the newcomer a point that meant
// something for somebody standing somewhere else. One frame of standing still instead of one frame
// walking the wrong way — and the fielding bands say the exactness pays for itself twice over
// (mean recovery 101.1 -> 99.0 frames, worst case 558 -> 324).
// The batter-selection slice HELD this hash and then moved it, and both halves are worth reading.
//
// Its first step deleted the AI's click-simulation walk through the batter offer, so a selection
// that wants a candidate other than the first is made in one frame instead of two per cycle step.
// Two of the three seeds this file runs moved; this one did not, because in its single hashed
// half-inning the AI wants the FIRST candidate all three times it is asked — and the old walk also
// selected immediately in that case, having no cycle step to spend. Measured, not assumed, and worth
// keeping: a green hash across a producer-side rewrite is evidence about the seed's coverage before
// it is evidence about the change.
//
// Re-recorded here by the slice's second step, deliberately. The batter's aim stopped being a held
// key and became a declared ANGLE that an engine behaviour walks the body to — the batting-side
// mirror of the fielder's destination. Timing moves because arrival is now exact: the key-driven
// version added a step only while the result stayed strictly inside the arc, so it stopped a step
// short of the end, and the AI, holding its key toward a target, overshot it by up to a full step.
// Both errors are doubled by the launch heading (theta = -batter_angle*2), so this is aim.
//
// The argument that the new behaviour is the intended one is the twenty bands in
// test_ai_offense_breakdown, recorded before any of this landed and all twenty holding — including
// the batting-selection bands added for this slice and the direction fan, which is what an aim
// change would break first. One further piece of evidence the bands do not assert: the AI-vs-AI net
// scored a RUN, which it has never done in this configuration. One run over 24 seeds is not a
// balance claim; it is a hint that being exact paid on this side of the field too, exactly as it did
// on the fielding side.
// Re-recorded again by the SWING slice, deliberately, and this one could not have been held. Three
// things move the fingerprint at once, and each is the point of the slice rather than a side effect:
//
//   1. The checksum's own field list changed. It used to fold the shared meter's counter; there is no
//      meter, so it folds the declared power and elevation instead. A fingerprint that tracks what a
//      producer DECLARED rather than what a meter had reached is the better instrument, and it cannot
//      agree with the old one by construction.
//   2. The AI's swing is no longer timed. It declared by releasing at a meter threshold, one lock
//      cycle late; it now declares two values the frame the ball is up. Reproducing the old timing
//      exactly would mean reproducing the lock cycle the slice exists to delete.
//   3. Power and elevation are independent now. The old elevation was read off a meter whose scale
//      moved with power, so the two were accidentally correlated — which is visible in the bands as
//      out-of-bounds falling from 12% to 4%, the extreme high-power-high-loft combinations having
//      stopped happening.
//
// The argument that the new behaviour is the intended one is the bands in test_ai_offense_breakdown,
// including five recorded on UNCHANGED code before this slice moved anything. The one that matters
// most: `swing elevation |V| mean` was 1.91 of a limit of 5 before and is 1.85 after — the batter
// meets the ball as centrally as it ever did, having got there by declaring a number instead of by
// timing a keypress. `whiffs caused by timing` is still zero, and the declaration margin the sim
// leaves before contact went from a mean of 35 frames (minimum 2) to a mean of 86 (minimum 73).
#define SIM_BEHAVIOUR_BASELINE_HASH 0xceac9839ffbf060cULL

int test_ai_vs_ai_determinism(void)
{
    unsigned long long a = run_once(0x1234ABCDu);
    unsigned long long b = run_once(0x1234ABCDu);
    ASSERT(a == b, "same-seed AI-vs-AI runs diverged (nondeterminism)");
    return TEST_PASSED;
}

// The necessary complement to the test above. "Same seed → same game" is satisfied just as well
// by a game with no randomness left in it at all: if a seed were zeroed on every reset, or a
// stream were never actually advanced, the determinism test would still pass while the game
// quietly played the identical inning forever.
//
// This matters directly for the engine/AI stream split: both streams are now seeded once at
// match start and owned by different structs, so a reset recipe clearing the wrong field is a
// real and silent hazard. Requiring different seeds to produce different games pins it.
int test_different_seeds_produce_different_games(void)
{
    unsigned long long a = run_once(0x0000AAAAu);
    unsigned long long b = run_once(0xBBBB0000u);
    ASSERT(a != b, "two different seeds produced an identical game — randomness is not reaching the sim");
    return TEST_PASSED;
}

// Determinism (above) proves the machine repeats ITSELF; this proves the machine still plays the
// SAME GAME it played before. The two are independent: a refactor that quietly changes when a
// fielder starts moving keeps every determinism property intact and still fails here.
//
// If this goes red, the question is never "what is the new number" — it is "what did I change about
// the game". The tests/sim box-score and offense-breakdown tests are the tools for answering it.
int test_sim_hash_matches_recorded_baseline(void)
{
    unsigned long long h = run_once(0x1234ABCDu);
    ASSERT(
        h == SIM_BEHAVIOUR_BASELINE_HASH,
        "the AI-vs-AI half-inning no longer plays out the same game (see the baseline constant above)"
    );
    return TEST_PASSED;
}
