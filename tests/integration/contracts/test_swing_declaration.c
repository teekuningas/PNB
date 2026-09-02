#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_contracts.h"
#include "execute_actions.h"
#include "actions/batting_system.h"
#include "actions_pure/swing_geometry.h"
#include "actions_pure/pitching_physics.h"
#include "vector_math.h"
#include <math.h>

/**
 * CONTRACT: the swing slice — a swing is two DECLARED VALUES the engine consumes at a frame it
 * decides from the ball, and never a phase a producer drives forward.
 *
 * These knight the migration. Each one is written so that it fails if the claim in its name stops
 * being true and for no other reason, and each was ablated against the code it defends.
 */

// Construct a live pitch: a batter standing ready, then a real ball on its way over the plate. The
// contract tier's power is exactly this — a state a real producer would take many seconds to reach,
// built in two frames so the claim under test is the only variable.
static void stand_a_batter_ready(ScenarioContext* ctx)
{
    MatchSession* m = ctx->state->match;
    setup_pitcher_ready(ctx);
    setup_batter_at_home(ctx, 0);
    initialize_referee_from_physical_state(ctx);

    m->pRAI.batter_ready = 1;
    m->pRAI.init_batter = 1;
    m->pRAI.pitch_state = PITCH_STAGE_NONE;
    m->pendingActionState.run_bat_flag = 0;
    simulate_frames(ctx, 1); // a ready batter with no pitch in the air is what starts the at-bat
}

static void put_a_pitch_in_the_air(ScenarioContext* ctx)
{
    MatchSession* m = ctx->state->match;
    Vector3D v = pitch_velocity_from_aim(0.5f, 0.0f); // a middling toss, dead over the plate

    vec3_set_xyz(&m->ballInfo.velocity, v.x, v.y, v.z);
    // Released at the pitcher's hand height, not at ground level. The engine solves the contact
    // frame for s = 0 deliberately — "the landing point is actually in the air" — so a ball started
    // ON the ground reaches it three frames before the bat does and the play is over first.
    vec3_set_xyz(&m->ballInfo.location, 0.0f, 1.5f, 0.0f);
    m->ballInfo.visible = 1;
    m->ballInfo.moving = 1;
    m->ballInfo.onGround = 0;
    m->ballInfo.currentFlightHasHitGround = 0;
    m->pII.hasBallIndex = -1;
    m->pRAI.pitch_state = PITCH_STAGE_AIRBORNE;
    m->pRAI.batter_can_advance = 1;
    simulate_frames(ctx, 1); // the engine solves the contact frame from the ball on this tick
}

static int reach_a_live_pitch(ScenarioContext* ctx)
{
    stand_a_batter_ready(ctx);
    put_a_pitch_in_the_air(ctx);
    return swing_may_be_declared(ctx->state->match, &ctx->state->rules->betweenPitchState);
}

static void tick(ScenarioContext* ctx)
{
    simulate_frames(ctx, 1);
}

static void declare_swing(ScenarioContext* ctx, float power, float vertical)
{
    intent_push(
        &ctx->state->channels.batting, (IntentMessage){.kind = INTENT_SWING_POWER, .as.swing_power = {.power = power}}
    );
    intent_push(
        &ctx->state->channels.batting,
        (IntentMessage){.kind = INTENT_SWING_VERTICAL, .as.swing_vertical = {.vertical = vertical}}
    );
}

// Run past the contact frame, so whatever the swing was going to do has happened.
static void run_to_contact(ScenarioContext* ctx)
{
    MatchSession* match = ctx->state->match;
    for (int i = 0; i < 400; i++) {
        if (match->pendingActionState.swing.contactFrame >= 0 &&
            match->pendingActionState.batting_frame_count > match->pendingActionState.swing.contactFrame + 2) {
            return;
        }
        tick(ctx);
    }
}

// 1. The declared values ARE the swing: the launch elevation at contact is the pure function of the
//    declared vertical, and a swing declared at the sweet spot meets the ball dead centre.
int test_swing_declared_values_drive_the_contact(void)
{
    ScenarioContext* ctx = create_scenario();
    ASSERT(reach_a_live_pitch(ctx), "never reached a live pitch to swing at");
    MatchSession* match = ctx->state->match;

    declare_swing(ctx, 0.8f, SWING_VERTICAL_FOCAL);
    tick(ctx);

    ASSERT_EQ(1, match->pendingActionState.swing.powerActive, "the gate must store a declared power");
    ASSERT_EQ(1, match->pendingActionState.swing.verticalActive, "the gate must store a declared elevation");
    ASSERT(fabsf(0.8f - match->pendingActionState.swing.power) < 0.0001f, "the stored power is the declared one");

    // A vertical at the sweet spot is a level hit whatever the ball is doing.
    float level = swing_vertical_angle(SWING_VERTICAL_FOCAL, match->ballInfo.velocity.y);
    ASSERT(fabsf(level) < 0.001f, "the sweet spot must produce a level swing at any ball speed");

    run_to_contact(ctx);
    ASSERT_EQ(
        (int)BAT_OUTCOME_HIT, (int)ctx->state->rules->betweenPitchState.batOutcome,
        "a swing declared at the sweet spot must meet the ball"
    );
    cleanup_scenario(ctx);

    // ...and the other end of the claim, which is the half that makes it a claim at all. A hit at
    // the sweet spot alone is satisfied by an elevation law that returns zero no matter what it is
    // told, so the pair is asserted: a vertical declared far from the sweet spot must MISS. Found by
    // ablation — the first version of this test passed with the law stubbed out.
    ctx = create_scenario();
    ASSERT(reach_a_live_pitch(ctx), "never reached a live pitch to swing at");
    match = ctx->state->match;

    declare_swing(ctx, 0.8f, 0.0f); // the bottom of the sweep: swung well over the ball
    tick(ctx);
    run_to_contact(ctx);
    ASSERT_EQ(
        (int)BAT_OUTCOME_MISSED, (int)ctx->state->rules->betweenPitchState.batOutcome,
        "a swing declared far from the sweet spot must miss — the declared value is what decides"
    );
    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// 2. Repeat-safety, the property the whole message shape is chosen for: three copies of the same
//    declaration in one frame leave the world exactly where one copy would.
int test_swing_declarations_are_repeat_safe(void)
{
    ScenarioContext* ctx = create_scenario();
    ASSERT(reach_a_live_pitch(ctx), "never reached a live pitch to swing at");
    MatchSession* match = ctx->state->match;

    declare_swing(ctx, 0.6f, 0.4f);
    declare_swing(ctx, 0.6f, 0.4f);
    declare_swing(ctx, 0.6f, 0.4f);
    tick(ctx);
    SwingActualization afterThree = match->pendingActionState.swing;

    // And restating it on later frames changes nothing either.
    for (int i = 0; i < 5; i++) {
        declare_swing(ctx, 0.6f, 0.4f);
        tick(ctx);
    }
    ASSERT(
        fabsf(afterThree.power - match->pendingActionState.swing.power) < 0.0001f &&
            fabsf(afterThree.vertical - match->pendingActionState.swing.vertical) < 0.0001f &&
            afterThree.contactFrame == match->pendingActionState.swing.contactFrame,
        "re-delivering a swing declaration must be a no-op — a value, never an increment"
    );
    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// 3. Silence is a complete answer. A producer that declares nothing has not swung, so there is no
//    miss to record — which is the difference between a ball and a strike.
int test_a_swing_never_declared_is_not_a_miss(void)
{
    ScenarioContext* ctx = create_scenario();
    ASSERT(reach_a_live_pitch(ctx), "never reached a live pitch to swing at");
    MatchSession* match = ctx->state->match;

    run_to_contact(ctx); // declare nothing at all

    ASSERT_EQ(
        (int)BAT_OUTCOME_NONE, (int)ctx->state->rules->betweenPitchState.batOutcome,
        "declaring nothing must not register as a swing and a miss"
    );
    ASSERT_EQ(1, match->pendingActionState.batting_stopped, "the engine must conclude that no swing happened");
    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// 4. The withdrawal — the "väärä!" call. A power already committed, then taken back, and the bat
//    does not come. Worth a ball where swinging and missing would be worth a strike.
int test_a_withdrawn_swing_does_not_reach_the_ball(void)
{
    ScenarioContext* ctx = create_scenario();
    ASSERT(reach_a_live_pitch(ctx), "never reached a live pitch to swing at");
    MatchSession* match = ctx->state->match;

    declare_swing(ctx, 0.9f, SWING_VERTICAL_FOCAL); // a committed, well-timed swing
    tick(ctx);
    ASSERT_EQ(1, match->pendingActionState.swing.powerActive, "the swing must be committed before it is withdrawn");

    intent_push(&ctx->state->channels.batting, (IntentMessage){.kind = INTENT_SWING_PASS});
    tick(ctx);
    ASSERT_EQ(1, match->pendingActionState.batting_stopped, "a withdrawal must stop the swing on the tick it arrives");
    ASSERT_EQ(
        (int)BATTING_MODE_STOP, (int)match->pendingActionState.batting_mode,
        "a withdrawn swing must re-shape the body to spread hands"
    );

    run_to_contact(ctx);
    ASSERT_EQ(
        (int)BAT_OUTCOME_NONE, (int)ctx->state->rules->betweenPitchState.batOutcome,
        "a withdrawn swing must not meet the ball — that is what makes it worth a ball and not a strike"
    );
    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// 5. The gate refuses what has no pitch behind it — the whole of the ordering the deleted phase
//    machine used to carry, and the reason a producer never has to read its declaration back.
int test_a_swing_declared_with_no_pitch_is_refused(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* match = ctx->state->match;
    stand_a_batter_ready(ctx); // ...but no pitch is put in the air

    ASSERT_EQ(
        0, swing_may_be_declared(match, &ctx->state->rules->betweenPitchState),
        "with no pitch on its way there is nothing to swing at"
    );

    declare_swing(ctx, 0.9f, SWING_VERTICAL_FOCAL);
    tick(ctx);

    ASSERT_EQ(0, match->pendingActionState.swing.powerActive, "a refused declaration must leave no trace in the world");
    ASSERT_EQ(
        0, match->pendingActionState.swing.verticalActive, "a refused declaration must leave no trace in the world"
    );
    cleanup_scenario(ctx);
    return TEST_PASSED;
}
