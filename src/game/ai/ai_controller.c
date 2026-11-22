#include "ai/ai_controller.h"
#include "field.h"
#include "rules_state.h"
#include "intents.h"
#include "world.h"
#include "ai/ai_memory.h"
#include "ai/catching_ai.h"
#include "ai/batting_ai.h"
#include "rules_pure/player_utils.h"

void ai_update(
    const MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions,
    AIControllerState* aiController, IntentChannels* channels
)
{
    int battingTeamIndex = get_batting_team_index(&rules->scoreboard);
    TeamControlMode battingControl = rules->scoreboard.teams[battingTeamIndex].control;
    TeamControlMode catchingControl = rules->scoreboard.teams[(battingTeamIndex + 1) % 2].control;

    // Each controller is handed ONLY its own team's channel — not both. A controller that cannot name
    // the other team's channel cannot write it, whatever it intends.
    if (team_is_ai(catchingControl)) {
        update_catching_ai(match, rules, fieldPositions, aiController, &channels->catching);
    }
    if (team_is_ai(battingControl)) {
        update_batting_ai(match, rules, fieldPositions, aiController, &channels->batting);
    }
}

void init_ai_controller(AIControllerState* aiController, unsigned int rngSeed)
{
    aiController->rng_seed = rngSeed;
    aiController->batter_ready_timer = -1;
    aiController->swing = (AISwingPlan){0};
    aiController->last_declared_move_target = (Vector3D){0};
    aiController->has_declared_move_target = 0;
    aiController->frames_since_move_declared = 0;
    aiController->last_steered_fielder = -1; // nobody steered yet; 0 is a real player
}
