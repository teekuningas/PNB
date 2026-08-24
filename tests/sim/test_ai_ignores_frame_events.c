#include "test_helpers.h"
#include "sim_harness.h"
#include "game_frame.h"
#include "execute_actions.h"

#include <string.h>

// Controller symmetry, law 1, made mechanical.
//
// Every controller — human, AI, scripted, net proxy — runs in one CONTROL stage at the top of the
// tick, reading the same settled end-of-previous-tick World (the control-stage slice, landed
// 2026-08-17). A property that placement quietly depends on: GameEvents is a ONE-FRAME struct,
// drained by the tick that produced it, so at the frame top it is structurally always empty. A
// controller that reads a frame event therefore reads zero now and read something else before the
// move — behaviour changing without anything in the diff saying so.
//
// This test was written the session BEFORE that move, precisely because the move re-baselines the
// determinism hash by design and the hash therefore could not be the thing that caught such a read.
// Until then the property rested on a one-time manual read of the AI code (2026-07-07). This test
// replaced the reading with a measurement, and keeps enforcing it now that the stage has moved: at
// many sampled frames of a real AI-vs-AI run,
// run the controller stage twice from the identical world — once with every frame event zeroed,
// once with every frame event set — and require both runs to produce byte-identical controller
// output. If a future controller ever reaches for an event edge, this goes red on the spot.
//
// It also pins the other direction: a controller must not WRITE frame events either. Events are
// produced by engine stages and consumed by the referee; a controller that posts one is speaking
// on the engine's behalf.

#define WARMUP_FRAMES 400
#define SAMPLE_FRAMES 3000
#define SAMPLE_EVERY 25

// Set every frame event. Written out field by field rather than memset so the values are the
// honest 0/1 the producers use, and so the static assert below can hold the test to the struct.
static GameEvents all_events_set(void)
{
    GameEvents e;
    e.catchMade = 1;
    e.playerArrivedAtBase = 1;
    e.pitchReleased = 1;
    e.ballHitByBat = 1;
    e.ballMissedByBat = 1;
    e.ballHitGround = 1;
    e.freeWalkAccepted = 1;
    e.freeWalkRejected = 1;
    e.batterEntered = 1;
    return e;
}

// If GameEvents gains a field, this stops compiling and all_events_set() must be extended. The
// test must never quietly stop covering part of the struct it exists to vary.
_Static_assert(sizeof(GameEvents) == 9 * sizeof(int), "GameEvents changed — extend all_events_set()");

// Everything the controller stage is allowed to touch: the physical world it declares intents
// into, and its own private memory. Captured after the stage runs, so two probes can be compared.
typedef struct {
    MatchSession match;
    AIControllerState aiController;
    IntentChannels channels; // the controller's real output: the messages it declared
    int wrote_an_event; // the controller posted a frame event — a law violation in itself
} ControllerOutput;

// Restore the world, plant `events`, run the controller stage alone, and record what it did.
static void probe_controllers(SimGame* g, const SimWorldCapture* cap, GameEvents events, ControllerOutput* out)
{
    sim_restore_world(cap, g);
    g->state->match->gameEvents = events;

    // Each probe starts from an empty channel, so what it holds afterwards is this probe's
    // declarations and nothing carried over from the previous one.
    g->state->channels = (IntentChannels){0};

    ai_update(g->state->match, g->state->rules, g->state->fieldPositions, g->state->aiController, &g->state->channels);

    memcpy(&out->match, g->state->match, sizeof(MatchSession));
    memcpy(&out->aiController, g->state->aiController, sizeof(AIControllerState));
    out->channels = g->state->channels;

    out->wrote_an_event = memcmp(&out->match.gameEvents, &events, sizeof(GameEvents)) != 0;

    // gameEvents is the input we varied on purpose, so it cannot take part in the comparison.
    // Everything else in MatchSession does.
    memset(&out->match.gameEvents, 0, sizeof(GameEvents));
}

int test_ai_ignores_frame_events(void)
{
    GameSetup setup;
    sim_make_normal_setup(&setup, 0, 1, CONTROL_AI, CONTROL_AI);
    SimGame* g = sim_create(&setup, 0x5EED0003u);
    ASSERT_NOT_NULL(g, "sim_create returned NULL");

    // Start mid-play rather than on the tidy opening frame: runners moving, a pitch in flight,
    // counters part-way — the states where an event edge would be tempting to read.
    for (int i = 0; i < WARMUP_FRAMES; i++) {
        update_game_frame(g->state, &g->menu);
        g->frame++;
    }

    const GameEvents no_events = {0};
    const GameEvents every_event = all_events_set();

    int samples = 0, active_samples = 0, differing = 0, event_writers = 0;
    long first_difference_frame = -1;

    for (int i = 0; i < SAMPLE_FRAMES; i++) {
        if (i % SAMPLE_EVERY == 0) {
            SimWorldCapture cap;
            sim_capture_world(&cap, g);

            ControllerOutput quiet, noisy;
            probe_controllers(g, &cap, no_events, &quiet);
            probe_controllers(g, &cap, every_event, &noisy);

            samples++;
            if (quiet.wrote_an_event || noisy.wrote_an_event) event_writers++;

            // A sample only proves something if the controller actually did something on it.
            // Comparing the world before the stage with the world after tells us that.
            if (memcmp(&cap.match, &quiet.match, sizeof(MatchSession)) != 0 ||
                memcmp(&cap.aiController, &quiet.aiController, sizeof(AIControllerState)) != 0 ||
                quiet.channels.batting.count != 0 || quiet.channels.catching.count != 0) {
                active_samples++;
            }

            // Declared messages are compared as the controller's output, not just its side effects on
            // the world: an event-dependent controller would most naturally reveal itself by declaring
            // a DIFFERENT message, and a comparison blind to the channel would not see it.
            if (memcmp(&quiet.match, &noisy.match, sizeof(MatchSession)) != 0 ||
                memcmp(&quiet.aiController, &noisy.aiController, sizeof(AIControllerState)) != 0 ||
                memcmp(&quiet.channels, &noisy.channels, sizeof(IntentChannels)) != 0) {
                differing++;
                if (first_difference_frame < 0) first_difference_frame = g->frame;
            }

            // Put the world back exactly as it was, so the probes leave no trace on the run — the
            // channel included, or the next real frame would ingest a probe's declarations.
            sim_restore_world(&cap, g);
            g->state->channels = (IntentChannels){0};
        }

        update_game_frame(g->state, &g->menu);
        g->frame++;
    }

    sim_destroy(g);

    printf(
        "\n  law 1: %d samples, %d with the controller actually writing, %d event-dependent\n", samples, active_samples,
        differing
    );

    ASSERT(samples > 0, "no samples were taken — the loop bounds are wrong");
    ASSERT(
        active_samples > 0, "the controller stage never wrote anything at any sampled frame, so identical output "
                            "proves nothing — pick a busier stretch of play"
    );
    ASSERT(
        event_writers == 0, "the controller stage WROTE a frame event. GameEvents is produced by engine stages and "
                            "consumed by the referee; a controller posting one speaks on the engine's behalf"
    );
    if (differing != 0) {
        printf(
            "  first divergence at frame %ld: the controller read a frame event.\n"
            "  Frame-top placement makes GameEvents structurally empty at the CONTROL\n"
            "  stage, so this read can only ever see zero in production — the controller is keyed on\n"
            "  something that is never there. Re-key it on durable world state: a level, not an edge.\n",
            first_difference_frame
        );
        return TEST_FAILED;
    }

    return TEST_PASSED;
}
