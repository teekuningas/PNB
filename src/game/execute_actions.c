/*
    execute_actions.c — Intent execution.

    Translates ActionFlags (set by human input or AI) into physical state changes:
    throwing, pitching, batting, base running, player selection.
*/

#include "globals.h"
#include "execute_actions.h"
#include "common_logic.h"
#include "actions/pitching_system.h"
#include "actions/batting_system.h"
#include "actions/throwing_system.h"
#include "ai/catching_ai.h"
#include "ai/batting_ai.h"
#include "base_logic.h"
#include "base_control.h"
#include "rules_pure/player_utils.h"

#define ANIMATION_FREQUENCY 3

static void change_batter(MatchSession* match, const Scoreboard* scoreboard, const PlayerCounters* playerCounters);
static void
take_free_walk_decision(MatchSession* match, const Scoreboard* scoreboard, const FieldPositions* fieldPositions);
static void
base_run(MatchSession* match, const RefereeState* referee, const FieldPositions* fieldPositions, BaseID base);
;

void init_execute_actions(MatchSession* match)
{
    // just initialize everyone of these static variables to zero
    match->pendingActionState.meter_counter = 0;
    match->pendingActionState.meter_counter_max = 0;
    match->pendingActionState.current_catching_action = CATCHING_ACTION_NONE;
    for (int i = 0; i < BASE_COUNT; i++) {
        match->pendingActionState.run_press_window[i] = 0;
    }

    reset_pitching_system(match);
    init_batting_system(match);
    init_throwing_system(match);
    match->pendingActionState.run_bat_flag = 0;

    // ai uses a few flags..

    init_catching_ai(&(match->aiState));
    match->pendingActionState.aiActionEventLock = -1;
    match->pendingActionState.aiLockUpdate = 0;

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

    // THROW — a single parameterized command (target base + declared power). The engine owns the
    // windup: declaring the intent starts a windup whose LENGTH scales with the declared power; the ball
    // leaves at the end of the windup (the release block below), with a velocity read from the declared
    // power — never a live meter. No START/STOP pair: there is one declaration, then the engine acts.
    if (match->aF.cTAF.throw.target != BASE_NONE) {
        BaseID base = match->aF.cTAF.throw.target;
        float power = match->aF.cTAF.throw.power;
        match->aF.cTAF.throw.target = BASE_NONE; // consume the intent regardless of success
        // can throw only if someone has the ball and no other catching action is in progress (the real
        // execution-side mutex — the AI no longer mirrors it with its own lock).
        if (match->pII.hasBallIndex != -1 &&
            match->pendingActionState.current_catching_action == CATCHING_ACTION_NONE) {
            // stop pitching if throwing
            if (match->pRAI.pitch_state != PITCH_STAGE_NONE) {
                match->aF.cTAF.pitch = PITCH_ACTION_IDLE;
                match->pRAI.pitch_state = PITCH_STAGE_NONE;
                match->pendingActionState.pitch_phase = PITCH_PHASE_NONE;
                // when pitching the ball is moved to the center of the plate so now when we are terminating the
                // pitch to throw, we must move the ball back to the player
                match->ballInfo.location.x = match->playerInfo[match->pII.hasBallIndex].tPI.location.x;
                match->ballInfo.location.z = match->playerInfo[match->pII.hasBallIndex].tPI.location.z;
            }
            // throw_going_to_base variables are used to have better control
            // over basemen who are wanting go out of base catching the ball.
            // throws can be directed only towards bases.
            prepare_throw(match, fieldPositions, base);
            // start by loading (begins the engine-owned windup, scaled by power)
            throw_load(match, base, power);
            // only commit the action state if the throw actually started (throw_load refuses if the
            // thrower is already on the target base)
            if (match->pendingActionState.throw_going_on == 1) {
                match->pendingActionState.current_catching_action = CATCHING_ACTION_THROWING;
            }
        }
    }
    // Engine-owned windup completion: when the windup clock reaches its (power-scaled) end, the ball
    // leaves. This replaces the old human-STOP / AI-meter-watch release — release is now time-driven.
    if (match->pendingActionState.current_catching_action == CATCHING_ACTION_THROWING &&
        match->pendingActionState.throw_going_on == 1 &&
        match->pendingActionState.meter_counter >= match->pendingActionState.meter_counter_max) {
        throw_release(match);
        match->pendingActionState.current_catching_action = CATCHING_ACTION_NONE;
    }
    // Auto-clear: if throw was interrupted externally (e.g., ball caught by game_manipulation
    // cleared throw_going_on), reset the action state so other actions can proceed.
    if (match->pendingActionState.current_catching_action == CATCHING_ACTION_THROWING &&
        match->pendingActionState.throw_going_on == 0) {
        match->pendingActionState.current_catching_action = CATCHING_ACTION_NONE;
    }
    // Safety: if throw_going_on is stuck but no throw action is active, clear it.
    // This catches the case where a reset cleared current_catching_action but missed throw_going_on.
    if (match->pendingActionState.throw_going_on == 1 &&
        match->pendingActionState.current_catching_action != CATCHING_ACTION_THROWING) {
        match->pendingActionState.throw_going_on = 0;
        match->pRAI.throw_going_to_base = -1;
    }
    // if move keys have been pressed, depending on if its down or release
    // call corresponding function for every direction
    for (i = 0; i < DIRECTION_COUNT; i++) {
        if (match->aF.cTAF.move[i] == ACTION_TRIGGER_START) {
            fielder_move(match, i);
        } else if (match->aF.cTAF.move[i] == ACTION_TRIGGER_STOP) {
            fielder_stop_move(match, i);
        }
    }

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
    // pitching
    if (match->aF.cTAF.pitch == PITCH_ACTION_START) {
        start_pitch(match);
    } else if (match->aF.cTAF.pitch == PITCH_ACTION_POWER_SET) {
        continue_pitch(match);
    } else if (match->aF.cTAF.pitch == PITCH_ACTION_ANGLE_SET) {
        release_pitch(match, &rules->referee, fieldPositions);
    }
    // Safety auto-clear: if pitching action is stuck but pitch_state is NONE, release it.
    if (match->pendingActionState.current_catching_action == CATCHING_ACTION_PITCHING &&
        match->pRAI.pitch_state == PITCH_STAGE_NONE && match->pendingActionState.pitch_phase == PITCH_PHASE_NONE) {
        match->pendingActionState.current_catching_action = CATCHING_ACTION_NONE;
    }

    /*
     * BATTING TEAM
     */
    // when there's no batter, user is prompted to select the next batter
    if (match->aF.bTAF.choose_batter == CHOOSE_BATTER_NEXT) {
        change_batter(match, &rules->scoreboard, &rules->playerCounters);
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
static void change_batter(MatchSession* match, const Scoreboard* scoreboard, const PlayerCounters* playerCounters)
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
    // it is a joker player.
    // there must be at least one player as this function cannot get called without
    // waitingForBatterDecision-flag, and that can flag cant be true if
    // there is not at least one player.
    while (done == 0) {
        if (match->pendingActionState.batter_select == 0) {
            if (playerCounters->nonJokerPlayersLeft != 0)
                done = 1;
            else
                match->pendingActionState.batter_select = 1;
        } else if (match->pendingActionState.batter_select == 4) {
            if (playerCounters->nonJokerPlayersLeft != 0) {
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
    set_vector_xyz(&(ballInfo->velocity), x, y, z);
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

void update_meters(MatchSession* match)
{
    update_pitching_meter(match);

    if (match->pendingActionState.throw_going_on == 1) {
        if (match->pendingActionState.meter_counter < match->pendingActionState.meter_counter_max) {
            match->pendingActionState.meter_counter += 1;
        }
        match->pRAI.meter_value =
            1.0f * match->pendingActionState.meter_counter / match->pendingActionState.meter_counter_max;
    } else {
        update_batting_meter(match);
    }
}

void ai_update(
    MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions, unsigned int* rng_seed
)
{
    int battingTeamIndex = get_batting_team_index(&rules->scoreboard);
    TeamControlMode battingControl = rules->scoreboard.teams[battingTeamIndex].control;
    TeamControlMode catchingControl = rules->scoreboard.teams[(battingTeamIndex + 1) % 2].control;

    // first ai for catching team

    if (team_is_ai(catchingControl)) {
        update_catching_ai(match, rules, rng_seed);
    }
    // then ai for batting team
    if (team_is_ai(battingControl)) {
        update_batting_ai(match, rules, fieldPositions, rng_seed);
    }
}