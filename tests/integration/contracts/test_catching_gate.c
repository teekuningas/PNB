#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_contracts.h"
#include "execute_actions.h"
#include "actions/pitching_system.h"
#include "actions/throwing_system.h"
#include <string.h>

/**
 * CONTRACT: the INGEST gate owns the pitch's and the throw's permissions.
 *
 * A power begins a pitch and a target begins a throw, so those two messages carry the whole of each
 * action's permission, asked once against the world at the top of the tick and refused with the rule
 * that refuses them. An aim and a throw's power only contribute, and need an action to contribute to —
 * one already running, or one begun in the same tick. And because a throw, a drop and a pitch claim the
 * same hands, at most one of them happens in a tick, decided by the engine's order and never by the
 * order the messages arrived in.
 *
 * None of these refusals happens in the game's own play — both controllers look before they declare
 * (measured over 42 golden runs: only "batter not ready", "pitcher off the plate" and "throw target too
 * close" ever fire). The gate does not get to trust that, so the states are BUILT here.
 */

#define FIELDER 15

static void tick(ScenarioContext* ctx)
{
    execute_actions(
        ctx->state->match, ctx->state->rules, ctx->state->fieldPositions, &ctx->state->channels,
        &ctx->state->playSoundEffect
    );
}

static void say(ScenarioContext* ctx, IntentMessage message)
{
    intent_push(&ctx->state->channels.catching, message);
}

static IntentMessage pitch_power(float power)
{
    return (IntentMessage){.kind = INTENT_PITCH_POWER, .as.pitch_power = {.power = power}};
}

static IntentMessage pitch_aim(float direction)
{
    return (IntentMessage){.kind = INTENT_PITCH_AIM, .as.pitch_aim = {.direction = direction}};
}

static IntentMessage throw_target(BaseID base)
{
    return (IntentMessage){.kind = INTENT_THROW_TARGET, .as.throw_target = {.base = base}};
}

static IntentMessage throw_power(float power)
{
    return (IntentMessage){.kind = INTENT_THROW_POWER, .as.throw_power = {.power = power}};
}

static const IntentMessage DROP = {.kind = INTENT_DROP_BALL};

// A fielder out in the field holding the ball, far enough from every base to throw to any of them.
static void fielder_with_ball(ScenarioContext* ctx)
{
    MatchSession* m = ctx->state->match;
    m->pII.hasBallIndex = FIELDER;
    m->pII.controlIndex = FIELDER;
    m->playerInfo[FIELDER].tPI.location.x = 5.0f;
    m->playerInfo[FIELDER].tPI.location.z = 5.0f;
}

// The World, captured whole, as test_fielder_movement does: MatchSession + GameRulesState is all of it.
typedef struct {
    MatchSession match;
    GameRulesState rules;
} World;

static void capture(const ScenarioContext* ctx, World* w)
{
    memcpy(&w->match, ctx->state->match, sizeof(MatchSession));
    memcpy(&w->rules, ctx->state->rules, sizeof(GameRulesState));
}

static void restore(ScenarioContext* ctx, const World* w)
{
    memcpy(ctx->state->match, &w->match, sizeof(MatchSession));
    memcpy(ctx->state->rules, &w->rules, sizeof(GameRulesState));
}

static int identical(const World* a, const World* b)
{
    return memcmp(&a->match, &b->match, sizeof(MatchSession)) == 0 &&
           memcmp(&a->rules, &b->rules, sizeof(GameRulesState)) == 0;
}

// 1. Every reason a pitch may not begin, each named by its own rule, against a control that may.
int test_pitch_permission_names_each_rule(void)
{
    ScenarioContext* ctx = create_scenario();
    setup_pitcher_ready(ctx);
    MatchSession* m = ctx->state->match;
    World ready;
    capture(ctx, &ready);

    ASSERT_EQ(1, pitch_may_begin(m, 0).allowed, "control: a ready pitcher with a ready batter may pitch");

    m->pendingActionState.catchingAction.kind = CATCHING_ACTION_THROWING;
    ASSERT_EQ(RULE_CATCHING_ACTION_IN_PROGRESS, pitch_may_begin(m, 0).denied_by, "not while a throw gathers");
    restore(ctx, &ready);
    m->pendingActionState.catchingAction.kind = CATCHING_ACTION_PITCHING;
    ASSERT_EQ(RULE_CATCHING_ACTION_IN_PROGRESS, pitch_may_begin(m, 0).denied_by, "not while a pitch winds up");
    restore(ctx, &ready);

    ASSERT_EQ(RULE_PITCH_BATTER_IS_A_RUNNER, pitch_may_begin(m, 3).denied_by, "§18(1): three correct pitches spent");

    m->pII.hasBallIndex = FIELDER;
    ASSERT_EQ(RULE_PITCH_NEEDS_BALL_AT_HOME, pitch_may_begin(m, 0).denied_by, "the pitcher must hold the ball");
    restore(ctx, &ready);

    m->pRAI.pitch_state = PITCH_STAGE_AIRBORNE;
    ASSERT_EQ(RULE_PITCH_PREVIOUS_UNRESOLVED, pitch_may_begin(m, 0).denied_by, "not before the last pitch is judged");
    restore(ctx, &ready);

    m->pRAI.batter_ready = 0;
    ASSERT_EQ(RULE_PITCH_NEEDS_A_READY_BATTER, pitch_may_begin(m, 0).denied_by, "nobody to pitch to");
    restore(ctx, &ready);

    m->flowControl.waitingForFreeWalkDecision = 1;
    ASSERT_EQ(RULE_PITCH_NOT_DURING_FREE_WALK_OFFER, pitch_may_begin(m, 0).denied_by, "§26: answer the walk first");
    restore(ctx, &ready);

    m->playerInfo[12].cTPI.isNearHomeLocation = 0;
    ASSERT_EQ(RULE_PITCHER_OFF_THE_PLATE, pitch_may_begin(m, 0).denied_by, "the pitcher must be at the plate");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// 2. A refused pitch begins nothing, now or later: there is nowhere for it to wait.
int test_a_refused_pitch_begins_nothing(void)
{
    ScenarioContext* ctx = create_scenario();
    setup_pitcher_ready(ctx);
    MatchSession* m = ctx->state->match;
    m->pRAI.batter_ready = 0;

    say(ctx, pitch_power(0.5f));
    say(ctx, pitch_aim(0.0f));
    tick(ctx);
    ASSERT_EQ(CATCHING_ACTION_NONE, (int)m->pendingActionState.catchingAction.kind, "refused: no batter ready");
    ASSERT_EQ(12, m->pII.hasBallIndex, "the ball stays in the pitcher's hand");

    m->pRAI.batter_ready = 1;
    tick(ctx);
    ASSERT_EQ(
        CATCHING_ACTION_NONE, (int)m->pendingActionState.catchingAction.kind,
        "once the batter is ready, nothing refused earlier begins by itself"
    );

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// 3. Every reason a throw may not begin, each named by its own rule, against a control that may.
int test_throw_permission_names_each_rule(void)
{
    ScenarioContext* ctx = create_scenario();
    fielder_with_ball(ctx);
    MatchSession* m = ctx->state->match;
    const FieldPositions* fp = ctx->state->fieldPositions;

    ASSERT_EQ(1, throw_may_begin(m, fp, BASE_FIRST).allowed, "control: a fielder with the ball may throw to first");

    m->pendingActionState.catchingAction.kind = CATCHING_ACTION_THROWING;
    ASSERT_EQ(RULE_CATCHING_ACTION_IN_PROGRESS, throw_may_begin(m, fp, BASE_FIRST).denied_by, "not mid-throw");
    m->pendingActionState.catchingAction.kind = CATCHING_ACTION_NONE;

    m->pII.hasBallIndex = -1;
    ASSERT_EQ(RULE_THROW_NEEDS_BALL_IN_HAND, throw_may_begin(m, fp, BASE_FIRST).denied_by, "nothing to throw");
    m->pII.hasBallIndex = FIELDER;

    m->playerInfo[FIELDER].tPI.location = fp->firstBase;
    ASSERT_EQ(
        RULE_THROW_TARGET_TOO_CLOSE, throw_may_begin(m, fp, BASE_FIRST).denied_by,
        "a fielder standing on first does not throw to first"
    );
    ASSERT_EQ(1, throw_may_begin(m, fp, BASE_SECOND).allowed, "...but may throw from there to second");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// 4. An aim with no windup, and a throw's power with no gather, contribute to nothing — and are not
//    held over for an action that begins later. Each is checked by beginning that action afterwards and
//    finding it bare.
int test_contributions_need_an_action_to_contribute_to(void)
{
    ScenarioContext* ctx = create_scenario();
    setup_pitcher_ready(ctx);
    MatchSession* m = ctx->state->match;

    say(ctx, pitch_aim(0.3f));
    tick(ctx);
    ASSERT_EQ(CATCHING_ACTION_NONE, (int)m->pendingActionState.catchingAction.kind, "an aim alone begins nothing");
    say(ctx, pitch_power(0.5f));
    tick(ctx);
    ASSERT_EQ(CATCHING_ACTION_PITCHING, (int)m->pendingActionState.catchingAction.kind, "control: the power begins it");
    ASSERT_EQ(0, m->pendingActionState.catchingAction.as.pitch.aimed, "the earlier aim was refused, not held");
    cleanup_scenario(ctx);

    ScenarioContext* ctx2 = create_scenario();
    fielder_with_ball(ctx2);
    MatchSession* m2 = ctx2->state->match;
    say(ctx2, throw_power(1.0f));
    tick(ctx2);
    ASSERT_EQ(CATCHING_ACTION_NONE, (int)m2->pendingActionState.catchingAction.kind, "a power alone begins nothing");
    say(ctx2, throw_target(BASE_FIRST));
    tick(ctx2);
    ASSERT_EQ(
        CATCHING_ACTION_THROWING, (int)m2->pendingActionState.catchingAction.kind, "control: the target begins it"
    );
    ASSERT_EQ(0, m2->pendingActionState.catchingAction.as.throw.powered, "the earlier power was refused, not held");

    // And the cross-wise case, where accepting would not merely be wasted: the record is ONE union, so an
    // aim landing on a gathering throw would be written over the throw's own power, and the throw would
    // fly at the pitch's direction.
    say(ctx2, pitch_aim(0.9f));
    tick(ctx2);
    ASSERT_EQ(0, m2->pendingActionState.catchingAction.as.throw.powered, "an aim cannot power a gathering throw");
    ASSERT_EQ(FIELDER, m2->pII.hasBallIndex, "...nor release it");
    cleanup_scenario(ctx2);

    ScenarioContext* ctx3 = create_scenario();
    setup_pitcher_ready(ctx3);
    MatchSession* m3 = ctx3->state->match;
    say(ctx3, pitch_power(0.5f));
    tick(ctx3);
    say(ctx3, throw_power(0.75f));
    tick(ctx3);
    ASSERT_EQ(0, m3->pendingActionState.catchingAction.as.pitch.aimed, "a throw's power cannot aim a windup");
    cleanup_scenario(ctx3);
    return TEST_PASSED;
}

// Two messages that claim the ball in one tick, delivered in both orders from the same world: the two
// worlds that result must be byte-identical, and are handed back for the caller to say which claim won.
static int both_orders_agree(ScenarioContext* ctx, IntentMessage first, IntentMessage second, World* out)
{
    World start, forward;
    capture(ctx, &start);
    say(ctx, first);
    say(ctx, second);
    tick(ctx);
    capture(ctx, &forward);
    restore(ctx, &start);
    say(ctx, second);
    say(ctx, first);
    tick(ctx);
    capture(ctx, out);
    return identical(&forward, out);
}

// 5. A throw, a drop and a pitch claim the same hands, so at most one happens in a tick — throw, then
//    drop, then pitch, the engine's own order — whichever order they arrived in. The throw-versus-drop
//    case used to depend on that order: a drop judged before the throw was ingested went through, and
//    the ball left a hand that was mid-gather.
int test_one_catching_action_per_tick_whatever_the_order(void)
{
    World after;

    ScenarioContext* ctx = create_scenario();
    setup_pitcher_ready(ctx);
    ctx->state->match->pII.controlIndex = 12;
    ASSERT(both_orders_agree(ctx, pitch_power(0.5f), throw_target(BASE_FIRST), &after), "pitch + throw: order-free");
    ASSERT_EQ(CATCHING_ACTION_THROWING, (int)after.match.pendingActionState.catchingAction.kind, "the throw wins");
    cleanup_scenario(ctx);

    ctx = create_scenario();
    setup_pitcher_ready(ctx);
    ctx->state->match->pII.controlIndex = 12;
    ASSERT(both_orders_agree(ctx, pitch_power(0.5f), DROP, &after), "pitch + drop: order-free");
    ASSERT_EQ(-1, after.match.pII.hasBallIndex, "the drop wins: the ball is on its way down");
    ASSERT_EQ(CATCHING_ACTION_NONE, (int)after.match.pendingActionState.catchingAction.kind, "and no windup began");
    cleanup_scenario(ctx);

    ctx = create_scenario();
    fielder_with_ball(ctx);
    ASSERT(both_orders_agree(ctx, DROP, throw_target(BASE_FIRST), &after), "drop + throw: order-free");
    ASSERT_EQ(CATCHING_ACTION_THROWING, (int)after.match.pendingActionState.catchingAction.kind, "the throw wins");
    ASSERT_EQ(FIELDER, after.match.pII.hasBallIndex, "and the gathering thrower still holds the ball");
    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// Run a catching action to its end from `start`, saying `script` on the first tick and, if `restate` is
// set, saying it again on every later tick too. Returns the world after `frames` ticks.
static void play(
    ScenarioContext* ctx, const World* start, const IntentMessage* script, int n, int copies, int restate, int frames,
    World* out
)
{
    restore(ctx, start);
    for (int f = 0; f < frames; f++) {
        if (f == 0 || restate) {
            for (int c = 0; c < copies; c++) {
                for (int i = 0; i < n; i++) {
                    say(ctx, script[i]);
                }
            }
        }
        tick(ctx);
    }
    capture(ctx, out);
}

// 6. The pitch's and the throw's values are repeat-safe: three copies in one frame are one message, and
//    restating a whole action on every frame of it changes nothing. That is the property rollback and a
//    redundant wire need — "the power is P" means the same thing however often it arrives.
int test_pitch_and_throw_values_are_repeat_safe(void)
{
    ScenarioContext* ctx = create_scenario();
    setup_pitcher_ready(ctx);
    World start, once, tripled, restated;
    capture(ctx, &start);
    const IntentMessage pitch[] = {pitch_power(0.4f), pitch_aim(-0.2f)};
    const int frames = pitch_windup_total_frames(0.4f) + 3;
    play(ctx, &start, pitch, 2, 1, 0, frames, &once);
    ASSERT_EQ(-1, once.match.pII.hasBallIndex, "control: the pitch was released");
    play(ctx, &start, pitch, 2, 3, 0, frames, &tripled);
    play(ctx, &start, pitch, 2, 1, 1, frames, &restated);
    ASSERT(identical(&once, &tripled), "three copies of a pitch in one frame are one pitch");
    ASSERT(identical(&once, &restated), "restating a pitch on every frame of its windup changes nothing");
    cleanup_scenario(ctx);

    ctx = create_scenario();
    fielder_with_ball(ctx);
    capture(ctx, &start);
    const IntentMessage throw[] = {throw_target(BASE_SECOND), throw_power(0.6f)};
    const int tframes = throw_windup_total_frames(0.6f) + 3;
    play(ctx, &start, throw, 2, 1, 0, tframes, &once);
    ASSERT_EQ(-1, once.match.pII.hasBallIndex, "control: the throw was released");
    play(ctx, &start, throw, 2, 3, 0, tframes, &tripled);
    play(ctx, &start, throw, 2, 1, 1, tframes, &restated);
    ASSERT(identical(&once, &tripled), "three copies of a throw in one frame are one throw");
    ASSERT(identical(&once, &restated), "restating a throw on every frame of its gather changes nothing");
    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// 7. A stale beginning arriving late — the power or target of an action already under way, as a
//    redundant or reordered wire would deliver it — changes nothing. Under the phased messages this was
//    not so: an old {phase=POWER} landing after {phase=AIMED} un-aimed the pitch, which then went as a
//    valesyöttö.
int test_a_late_beginning_changes_nothing(void)
{
    ScenarioContext* ctx = create_scenario();
    setup_pitcher_ready(ctx);
    World mid, clean, late;
    say(ctx, pitch_power(0.4f));
    say(ctx, pitch_aim(0.1f));
    for (int f = 0; f < 10; f++) {
        tick(ctx);
    }
    capture(ctx, &mid);
    const int rest = pitch_windup_total_frames(0.4f);
    for (int f = 0; f < rest; f++) {
        tick(ctx);
    }
    capture(ctx, &clean);
    ASSERT_EQ(-1, clean.match.pII.hasBallIndex, "control: the aimed pitch was released");

    restore(ctx, &mid);
    say(ctx, pitch_power(0.9f)); // an older, different power, arriving mid-windup
    for (int f = 0; f < rest; f++) {
        tick(ctx);
    }
    capture(ctx, &late);
    ASSERT(identical(&clean, &late), "a late pitch power is refused: the windup runs as declared, aimed");
    cleanup_scenario(ctx);

    ctx = create_scenario();
    fielder_with_ball(ctx);
    say(ctx, throw_target(BASE_SECOND));
    tick(ctx);
    tick(ctx);
    capture(ctx, &mid);
    say(ctx, throw_power(0.5f));
    for (int f = 0; f < 60; f++) {
        tick(ctx);
    }
    capture(ctx, &clean);
    ASSERT_EQ(-1, clean.match.pII.hasBallIndex, "control: the throw was released");

    restore(ctx, &mid);
    say(ctx, throw_target(BASE_THIRD)); // an older, different target, arriving mid-gather
    say(ctx, throw_power(0.5f));
    for (int f = 0; f < 60; f++) {
        tick(ctx);
    }
    capture(ctx, &late);
    ASSERT(identical(&clean, &late), "a late throw target is refused: no redirect once the body has turned");
    cleanup_scenario(ctx);
    return TEST_PASSED;
}
