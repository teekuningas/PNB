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
 * It asserts the CLAIM ("nothing declared survives a reset"), not the mechanism, so it keeps its
 * meaning when the declarations merge into the actualizations.
 */
int test_reset_clears_declared_intent(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* match = ctx->state->match;

    // A pitch aimed and a throw committed — both mid-flight as far as the engine is concerned.
    match->pendingActionState.pitchDeclaration.phase = PITCH_DECL_AIMED;
    match->pendingActionState.pitchDeclaration.power = 0.75f;
    match->pendingActionState.pitchDeclaration.direction = -0.4f;
    match->pendingActionState.throwDeclaration.phase = THROW_DECL_COMMITTED;
    match->pendingActionState.throwDeclaration.target = BASE_SECOND;
    match->pendingActionState.throwDeclaration.power = 1.0f;

    initialize_action_info(match);

    ASSERT_EQ(PITCH_DECL_IDLE, (int)match->pendingActionState.pitchDeclaration.phase, "the pitch declaration is idle");
    ASSERT_FLOAT_EQ(0.0f, match->pendingActionState.pitchDeclaration.power, 1e-6f, "no declared pitch power survives");
    ASSERT_FLOAT_EQ(
        0.0f, match->pendingActionState.pitchDeclaration.direction, 1e-6f, "no declared pitch direction survives"
    );
    ASSERT_EQ(THROW_DECL_IDLE, (int)match->pendingActionState.throwDeclaration.phase, "the throw declaration is idle");
    ASSERT_FLOAT_EQ(0.0f, match->pendingActionState.throwDeclaration.power, 1e-6f, "no declared throw power survives");
    ASSERT_EQ(
        BASE_NONE, (int)match->pendingActionState.throwDeclaration.target,
        "the throw target returns to its absent sentinel, which is NOT zero"
    );

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
