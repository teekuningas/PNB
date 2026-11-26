#include "test_helpers.h"
#include "tick_outputs.h"
#include <stdio.h>
#include "world.h"
#include "ai/ai_memory.h"
#include "actions/batting_system.h"
#include "intent_channel.h"
#include "sim_harness.h"

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
    int prevPitchInFlight;
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
    const AISwingPlan* plan = &g->state->ai_controller->swing;
    const MatchSession* m = g->state->match;

    if (plan->decided && !o->prevDecided) {
        o->plansMade++;
        o->planMadeSinceRelease = 1;
    }

    if (m->ball.pitch_in_flight && !o->prevPitchInFlight) {
        o->releases++;
        if (m->at_bat.swing.power_active) {
            o->swungReleases++;
            if (!o->planMadeSinceRelease) o->staleReleases++;
        }
        if (plan->wrong_pitch) o->carriedJudgements++;
        o->planMadeSinceRelease = 0;
    }

    o->prevDecided = plan->decided;
    o->prevPitchInFlight = m->ball.pitch_in_flight;
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

        PlanLifecycleObserver o = {.prevPitchInFlight = 0};
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

// When the AI has a runner armed on 3rd base and the opposing pitcher throws a väärä (including a
// tight low pitch landing just outside PLATE_WIDTH / 2), the runner breaks on release, the AI
// recognizes väärä from exact ground-impact kinematics, withdraws the swing, and recalls the runner
// cleanly back onto 3rd base before the pitcher can recover the ball at home — never burning them
// out one step off the base.
int test_ai_recalls_third_base_runner_on_vaara(void)
{
    const float powers[2] = {0.00f, 0.50f};
    const float dirs[2] = {0.065f, 0.250f}; // tight low väärä (~0.81u) and clear medium väärä (~4.1u)

    for (int c = 0; c < 2; c++) {
        GameSetup setup;
        sim_make_normal_setup(&setup, 0, 1, CONTROL_AI, CONTROL_PLAYER_1);
        SimGame* g = sim_create(&setup, 0x12345u + (unsigned int)c);
        ASSERT_NOT_NULL(g, "sim_create returned NULL");

        StateInfo* st = g->state;
        MatchSession* m = st->match;
        GameRulesState* r = st->rules;

        for (int f = 0; f < 600; f++) {
            sim_tick(g);
            if (batter_stands_ready(&m->at_bat) && m->ball.holder == m->fielding_posts.baseman[BASE_HOME]) break;
        }
        ASSERT_TRUE(batter_stands_ready(&m->at_bat), "batter must be standing ready before pitching");

        const int r3 = 2;
        m->players[r3].batting.presence.base_id = BASE_THIRD;
        m->players[r3].batting.presence.state = PLAYER_STATE_ON_BASE;
        m->players[r3].body.location = st->field_positions->third_base_run;
        m->players[r3].body.previous_location = st->field_positions->third_base_run;
        r->referee.batting_players[r3].base_at_pitch_start = BASE_THIRD;
        r->referee.batting_players[r3].current_safety_base = BASE_THIRD;
        r->referee.batting_players[r3].status = PLAYER_STATUS_ACTIVE;
        r->half_inning.strikes = 1; // AI arms the 3rd-base runner to break on pitch release
        r->half_inning.balls = 0;

        for (int f = 0; f < 5; f++)
            sim_tick(g);
        ASSERT_EQ(1, m->runner_commitments.armed[BASE_THIRD], "AI must arm 3rd-base runner on strike 1");

        intent_push(
            &st->channels.catching, (IntentMessage){.kind = INTENT_PITCH_POWER, .as.pitch_power = {.power = powers[c]}}
        );
        intent_push(
            &st->channels.catching, (IntentMessage){.kind = INTENT_PITCH_AIM, .as.pitch_aim = {.direction = dirs[c]}}
        );

        int sawBall = 0, r3LeftBase = 0, r3Returned = 0, pitcherRecoveredHome = 0;
        for (int f = 0; f < 300; f++) {
            if (m->ball.holder == -1 && !m->ball.pitch_in_flight && m->catching.controlled_fielder != -1) {
                intent_push(
                    &st->channels.catching,
                    (IntentMessage){.kind = INTENT_MOVE_TARGET, .as.move_target.point = m->ball.location}
                );
            } else if (m->ball.holder != -1 && sawBall) {
                intent_push(
                    &st->channels.catching,
                    (IntentMessage){.kind = INTENT_MOVE_TARGET, .as.move_target.point = st->field_positions->pitcher}
                );
            }
            sim_tick(g);

            if (st->announcement == EVENT_BALL) sawBall = 1;
            if (m->players[r3].batting.presence.state == PLAYER_STATE_RUNNING) r3LeftBase = 1;
            if (r3LeftBase && m->players[r3].batting.presence.state == PLAYER_STATE_ON_BASE &&
                m->players[r3].batting.presence.base_id == BASE_THIRD) {
                r3Returned = 1;
            }
            if (sawBall && m->ball.holder == m->fielding_posts.baseman[BASE_HOME] &&
                m->players[m->ball.holder].fielding.near_home == 1) {
                pitcherRecoveredHome = 1;
                break;
            }
        }

        int finalOut = (r->referee.batting_players[r3].status == PLAYER_STATUS_OUT);
        PlayerUnitState finalState = m->players[r3].batting.presence.state;
        BaseID finalBase = m->players[r3].batting.presence.base_id;
        BatOutcome outcome = r->between_pitch.bat_outcome;
        sim_destroy(g);

        ASSERT_EQ(1, sawBall, "väärä pitch must be called BALL");
        ASSERT_EQ(BAT_OUTCOME_NONE, outcome, "AI must withdraw its swing on väärä");
        ASSERT_EQ(1, r3LeftBase, "3rd-base runner must actually break on pitch release for the recall to be tested");
        ASSERT_EQ(1, r3Returned, "3rd-base runner must return onto 3rd base after väärä recall");
        ASSERT_EQ(1, pitcherRecoveredHome, "pitcher must recover the ball at home to test §33 burn protection");
        ASSERT_EQ(0, finalOut, "recalled 3rd-base runner must never be burned OUT when the ball returns home");
        ASSERT_EQ(PLAYER_STATE_ON_BASE, finalState, "3rd-base runner must finish settled ON_BASE");
        ASSERT_EQ(BASE_THIRD, finalBase, "3rd-base runner must remain on BASE_THIRD");
    }

    return TEST_PASSED;
}
