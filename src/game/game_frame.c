/*
    game_frame.c — The per-frame game pipeline.

    Each frame executes 7 stages in strict order:
      1. Input           (action_invocations)
      2. Physics & Logic (action_implementation + game_manipulation)
      3. Referee         (update_referee)         — WRITES: RefereeState, HalfInningState, BetweenPitchState,
   PlayerCounters, Scoreboard
      4. Consolidation   (consolidation_update)   — READS referee, scoreboard, bps, his (const),
   WRITES: PlayerInfo, FlowControl, pRAI, GameFlowState
      5. Referee Finalize(referee_finalize)        — WRITES: RefereeState (RESETTING→NONE only)
      6. Snapshot        (debug)
      7. Cleanup         (clear_frame_events)
*/

#include "globals.h"
#include "ball.h"
#include "player.h"
#include "action_implementation.h"
#include "action_invocations.h"
#include "game_consolidation.h"
#include "game_manipulation.h"

#include "game_frame.h"
#include "common_logic.h"
#include "game_setup.h"
#include "../renderer/player_renderer.h" // Include player_renderer.h
#include "state_validator.h"
#include "referee.h"
#include "rules_pure/player_utils.h"

int init_game_frame(StateInfo* stateInfo, ResourceManager* rm)
{
    int result;

    // The initPlayer is now handled by initPlayerRenderer, which is called by initPlayer in player.c
    // No direct call to initPlayerRenderer here.
    result = initPlayer(stateInfo, rm);
    if (result != 0) {
        printf("Could not init player. Exiting.");
        return -1;
    }

    result = initBall(rm);
    if (result != 0) {
        printf("Could not init ball. Exiting.");
        return -1;
    }

    init_action_implementation(stateInfo);
    init_action_invocations(stateInfo);

    // Consolidated Init (Game Flow + Reset Logic)
    consolidation_init(&(stateInfo->match->gameFlowState));

    init_game_manipulation(&(stateInfo->match->gameFlowState));

    return 0;
}

void update_game_frame(StateInfo* stateInfo, MenuInfo* menuInfo, unsigned int* rng_seed)
{
    if (stateInfo->match->flowControl.pause == 0) {
        // 1. Inputs
        action_invocations(stateInfo);

        // 2. Physics & Logic
        action_implementation(stateInfo, rng_seed);
        game_manipulation(stateInfo);

        // 3. Referee (Legal State Authority)
        // Runs AFTER physics to ensure legal state matches physical events
        MatchSession* game = stateInfo->match;
        update_referee(
            stateInfo, &game->referee, &game->halfInningState, &game->betweenPitchState, &game->playerCounters,
            &stateInfo->match->scoreboard, &game->homeRunContestState
        );

        // 4. Consolidation (Reaction Phase)
        // - Updates Game Flow (innings, user prompts)
        // - Handles Physical Resets (Foul Play)
        // - Enforces Legal State (Outs, Scoring)
        // Referee-owned state (bps, his) passed as const — consolidation reads but never writes.
        ConsolidationOutput consolidation_output;
        consolidation_update(
            game, stateInfo->fieldPositions, stateInfo->teamData, stateInfo->gameConclusion, &game->referee,
            &game->betweenPitchState, &game->halfInningState, &game->scoreboard, menuInfo, rng_seed,
            &consolidation_output
        );

        // Handle screen transition requests from consolidation
        if (consolidation_output.request_screen_change) {
            stateInfo->screen = consolidation_output.target_screen;
            stateInfo->changeScreen = 1;
            stateInfo->updated = 0;
        }

        // 5. Referee Finalize (Post-Consolidation)
        // Handles RESETTING→NONE transitions after consolidation has performed
        // physical resets. Scans the new physical world and establishes legal tracking.
        referee_finalize(stateInfo, &game->referee, &game->betweenPitchState);

        // 6. Capture snapshot after all updates when pitch is released
        if (stateInfo->match->gameEvents.pitchReleased) {
            StateValidator_CaptureSnapshot(stateInfo, "PITCH_START");
        }

        // Validate state consistency (Debug only)
        if (!StateValidator_Check(stateInfo)) {
            StateValidator_Dump(stateInfo, "State Consistency Check Failed");
            stateInfo->match->flowControl.pause = 1;
        }

        // 7. Clear transient events for the next frame
        clear_frame_events(&stateInfo->match->gameEvents);
    }
}
void draw_game_frame(const StateInfo* stateInfo, double alpha, ResourceManager* rm)
{
    // players and ball are the building blocks of all the action on the screen.
    if (stateInfo->match->flowControl.pause == 0) {
#ifndef NO_RENDER
        drawPlayerRenderer(stateInfo, stateInfo->match->playerInfo, alpha, rm);
        drawBall(&(stateInfo->match->ballInfo), alpha, rm);
#endif
    }
}
int clean_game_frame(StateInfo* stateInfo)
{
    int result;
    result = cleanBall();
    if (result != 0) {
        printf("Could not clean ball properly.\n");
        return -1;
    }
#ifndef NO_RENDER
    result = cleanPlayerRenderer();
    if (result != 0) {
        printf("Could not clean player properly.\n");
        return -1;
    }
#endif
    return 0;
}
