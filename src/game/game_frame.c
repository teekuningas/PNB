/*
    game_frame.c — The per-frame game pipeline.

    Each frame executes 7 stages in strict order:
      1. Control         (action_invocations + ai_update) — EVERY producer, reading the same settled world,
                         writing nothing but value messages into its own team's intent channel
      2. Physics & Logic (execute_actions: INGEST the channels, then actualize; + update_meters + game_manipulation)
      3. Referee         (update_referee)         — WRITES: RefereeState, HalfInningState, BetweenPitchState,
   Scoreboard
      4. Consolidation   (consolidation_update)   — READS referee, scoreboard, bps, his (const),
   WRITES: PlayerInfo, FlowControl, pRAI, GameFlowState
      5. Referee Finalize(referee_finalize)        — WRITES: RefereeState (RESETTING→NONE only)
      6. Snapshot        (debug)
      7. Cleanup         (clear_frame_events)
*/

#include "globals.h"
#include "ball.h"
#include "player.h"
#include "execute_actions.h"
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

    // The init_player is now handled by init_player_renderer, which is called by init_player in player.c
    // No direct call to init_player_renderer here.
    result = init_player(stateInfo, rm);
    if (result != 0) {
        printf("Could not init player. Exiting.");
        return -1;
    }

    result = init_ball(rm);
    if (result != 0) {
        printf("Could not init ball. Exiting.");
        return -1;
    }

    init_execute_actions(stateInfo->match, stateInfo->clientInput);

    // Consolidated Init (Game Flow + Reset Logic)
    consolidation_init(&(stateInfo->match->gameFlowState));

    init_game_manipulation(&(stateInfo->match->gameFlowState));

    return 0;
}

void update_game_frame(StateInfo* stateInfo, MenuInfo* menuInfo)
{
    if (stateInfo->match->flowControl.pause == 0) {
        MatchSession* game = stateInfo->match;
        GameRulesState* rules = stateInfo->rules;

        // 1. CONTROL — every producer, one stage, one settled world.
        // Both controllers run here at the frame top and read the SAME end-of-previous-tick world:
        // the human's keys through action_invocations, the AI through ai_update. Their intents are
        // then consumed by the SAME frame's execution, so neither producer is privileged and the AI
        // has no 1-frame input buffer (that buffer was an accident of call placement, not a design).
        // This is controller symmetry: the engine cannot tell the producers apart by when they ran.
        //
        // Order within the stage is free, and deliberately so: every check* in action_invocations
        // returns early on CONTROL_AI and ai_update dispatches per team on team_is_ai(), so the two
        // producers write disjoint per-team channels and cannot see each other's writes. Human first
        // only because that is the order that held before this stage existed.
        //
        // Frame-top placement also makes one-frame transients structurally invisible: gameEvents is
        // drained by the tick that produced it (stage 7), so a controller here can only ever read
        // DURABLE world state. tests/sim/test_ai_ignores_frame_events.c holds that mechanically.
        action_invocations(
            game, stateInfo->clientInput, stateInfo->keyStates, &rules->scoreboard, &rules->referee,
            &stateInfo->channels
        );
        ai_update(game, rules, stateInfo->fieldPositions, stateInfo->aiController, &stateInfo->channels);

        // 2. Physics & Logic
        // StateInfo is destructured here (the assembly point): each stage receives exactly the
        // worlds it touches — mutable physical (MatchSession), client-local input read-only
        // (const ClientInputState: stage 1 writes it, execution only reads), read-only legal
        // (GameRulesState), geometry, and its one output — a signature is its complete edge list.
        execute_actions(game, rules, stateInfo->fieldPositions, &stateInfo->channels, &stateInfo->playSoundEffect);
        update_meters(game, stateInfo->clientInput);
        game_manipulation(game, stateInfo->fieldPositions, &rules->referee, &stateInfo->playSoundEffect);

        // 3. Referee (Legal State Authority)
        // Runs AFTER physics to ensure legal state matches physical events
        update_referee(
            stateInfo, &rules->referee, &rules->halfInningState, &rules->betweenPitchState, &rules->scoreboard,
            &rules->homeRunContestState
        );

        // 4. Consolidation (Reaction Phase)
        // - Updates Game Flow (innings, user prompts)
        // - Handles Physical Resets (Foul Play)
        // - Enforces Legal State (Outs, Scoring)
        // Referee-owned state passed via GameRulesState — consolidation reads but minimally writes.
        ConsolidationOutput consolidation_output;
        consolidation_update(
            game, stateInfo->fieldPositions, stateInfo->teamData, stateInfo->gameConclusion, rules, menuInfo,
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
        referee_finalize(stateInfo, &rules->referee, &rules->betweenPitchState);

        // 6. Capture snapshot after all updates when pitch is released
        if (stateInfo->match->gameEvents.pitchReleased) {
            state_validator_capture_snapshot(stateInfo, "PITCH_START");
        }

        // Validate state consistency (Debug only)
        if (!state_validator_check(stateInfo)) {
            state_validator_dump(stateInfo, "State Consistency Check Failed");
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
        draw_player_renderer(stateInfo, stateInfo->match->playerInfo, alpha, rm);
        draw_ball(&(stateInfo->match->ballInfo), alpha, rm);
#endif
    }
}
int clean_game_frame(StateInfo* stateInfo)
{
    int result;
    result = clean_ball();
    if (result != 0) {
        printf("Could not clean ball properly.\n");
        return -1;
    }
#ifndef NO_RENDER
    result = clean_player_renderer();
    if (result != 0) {
        printf("Could not clean player properly.\n");
        return -1;
    }
#endif
    return 0;
}
