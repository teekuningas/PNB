/*
    execute_actions.c — Intent execution.

    Translates ActionFlags (set by human input or AI) into physical state changes:
    throwing, pitching, batting, base running, player selection.
*/

#include "globals.h"
#include "execute_actions.h"
#include "common_logic.h"
#include "vector_math.h"
#include "actions/pitching_system.h"
#include "actions/batting_system.h"
#include "actions/throwing_system.h"
#include "ai/catching_ai.h"
#include "ai/batting_ai.h"
#include "base_logic.h"
#include "base_control.h"
#include "rules_pure/player_utils.h"

#define ANIMATION_FREQUENCY 3

static void change_batter(MatchSession* match, const Scoreboard* scoreboard, const HalfInningState* his);
static void
take_free_walk_decision(MatchSession* match, const Scoreboard* scoreboard, const FieldPositions* fieldPositions);
static void
base_run(MatchSession* match, const RefereeState* referee, const FieldPositions* fieldPositions, BaseID base);

void init_execute_actions(MatchSession* match, ClientInputState* clientInput)
{
    // just initialize everyone of these static variables to zero
    match->pendingActionState.meter_counter = 0;
    match->pendingActionState.meter_counter_max = 0;
    match->pendingActionState.current_catching_action = CATCHING_ACTION_NONE;

    // Client-local input memory: initialized once here, self-clears during play (a physical-world reset
    // never touches it — see reset_flow_state).
    for (int i = 0; i < BASE_COUNT; i++) {
        clientInput->run_press_window[i] = 0;
    }
    clientInput->pitchWidget.counter = 0;
    clientInput->pitchWidget.counter_max = 0;
    clientInput->pitchWidget.dir = 0;
    clientInput->pitchWidget.mode = WIDGET_IDLE;
    clientInput->throwWidget.counter = 0;
    clientInput->throwWidget.counter_max = 0;
    clientInput->throwWidget.dir = 0;
    clientInput->throwWidget.mode = WIDGET_IDLE;

    reset_pitching_system(match);
    init_batting_system(match);
    init_throwing_system(match);
    match->pendingActionState.run_bat_flag = 0;

    // ai uses a few flags..

    init_catching_ai(&(match->aiState));

    init_batting_ai(&(match->aiState));
}

void execute_actions(
    MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions, int* playSoundEffect
)
{
    int i;

    /*
     * CATCHING TEAM
     */

    // THROW — the engine-owned actualizer reads the phased ThrowDeclaration (cTAF.throw), runs the
    // deterministic windup clock (ThrowActualization), and releases once the intent is COMMITTED (power
    // known) when the clock reaches throw_windup_total_frames(power) — ONE rule for both producers, no
    // client "fire-now" edge. The AI declares COMMITTED in one frame; a human streams
    // INITIATED{target} → COMMITTED{power}, its power a trusted value from the client charge widget — never
    // a live meter or this clock. It also cancels a pitch for a throw (never the reverse). The same
    // phased-declaration + engine-windup-clock shape as the pitch.
    update_throw_actualization(match, fieldPositions);
    // if move keys have been pressed, depending on if its down or release
    // call corresponding function for every direction
    for (i = 0; i < DIRECTION_COUNT; i++) {
        if (match->aF.cTAF.move[i] == ACTION_TRIGGER_START) {
            fielder_move(match, i);
        } else if (match->aF.cTAF.move[i] == ACTION_TRIGGER_STOP) {
            fielder_stop_move(match, i);
        }
    }
    // (No charging-thrower facing pass: the throw windup begins the instant a throw is declared —
    // begin_throw_windup faces the thrower at the target base and movement is suppressed for the whole
    // windup — so there is no pre-windup charge window whose orientation needs a separate writer.)

    // if change player key has been pressed
    if (match->aF.cTAF.change_player == ACTION_TRIGGER_START) {
        // no one must have the ball
        if (match->pII.hasBallIndex == -1) {
            // we go to next element in changePlayerArray.
            match->pII.changePlayerArrayIndex = (match->pII.changePlayerArrayIndex + 1) % CHANGE_PLAYER_COUNT;
            // and try to ensure that there is difference. we dont want to end up in a endless loop
            // though so we do it only once.
            if (match->pII.controlIndex == match->pII.fielderRankedIndices[match->pII.changePlayerArrayIndex]) {
                match->pII.changePlayerArrayIndex = (match->pII.changePlayerArrayIndex + 1) % CHANGE_PLAYER_COUNT;
            }
            // and then set the flag, so that other parts of code can handle
            // the job
            change_player(match);
        }
        match->aF.cTAF.change_player = ACTION_IDLE;
    }
    // if drop ball key has been pressed, try dropping
    if (match->aF.cTAF.drop_ball == ACTION_TRIGGER_START) {
        drop_ball(match);
    }
    // pitching — the engine-owned actualizer reads the phased declaration (cTAF.pitch), runs the
    // deterministic windup clock, and releases on AIMED (or fakes / valesyöttö otherwise). No meter read,
    // no lock machine, no stuck-state auto-clear: the declaration is consumer-cleared at resolution.
    update_pitch_actualization(match, &rules->referee, fieldPositions);

    /*
     * BATTING TEAM
     */
    // when there's no batter, user is prompted to select the next batter
    if (match->aF.bTAF.choose_batter == CHOOSE_BATTER_NEXT) {
        change_batter(match, &rules->scoreboard, &rules->halfInningState);
    } else if (match->aF.bTAF.choose_batter == CHOOSE_BATTER_SELECT) {
        select_batter(match, &rules->referee, fieldPositions);
    }
    // free walk decisions, take_free_walk can be 0, 1 or 2. if its 2
    // takeFreeWalkDecision() is called but will basically just set take_free_walk to 0.
    if (match->aF.bTAF.take_free_walk > FREE_WALK_IDLE) {
        take_free_walk_decision(match, &rules->scoreboard, fieldPositions);
    }
    // batter angles
    if (match->aF.bTAF.increase_batter_angle == ACTION_TRIGGER_START) {
        start_increase_batter_angle(match);
    } else if (match->aF.bTAF.increase_batter_angle == ACTION_TRIGGER_STOP) {
        stop_increase_batter_angle(match);
    }
    if (match->aF.bTAF.decrease_batter_angle == ACTION_TRIGGER_START) {
        start_decrease_batter_angle(match);
    } else if (match->aF.bTAF.decrease_batter_angle == ACTION_TRIGGER_STOP) {
        stop_decrease_batter_angle(match);
    }
    // batting
    if (match->aF.bTAF.swing == BAT_ACTION_POWER_SET) {
        select_power(match);
    } else if (match->aF.bTAF.swing == BAT_ACTION_ANGLE_SET) {
        select_angle(match);
    }
    // baserunners must be able to run!
    for (i = 0; i < BASE_COUNT; i++) {
        base_run(match, &rules->referee, fieldPositions, i);
    }
    // this is used to handle a lot of stuff happening between and after the decisions.
    update_batting(match, &rules->referee, &rules->betweenPitchState, fieldPositions, playSoundEffect);
}

static void
take_free_walk_decision(MatchSession* match, const Scoreboard* scoreboard, const FieldPositions* fieldPositions)
{
    if (match->aF.bTAF.take_free_walk == FREE_WALK_ACCEPT) {
        int index = match->flowControl.freeWalkIndex;
        BaseID base = match->flowControl.freeWalkBase;
        if (index != -1) {
            // there can be a little gap between the decision and when the possibility to decide came
            // so player might have run already to the following base, and free walk actually
            // gave him the right to go to just that base.
            // so if he still has the same base as before we can go on
            if (scoreboard->period >= 4) {
                // REFEREE MIGRATION: Logic moved to referee.c
                // We just signal the event here.
                match->gameEvents.freeWalkAccepted = 1;

            } else {
                BaseID currentBaseId = match->playerInfo[index].bTPI.baseId;

                if (currentBaseId == base) {
                    if (base != BASE_THIRD) {
                        // Start running to the next base
                        run_to_next_base(match, fieldPositions, index, base);
                        // Protected from wounds/tags while advancing freely
                        match->playerInfo[index].bTPI.state = PLAYER_STATE_ADVANCING_FREELY;
                    }
                    // 3rd base: no physical run needed — referee scores the run directly
                    // and sets hasScored, which causes enforceLegalState to remove the player
                }
                // REFEREE MIGRATION: Logic moved to referee.c
                // We just signal the event here.
                match->gameEvents.freeWalkAccepted = 1;
            }
        }
    }
    // no more decision to make.
    match->flowControl.waitingForFreeWalkDecision = 0;
    match->aF.bTAF.take_free_walk = FREE_WALK_IDLE;
}
// so when there is no batter and few other conditions hold
// we can select the batter from one player from the normal ordering of players and three joker players
static void change_batter(MatchSession* match, const Scoreboard* scoreboard, const HalfInningState* his)
{
    int done = 0;
    int counter = 0;
    // index in a teams[] array
    int battingTeamIndex = get_batting_team_index(scoreboard);
    int index;

    match->aF.bTAF.choose_batter = 0;
    // batter_select variable will point to the current player in selection
    // and now as we are changing the selection, we add one to it.
    match->pendingActionState.batter_select++;
    // here we have a loop that basically just searches through the possible players and selects
    // the next one. batter_select == 0 indicates that it is a normal player, batter_select != 0 indicates
    // it is a joker player. §12: the regular slot is always on offer until the referee has pronounced
    // the batting turn spent (halfInningState.lastBatter.turnExhausted), after which only jokers are.
    // there must be at least one player as this function cannot get called without
    // waitingForBatterDecision-flag, and that can flag cant be true if
    // there is not at least one player.
    while (done == 0) {
        if (match->pendingActionState.batter_select == 0) {
            if (his->lastBatter.turnExhausted == 0)
                done = 1;
            else
                match->pendingActionState.batter_select = 1;
        } else if (match->pendingActionState.batter_select == 4) {
            if (his->lastBatter.turnExhausted == 0) {
                match->pendingActionState.batter_select = 0;
                done = 1;
            } else
                match->pendingActionState.batter_select = 1;

        } else {
            if (match->playerInfo[match->pII.jokerIndices[match->pendingActionState.batter_select - 1]].bTPI.joker ==
                JOKER_USED)
                match->pendingActionState.batter_select++;
            else
                done = 1;
        }
        if (counter == 4) done = 1;
        counter++;
    }
    // now we have the batter_select value and we just need to find a corresponding index for that
    // player.
    if (match->pendingActionState.batter_select == 0) {
        index = scoreboard->teams[battingTeamIndex].batterOrder[scoreboard->teams[battingTeamIndex].batterOrderIndex];
    } else {
        index = match->pII.jokerIndices[match->pendingActionState.batter_select - 1];
    }
    // and set it here.
    match->pII.batterSelectionIndex = index;
}

void generic_sling_ball(BallInfo* ballInfo, float x, float y, float z)
{
    // Make ball visible and moving
    ballInfo->visible = 1;
    ballInfo->moving = 1;

    // Set the velocity
    vec3_set_xyz(&(ballInfo->velocity), x, y, z);
}

// Base running — the shared actualizer for the RunIntent command (set by the human in
// action_invocations or by the AI in batting_ai; execute_actions does not care which).
//
// The producer declares ARM vs COMMIT explicitly (RunIntent in globals.h); the actualizer NEVER
// infers it from live-ball state. Reading the stale `batter_can_advance` to choose was the slice's
// offense regression — a pre-pitch arm fired after a caught fly got mis-read as "ball live, run now",
// sending the next batter into a caught-fly out. With the intent explicit, that is unrepresentable.
//
//   RUN_FORWARD (single press — "arm / lead")
//     - settled on base / at bat  -> arm to advance on the next pitch (release_pitch commits a base
//                                    runner, the hit commits the batter), and lead off 1st/2nd. If
//                                    still settling onto the base (mid-move), wait this frame. NEVER
//                                    starts a settled runner moving now — that is RUN_COMMIT's job.
//     - leading / already running -> keep going to the next base.
//   RUN_COMMIT (double press — "run now")
//     - any state                 -> start for the next base immediately (a steal / chaining a hit).
//                                    For the batter this is gated inside run_to_next_base(BASE_HOME),
//                                    which refuses unless the ball is live, so a commit issued when
//                                    the ball is dead is harmlessly ignored — never a mis-run.
//   RUN_BACK
//     - settled on base / at bat  -> disarm a pending lead.
//     - otherwise                 -> run back to the previous base.
//
// The command is single-frame and consumed here; it encodes no timing (the old AI double-click sim
// is gone). A human's single/double press maps to RUN_FORWARD/RUN_COMMIT in action_invocations.
static void
base_run(MatchSession* match, const RefereeState* referee, const FieldPositions* fieldPositions, BaseID base)
{
    int index = get_base_controller(match, referee, base);
    RunIntent cmd = match->aF.bTAF.base_run[base];
    match->aF.bTAF.base_run[base] = RUN_NONE; // consume

    if (index == -1 || cmd == RUN_NONE) return;

    PlayerUnitState state = match->playerInfo[index].bTPI.state;
    int settled_on_base = (state == PLAYER_STATE_ON_BASE || state == PLAYER_STATE_AT_BAT);

    if (cmd == RUN_FORWARD) {
        if (settled_on_base) {
            // Arm only — never commit a settled runner here.
            if (match->pRAI.will_start_running[base] == 0 && match->playerInfo[index].cPI.moving == 0) {
                match->pRAI.will_start_running[base] = 1;
                if (base == BASE_FIRST || base == BASE_SECOND) {
                    lead_from_base(match->playerInfo, match->playerRuntime, fieldPositions, index);
                }
            }
            // else: already armed, or still settling onto the base — wait.
        } else {
            // Leading or mid-run: keep going to the next base.
            run_to_next_base(match, fieldPositions, index, base);
        }
    } else if (cmd == RUN_COMMIT) {
        // Deliberate "go now". Safe for the batter even if mis-issued: run_to_next_base(BASE_HOME)
        // self-gates on batter_can_advance.
        run_to_next_base(match, fieldPositions, index, base);
    } else { // RUN_BACK
        if (settled_on_base) {
            match->pRAI.will_start_running[base] = 0; // cancel a pending lead
        } else {
            run_to_previous_base(match, fieldPositions, index, base);
        }
    }
}

void update_meters(MatchSession* match, const ClientInputState* clientInput)
{
    // The catching meter (meter_value) is a CLIENT display, read from the LOCAL HUMAN's input widget — the
    // pitch sampler (power ping-pong, then aim descent) or the throw charge widget — whichever is active.
    // Both are self-contained client widgets (ClientInputState); neither the meter nor any input logic reads
    // an engine actualization clock (the engine↔client contract). An AI pitch/throw arms NO widget, so it
    // drives no meter automatically — no phase-gate special-case needed. (meter_value is render-only: read
    // solely by game_screen.c, never by logic and not in the checksum.)
    if (clientInput->pitchWidget.mode != WIDGET_IDLE && clientInput->pitchWidget.counter_max > 0) {
        match->pRAI.meter_value = (float)clientInput->pitchWidget.counter / clientInput->pitchWidget.counter_max;
    } else if (clientInput->throwWidget.mode != WIDGET_IDLE && clientInput->throwWidget.counter_max > 0) {
        match->pRAI.meter_value = (float)clientInput->throwWidget.counter / clientInput->throwWidget.counter_max;
    } else {
        update_batting_meter(match);
    }
}

void ai_update(
    MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions,
    AIControllerState* aiController
)
{
    int battingTeamIndex = get_batting_team_index(&rules->scoreboard);
    TeamControlMode battingControl = rules->scoreboard.teams[battingTeamIndex].control;
    TeamControlMode catchingControl = rules->scoreboard.teams[(battingTeamIndex + 1) % 2].control;

    // first ai for catching team

    if (team_is_ai(catchingControl)) {
        update_catching_ai(match, rules, aiController);
    }
    // then ai for batting team
    if (team_is_ai(battingControl)) {
        update_batting_ai(match, rules, fieldPositions, aiController);
    }
}