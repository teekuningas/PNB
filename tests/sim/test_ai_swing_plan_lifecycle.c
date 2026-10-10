#include "test_helpers.h"
#include "sim_harness.h"
#include "sim_observers.h"

// The batting controller's per-pitch memory lives exactly as long as the pitch, and the controller
// alone ends it.
//
// The plan (AISwingPlan: the swing, the style it was decided as, and the wrong-pitch judgement) is
// controller memory, outside the World. Nothing in the engine may clear it, so the only thing that
// stops one pitch's plan becoming the next pitch's is the controller noticing, from durable world
// state, that the pitch is over. If that ever stops happening the game still plays — the AI keeps
// declaring a swing every pitch — but it is the SAME swing, and one stale wrong-pitch judgement is
// enough to make it withdraw from every pitch after the first ball. That second failure was measured
// before this test existed: the determinism hash did not see it, the quality bands did.
//
// So this looks at every pitch release of a real AI-vs-AI run and holds two claims there:
//   1. a pitch the AI swings at was planned since the previous release — a fresh plan, not a carried one;
//   2. no wrong-pitch judgement is standing at release. The controller runs at the frame top, before
//      the engine releases the ball, so on the release frame it cannot have judged THIS pitch yet:
//      anything it holds there was carried in from an earlier one.

#define PERIOD_MAX_FRAMES 120000L

typedef struct {
    PitchCycleState prevPitchState;
    int prevDecided;
    int planMadeSinceRelease;

    int releases;
    int swungReleases; // releases with a power declared for them — the pitches the plan is for
    int plansMade;
    int staleReleases; // claim 1 broken
    int carriedJudgements; // claim 2 broken
} PlanLifecycleObserver;

static void plan_lifecycle_hook(const SimGame* g, void* ctx)
{
    PlanLifecycleObserver* o = (PlanLifecycleObserver*)ctx;
    const AISwingPlan* plan = &g->state->aiController->swing;
    const MatchSession* m = g->state->match;

    if (plan->decided && !o->prevDecided) {
        o->plansMade++;
        o->planMadeSinceRelease = 1;
    }

    if (m->pRAI.pitch_state == PITCH_STAGE_AIRBORNE && o->prevPitchState != PITCH_STAGE_AIRBORNE) {
        o->releases++;
        if (m->pendingActionState.swing.powerActive) {
            o->swungReleases++;
            if (!o->planMadeSinceRelease) o->staleReleases++;
        }
        if (plan->wrongPitch) o->carriedJudgements++;
        o->planMadeSinceRelease = 0;
    }

    o->prevDecided = plan->decided;
    o->prevPitchState = m->pRAI.pitch_state;
}

static int three_half_innings_done(const SimGame* g)
{
    return (g->state->rules->scoreboard.inning - g->start_inning) >= 3 || g->state->screen != SCREEN_GAME;
}

int test_ai_swing_plan_lives_one_pitch(void)
{
    const unsigned int seeds[] = {0x12345678u, 0x0000AAAAu, 0xBBBB0000u, 0x5EED0007u, 0xC0FFEE11u, 0x0BADF00Du};
    PlanLifecycleObserver total = {0};

    for (unsigned int s = 0; s < sizeof(seeds) / sizeof(seeds[0]); s++) {
        GameSetup setup;
        sim_make_normal_setup(&setup, 0, 1, CONTROL_AI, CONTROL_AI);
        SimGame* g = sim_create(&setup, seeds[s]);
        ASSERT_NOT_NULL(g, "sim_create returned NULL");

        PlanLifecycleObserver o = {.prevPitchState = PITCH_STAGE_NONE};
        sim_attach(g, plan_lifecycle_hook, &o);
        sim_run_until(g, three_half_innings_done, PERIOD_MAX_FRAMES);
        int failed = g->failed;
        sim_destroy(g);
        ASSERT_EQ(0, failed, "the run itself must stay healthy");

        total.releases += o.releases;
        total.swungReleases += o.swungReleases;
        total.plansMade += o.plansMade;
        total.staleReleases += o.staleReleases;
        total.carriedJudgements += o.carriedJudgements;
    }

    printf(
        "    releases=%d swung=%d plans=%d stale=%d carried=%d\n", total.releases, total.swungReleases, total.plansMade,
        total.staleReleases, total.carriedJudgements
    );

    // A claim about every pitch says nothing unless pitches happened, and swung at.
    ASSERT_TRUE(total.swungReleases >= 100, "the run must contain enough swung-at pitches to mean anything");
    ASSERT_EQ(0, total.staleReleases, "every pitch the AI swings at must be planned afresh for that pitch");
    ASSERT_EQ(0, total.carriedJudgements, "a wrong-pitch judgement must never outlive the pitch it was about");
    return TEST_PASSED;
}
