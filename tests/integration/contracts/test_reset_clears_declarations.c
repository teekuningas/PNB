#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_contracts.h"
#include "common_logic.h"
#include "globals.h"

/**
 * CONTRACT: a reset leaves no declared intent behind.
 *
 * The reset recipes put the physical world back to a known state between plays. A pitch or throw
 * declaration that survives one is a producer's intent from a play that no longer exists — and the
 * engine would act on it, beginning a windup nobody asked for.
 *
 * This test exists because the channel slice broke exactly that and nothing noticed. The declarations
 * used to live inside ActionFlags, which the reset cleared as one struct; moving them to
 * PendingActionState left the reset clearing only the sentinel field it named explicitly. All five
 * tiers stayed green, because in AI-vs-AI play the engine happens to have cleared both declarations
 * before every reset — a latent bug, invisible to a determinism hash by construction.
 *
 * It asserts the CLAIM ("nothing declared survives a reset"), not the mechanism, which is why it kept its
 * meaning when the declarations merged into the catching action they describe.
 */
int test_reset_clears_declared_intent(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* match = ctx->state->match;

    // A pitch aimed and mid-windup, as far as the engine is concerned.
    match->pendingActionState.catchingAction = (CatchingAction){
        .kind = CATCHING_ACTION_PITCHING, .timer = 30, .as.pitch = {.power = 0.75f, .aimed = 1, .direction = -0.4f}
    };

    initialize_action_info(match);

    ASSERT_EQ(CATCHING_ACTION_NONE, (int)match->pendingActionState.catchingAction.kind, "no pitch survives a reset");
    ASSERT_EQ(0, match->pendingActionState.catchingAction.timer, "no windup clock survives a reset");

    // And a throw gathering, with its power in.
    match->pendingActionState.catchingAction = (CatchingAction){
        .kind = CATCHING_ACTION_THROWING, .timer = 12, .as.throw = {.target = BASE_SECOND, .powered = 1, .power = 1.0f}
    };

    initialize_action_info(match);

    ASSERT_EQ(CATCHING_ACTION_NONE, (int)match->pendingActionState.catchingAction.kind, "no throw survives a reset");
    ASSERT_EQ(0, match->pendingActionState.catchingAction.as.throw.powered, "no declared throw power survives");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
