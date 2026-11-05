/*
    game_frame.c — the per-frame game pipeline, the tick.

    Each frame runs its stages in strict order, each named for its role:
      1. CONTROL      action_invocations + ai_update — every producer, reading the same settled world,
                      writing nothing but value messages into its own team's intent channel
      2. INGEST       ingest_intents — the gate: drains the channels, asks every permission, returns the
                      tick's commands
      3. ACTUALIZE    actualize — the engine's own actualization order over those commands
      4. SIMULATE     simulate — the physical step: the ball, the catch, the fielders, the bodies; the
                      ground, the catch and the arrivals are detected inside the steps that make them
      5. ADJUDICATE   update_referee — WRITES the legal world: RefereeState, HalfInningState,
                      BetweenPitchState, Scoreboard, HomeRunContestState
      6. ENFORCE      consolidation_update — READS the legal world (const); WRITES the bodies, the offers
                      to the batting side, the free-walk entitlement, the ball's flight, the at-bat, the
                      home-run camera cue, and performs the reset recipes
      7. SETTLE       referee_finalize — WRITES RefereeState (RESETTING→NONE only), after the resets
      8. VALIDATE     the pitch-start snapshot and the state validator (debug)
      9. CLEAR        clear_frame_events
*/

#include "globals.h"
#include "ball.h"
#include "player.h"
#include "ingest.h"
#include "actualize.h"
#include "action_invocations.h"
#include "ai/ai_controller.h"
#include "game_consolidation.h"
#include "simulate.h"

#include "game_frame.h"
#include "game_setup.h"
#include "../renderer/player_renderer.h" // Include player_renderer.h
#include "state_validator.h"
#include "referee.h"
#include "rules_pure/player_utils.h"

void clear_frame_events(GameEvents* events)
{
    events->catchMade = 0;
    events->playerArrivedAtBase = 0;
    events->pitchReleased = 0;
    events->ballHitByBat = 0;
    events->ballMissedByBat = 0;
    events->ballHitGround = 0;
    events->freeWalkAccepted = 0;
    events->batterEntered = 0;
}

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

    init_actualization(stateInfo->match);
    init_client_input(stateInfo->clientInput);

    stateInfo->match->home_run_camera = (HomeRunCameraCue){.active = 0, .hold = -1};

    return 0;
}

int update_game_frame(StateInfo* stateInfo, MenuInfo* menuInfo)
{
    if (stateInfo->paused) return 0;
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
        &rules->halfInningState, &stateInfo->channels
    );
    ai_update(game, rules, stateInfo->fieldPositions, stateInfo->aiController, &stateInfo->channels);

    // StateInfo is destructured here (the assembly point): each stage receives exactly the
    // worlds it touches — mutable physical (MatchSession), read-only legal (GameRulesState),
    // geometry, the tick's input (the channels) and its one output — a signature is its complete
    // edge list. The client's input memory is not among them: only CONTROL ever names it.

    // 2. INGEST — every controller's declarations, judged against the world as it stands now. After this
    // line the channels are empty and no intent-shaped thing is left to read; what survives is a local
    // of this tick, handed to the next stage and gone with the frame.
    IngestedCommands commands = ingest_intents(game, rules, stateInfo->fieldPositions, &stateInfo->channels);

    // 3. ACTUALIZE — the engine's own order, whatever order the messages came in.
    actualize(game, rules, stateInfo->fieldPositions, &commands, &stateInfo->playSoundEffect);

    // 4. SIMULATE — the physical world takes its step.
    simulate(game, stateInfo->fieldPositions, &rules->referee, &stateInfo->playSoundEffect);

    // 5. ADJUDICATE — the referee, the sole legal authority, reads the physical world const and
    // pronounces what it means.
    update_referee(
        stateInfo->match, stateInfo->fieldPositions, &rules->referee, &rules->halfInningState,
        &rules->betweenPitchState, &rules->scoreboard, &rules->homeRunContestState, &stateInfo->announcement
    );

    // 6. ENFORCE — the legal decisions made physical: the burnt and the scored leave the field, the
    // batting side is asked its questions, and a reset the referee called for is performed. It reads
    // the legal world const; only the referee writes it.
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

    // 7. SETTLE — the referee closes the RESETTING→NONE handshake once the resets are done, and
    // rescans the new physical world to establish its legal tracking.
    referee_finalize(stateInfo->match, &rules->referee, &rules->betweenPitchState);

    // 8. VALIDATE — capture a snapshot when a pitch is released, then check the state's invariants.
    if (stateInfo->match->gameEvents.pitchReleased) {
        state_validator_capture_snapshot(stateInfo, "PITCH_START");
    }

    // Validate state consistency (Debug only)
    if (!state_validator_check(stateInfo)) {
        state_validator_dump(stateInfo, "State Consistency Check Failed");
        stateInfo->paused = 1;
    }

    // 9. CLEAR — the frame's events end with the frame.
    clear_frame_events(&stateInfo->match->gameEvents);
    return 1;
}
void draw_game_frame(
    const MatchSession* match, const Scoreboard* scoreboard, int paused, double alpha, ResourceManager* rm
)
{
    // players and ball are the building blocks of all the action on the screen.
    if (!paused) {
#ifndef NO_RENDER
        draw_player_renderer(match, scoreboard, alpha, rm);
        draw_ball(&(match->ballInfo), alpha, rm);
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
