#include "test_helpers.h"
#include "sim_harness.h"
#include "sim_observers.h"
#include "game_frame.h"

#include <string.h>

// The tick equation says the whole game is
//
//     World(T) = tick( World(T-1), messages_batting(T), messages_catching(T) )
//
// and that nothing outside the World and the messages influences a future frame. Replay,
// rollback and networking are all corollaries of that one property, so it deserves an
// executable proof rather than a claim in a document.
//
// This test is that proof at the strength the code currently supports: capture the World and
// the controller memory, tick forward, restore the capture, tick forward again, and require
// the two futures to be identical.
//
// It could not be written before the engine's RNG seed moved into MatchSession. The seed was a
// main.c local, so a captured MatchSession was missing the one field that decides every future
// engine draw — restoring it and ticking on would silently produce a different world. Note that
// state_validator.c already memcpy's MatchSession as a "snapshot"; that mechanism was quietly
// incomplete for the same reason.
//
// The controller memory (AIControllerState) is captured alongside the World, not inside it, and
// that is the correct shape: a controller's memory is private and outside the World by design
// (controller memory is controller-private). Once the intent channel exists — the channel slice — the stronger form of
// this test becomes possible — capture the World alone and replay a recorded message log, with no controller running at
// all.

// The capture itself (SimWorldCapture + sim_capture_world/sim_restore_world) lives in the
// harness, because the law-1 probe in test_ai_ignores_frame_events.c needs exactly the same
// thing: everything that can influence a future frame, put back byte for byte.

// Fold the same curated fields the checksum observer uses, so a divergence anywhere in the
// simulated world shows up as a different number.
static unsigned long long tick_and_hash(SimGame* g, int frames)
{
    ChecksumObserver cs;
    checksum_observer_init(&cs);
    for (int i = 0; i < frames; i++) {
        update_game_frame(g->state, &g->menu);
        g->frame++;
        checksum_observer_hook(g, &cs);
    }
    return cs.hash;
}

int test_world_snapshot_retick_is_identical(void)
{
    GameSetup setup;
    sim_make_normal_setup(&setup, 0, 1, CONTROL_AI, CONTROL_AI);
    SimGame* g = sim_create(&setup, 0x5EED0001u);

    // Run a while first so the capture lands mid-play (runners moving, a pitch in flight,
    // counters part-way) rather than on the tidy opening frame.
    for (int i = 0; i < 400; i++) {
        update_game_frame(g->state, &g->menu);
        g->frame++;
    }

    SimWorldCapture cap;
    sim_capture_world(&cap, g);

    unsigned long long first = tick_and_hash(g, 600);

    sim_restore_world(&cap, g);
    unsigned long long second = tick_and_hash(g, 600);

    sim_destroy(g);

    ASSERT(
        first == second, "restoring the captured World + controller memory did not reproduce the same future — "
                         "something that influences the next frame still lives outside the capture"
    );
    return TEST_PASSED;
}

// The engine seed is World state, so a capture must carry it. This pins the specific failure the
// move fixes: rewind the world but leave the engine seed where the first run left it, and the
// futures must diverge.
//
// The window has to be chosen carefully, and the reason is worth stating. The engine draws from
// its stream in exactly one place today — initialize_spatial_player_information, which arranges
// the batting side around the home circle — and that runs only inside a reset recipe (half-inning
// end, foul play, next HR pair). Across a stretch of ordinary play the engine draws nothing at
// all, so a stale seed would be undetectable and this test would pass for the wrong reason. It
// therefore ticks until the seed has demonstrably been consumed, and says so if it never is.
int test_retick_diverges_when_engine_seed_is_not_restored(void)
{
    GameSetup setup;
    sim_make_normal_setup(&setup, 0, 1, CONTROL_AI, CONTROL_AI);
    SimGame* g = sim_create(&setup, 0x5EED0002u);

    for (int i = 0; i < 400; i++) {
        update_game_frame(g->state, &g->menu);
        g->frame++;
    }

    SimWorldCapture cap;
    sim_capture_world(&cap, g);

    // Run until an engine draw has actually happened (the seed moved), so the comparison below
    // is testing something. Record how far that took, and replay exactly that far.
    ChecksumObserver cs;
    checksum_observer_init(&cs);
    int frames = 0;
    const int budget = 40000;
    while (frames < budget && g->state->match->rngSeed == cap.match.rngSeed) {
        update_game_frame(g->state, &g->menu);
        g->frame++;
        checksum_observer_hook(g, &cs);
        frames++;
    }
    ASSERT(
        g->state->match->rngSeed != cap.match.rngSeed,
        "the engine never drew from its stream — no reset recipe ran, so this test proves nothing"
    );

    // Carry on a little past the draw so its effect is folded into the hash.
    for (int i = 0; i < 120; i++) {
        update_game_frame(g->state, &g->menu);
        g->frame++;
        checksum_observer_hook(g, &cs);
        frames++;
    }
    unsigned long long first = cs.hash;
    unsigned int seed_after_first = g->state->match->rngSeed;

    sim_restore_world(&cap, g);
    g->state->match->rngSeed = seed_after_first; // the one field deliberately NOT rewound
    unsigned long long second = tick_and_hash(g, frames);

    sim_destroy(g);

    ASSERT(
        first != second, "restoring everything EXCEPT the engine seed still reproduced the same future — the seed "
                         "is not actually feeding the engine's randomness"
    );
    return TEST_PASSED;
}
