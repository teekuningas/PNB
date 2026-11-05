/*
    actualize.c — ACTUALIZE, stage 3 of the tick.

    Run the engine's own fixed order of actualization — throw windup, fielder movement, control changes,
    drops, the pitch clock, batting, base running — reading the commands the INGEST gate left. The order
    here is the engine's, never the order messages happened to arrive in, so which producer spoke first
    can never change the simulation. Nothing here asks a permission: the gate already has.
*/

#include "actualize.h"
#include "common_logic.h"
#include "actions/pitching_system.h"
#include "actions/batting_system.h"
#include "actions/throwing_system.h"
#include "actions/fielder_movement.h"

static void take_free_walk_decision(
    MatchSession* match, const Scoreboard* scoreboard, const FieldPositions* fieldPositions, int accepted
);
static void base_run(MatchSession* match, const FieldPositions* fieldPositions, BaseID base, int index, RunIntent cmd);

void init_actualization(MatchSession* match)
{
    match->catching_action = (CatchingAction){0};
    init_batting_system(match);
}

void actualize(
    MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions,
    const IngestedCommands* commands, int* playSoundEffect
)
{
    int i;

    /*
     * CATCHING TEAM
     */

    // THROW — a declared target begins the gather, a declared power is held on it, and the engine releases
    // once the power is known and its windup has elapsed: one rule for both producers, no client
    // "fire-now" edge. The AI declares both values in one tick; a human declares the power when it lets
    // go of the key, a trusted value from its own charge widget — never a live meter or this clock.
    update_throw_actualization(match, fieldPositions, &commands->throw);
    // MOVE — the engine walks the controlled fielder toward the destination the gate stored. One
    // behaviour for every producer: the four per-direction key flags that used to be read here are
    // gone, and with them the last thing about movement that was shaped like an input device. It
    // runs after the throw actualizer on purpose — a throw declared this tick has already claimed
    // the fielder's feet by the time we get here.
    update_controlled_fielder_movement(match);
    // (No charging-thrower facing pass: the throw windup begins the instant a throw is declared —
    // begin_throw_windup faces the thrower at the target base and movement is suppressed for the whole
    // windup — so there is no pre-windup charge window whose orientation needs a separate writer.)

    // Hand control to the next fielder. The gate has already established that nobody holds the ball,
    // so this is the move itself and nothing else.
    if (commands->change_player) {
        // we go to next element in changePlayerArray.
        match->catchingState.ranking_cursor = (match->catchingState.ranking_cursor + 1) % CHANGE_PLAYER_COUNT;
        // and try to ensure that there is difference. we dont want to end up in a endless loop
        // though so we do it only once.
        if (match->catchingState.controlled_fielder ==
            match->catchingState.chase_ranking[match->catchingState.ranking_cursor]) {
            match->catchingState.ranking_cursor = (match->catchingState.ranking_cursor + 1) % CHANGE_PLAYER_COUNT;
        }
        // and hand control to whoever the cursor now names
        change_player(match);
    }
    // Put the held ball on the ground. Permission (someone is holding it, and no throw or pitch has a
    // claim on it) was settled at the gate; nothing between there and here can have taken the ball
    // away, because a release requires exactly the throw the gate refuses to drop through.
    if (commands->drop_ball) {
        drop_ball(match);
    }
    // PITCH — a declared power begins the windup, a declared aim is held on it, and the engine's clock
    // decides the release: aimed, a real pitch; unaimed, a valesyöttö. No meter read, no phase.
    update_pitch_actualization(match, &rules->referee, fieldPositions, &commands->pitch);

    /*
     * BATTING TEAM
     */
    // §27: somebody was named to take the bat. The seating owns the one question the gate left it —
    // whether the seat is physically free — because a batter who has left the plate keeps safety at
    // home for a while yet, and that is true of the body whoever asked for what.
    if (commands->seat_batter_index != -1) {
        seat_batter(match, &rules->referee, fieldPositions, commands->seat_batter_index);
    }
    // The offered free walk was answered this tick — taking it starts the run, declining it just
    // closes the offer. Either answer ends the prompt.
    if (commands->take_free_walk != FREE_WALK_IDLE) {
        take_free_walk_decision(
            match, &rules->scoreboard, fieldPositions, commands->take_free_walk == FREE_WALK_ACCEPT
        );
    }

    // baserunners must be able to run!
    for (i = 0; i < BASE_COUNT; i++) {
        if (commands->base_run[i] != RUN_NONE) {
            base_run(match, fieldPositions, i, commands->base_runner[i], commands->base_run[i]);
        }
    }
    // SWING — the batter's frame. The two declared VALUES are not passed: the gate wrote them into
    // at_bat.swing, where they are held until the contact frame consumes them. Only the
    // aim and the withdrawal come through as per-tick commands, because only they are things that
    // HAPPEN; a power and an elevation are things that ARE.
    update_batting(
        match, &rules->referee, fieldPositions, commands->swing_angle_declared, commands->swing_angle,
        commands->swing_pass, playSoundEffect
    );
}

static void take_free_walk_decision(
    MatchSession* match, const Scoreboard* scoreboard, const FieldPositions* fieldPositions, int accepted
)
{
    if (accepted) {
        int index = match->free_walk.runner;
        BaseID base = match->free_walk.from;
        if (index != -1) {
            // The walk's legal consequence is the referee's: the event is all it is told. What is left
            // here is the body's part of it, and the home-run contest's walk has none.
            match->gameEvents.freeWalkAccepted = 1;
            // there can be a little gap between the decision and when the possibility to decide came
            // so player might have run already to the following base, and free walk actually
            // gave him the right to go to just that base.
            // so if he still has the same base as before we can go on
            if (scoreboard->period < 4 && match->playerInfo[index].bTPI.baseId == base && base != BASE_THIRD) {
                // Start running to the next base
                run_to_next_base(match, fieldPositions, index, base);
                // Protected from wounds/tags while advancing freely
                match->playerInfo[index].bTPI.state = PLAYER_STATE_ADVANCING_FREELY;
            }
            // 3rd base: no physical run needed — the referee scores the run directly and sets
            // hasScored, which is what makes enforce_legal_state remove the player
        }
    }
    // no more decision to make.
    match->offers.free_walk = 0;
}

// Base running — the shared actualizer for the RunIntent command (set by the human in
// action_invocations or by the AI in batting_ai; the actualizer does not care which).
//
// The producer declares ARM vs COMMIT explicitly (RunIntent in globals.h); the actualizer NEVER
// infers it from live-ball state. Reading the batter's §32 right to leave home (`may_leave_home`, held for
// the whole at-bat) as if it meant "the ball is live" was the slice's offense regression — a pre-pitch
// arm fired after a caught fly got mis-read as "ball live, run now",
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
//                                    which refuses until the turn's first pitch has left the hand
//                                    (§32), so a commit issued before then is harmlessly ignored.
//   RUN_BACK
//     - settled on base / at bat  -> disarm a pending lead.
//     - otherwise                 -> run back to the previous base.
//
// The command is single-frame and consumed here; it encodes no timing (the old AI double-click sim
// is gone). A human's single/double press maps to RUN_FORWARD/RUN_COMMIT in action_invocations.
static void base_run(MatchSession* match, const FieldPositions* fieldPositions, BaseID base, int index, RunIntent cmd)
{
    PlayerUnitState state = match->playerInfo[index].bTPI.state;
    int settled_on_base = (state == PLAYER_STATE_ON_BASE || state == PLAYER_STATE_AT_BAT);

    if (cmd == RUN_FORWARD) {
        if (settled_on_base) {
            // Arm only — never commit a settled runner here.
            if (match->runner_commitments.armed[base] == 0 && match->playerInfo[index].cPI.moving == 0) {
                match->runner_commitments.armed[base] = 1;
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
        // self-gates on the batter's §32 right to leave home.
        run_to_next_base(match, fieldPositions, index, base);
    } else { // RUN_BACK
        if (settled_on_base) {
            match->runner_commitments.armed[base] = 0; // cancel a pending lead
        } else {
            run_to_previous_base(match, fieldPositions, index, base);
        }
    }
}
