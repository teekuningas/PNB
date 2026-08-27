/*
    execute_actions.c — the INGEST gate, and intent execution.

    Two jobs, in this order, once per tick:

    1. INGEST. Drain both teams' intent channels — everything the controllers declared during the
       CONTROL stage at the frame top — decide what each message is PERMITTED to do against the world
       as it stands at the top of this tick, and interpret the survivors into engine-shaped commands.
       Nothing intent-shaped survives ingestion: what comes out is a plain per-tick command block, and
       the channels are empty from here to the end of the frame.

    2. ACTUALIZE. Run the engine's own fixed order of actualization — throw windup, fielder movement,
       control changes, drops, the pitch clock, batting, base running — reading those commands. The
       order here is the engine's, never the order messages happened to arrive in, so which producer
       spoke first can never change the simulation.

    Actions not yet moved onto the channel are still read from the persistent ActionFlags struct
    (globals.h) further down this file; they convert one slice at a time.
*/

#include "globals.h"
#include "execute_actions.h"
#include "common_logic.h"
#include "vector_math.h"
#include "actions/pitching_system.h"
#include "actions/batting_system.h"
#include "actions/throwing_system.h"
#include "actions/fielder_movement.h"
#include "ai/catching_ai.h"
#include "ai/batting_ai.h"
#include "base_logic.h"
#include "base_control.h"
#include "rules_pure/player_utils.h"

#define ANIMATION_FREQUENCY 3

static void change_batter(MatchSession* match, const Scoreboard* scoreboard, const HalfInningState* his);
static void take_free_walk_decision(
    MatchSession* match, const Scoreboard* scoreboard, const FieldPositions* fieldPositions, int accepted
);
static void base_run(
    MatchSession* match, const RefereeState* referee, const FieldPositions* fieldPositions, BaseID base, RunIntent cmd
);

// What ingestion leaves behind: this tick's commands, in engine terms. It is a local of
// execute_actions, so it cannot outlive the tick, cannot be snapshot, and cannot be read by a
// controller — the same guarantees the channel itself has, carried through the interpretation step.
// Only actions whose whole content is "do this, now" appear here; a phased declaration (pitch, throw)
// is engine state with a lifetime and lives in the World.
typedef struct _IngestedCommands {
    int change_player; // hand control to the next fielder
    int drop_ball; // put the held ball down
    FreeWalkAction take_free_walk; // FREE_WALK_IDLE when no answer was given this tick
    RunIntent base_run[BASE_COUNT]; // per base, RUN_NONE where nothing was commanded
} IngestedCommands;

static Permission permit(const MatchSession* match, const IntentMessage* message);
static IngestedCommands ingest_intents(MatchSession* match, IntentChannels* channels);

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

    init_batting_ai(&(match->aiState));
}

// The channel's producer-side operation. It lives beside the gate that drains it because the channel
// has exactly two operations and no other owner yet; when the intent types stop needing the whole
// globals.h monolith they get a file of their own.
//
// A full channel raises `overflowed` rather than growing or wrapping. Dropping the oldest message
// would be the same bug with better manners: an intent silently lost is a divergence between two
// machines that agreed on everything else.
void intent_push(IntentChannel* channel, IntentMessage message)
{
    if (channel->count >= INTENT_CHANNEL_CAPACITY) {
        channel->overflowed = 1;
        return;
    }
    channel->message[channel->count] = message;
    channel->count++;
}

// The permission rules — the one place a declared intent is told no.
//
// Each answer carries the rule that refused it, not just a bool. Nothing consumes the reason yet; the
// point is that the refusal is not thrown away at the moment it is made, because every later consumer
// (an on-screen hint, a replay that reads back, a rule set that can be asked what it still forbids)
// needs the rule identity and cannot recover it afterwards.
//
// These read the world as it stands at the TOP of the tick, which is the whole reason ingestion runs
// before actualization: every producer's messages are judged against the same settled world, so no
// producer can be advantaged by the engine's internal ordering. Where that reading is not yet
// provably the same as the one the old flag-consumption site made, the check stays at its site for
// now and moves in the slice that rewrites it — see base running below.
static Permission permit(const MatchSession* match, const IntentMessage* message)
{
    switch (message->kind) {
    case INTENT_CHANGE_PLAYER:
        // Control follows the ball: while somebody holds it, it is theirs.
        if (match->pII.hasBallIndex != -1) {
            return (Permission){0, RULE_CHANGE_PLAYER_NEEDS_EMPTY_HANDS};
        }
        return (Permission){1, RULE_NONE};

    case INTENT_DROP_BALL:
        if (match->pII.hasBallIndex == -1) {
            return (Permission){0, RULE_DROP_NEEDS_BALL_IN_HAND};
        }
        // A gathered throw owns the ball until the engine releases it. Both the declaration and the
        // engine's own throwing mutex are checked: the declaration covers a throw declared THIS tick
        // (whose windup has not started yet at this point in the frame), the mutex covers one already
        // running. Together they are exactly the window in which the ball is spoken for.
        if (match->pendingActionState.current_catching_action == CATCHING_ACTION_THROWING ||
            match->pendingActionState.throwDeclaration.phase != THROW_DECL_IDLE) {
            return (Permission){0, RULE_DROP_NOT_WHILE_THROWING};
        }
        if (match->pRAI.pitch_state != PITCH_STAGE_NONE) {
            return (Permission){0, RULE_DROP_NOT_WHILE_PITCHING};
        }
        return (Permission){1, RULE_NONE};

    case INTENT_MOVE_TARGET:
        // The one question about this message that the gate can answer: is there anybody to send?
        // Everything else that stops a fielder moving — a throw gathering, a pitch in progress, a
        // throw's recoil still playing — is a physical claim on the fielder's feet that holds no
        // matter who declared what, so it lives with the walking (fielder_movement.c) and is asked
        // once, there. The point itself is never validated: values are trusted, state is checked.
        if (match->pII.controlIndex == -1) {
            return (Permission){0, RULE_MOVE_NEEDS_A_CONTROLLED_FIELDER};
        }
        return (Permission){1, RULE_NONE};

    case INTENT_TAKE_FREE_WALK:
        // §26: an answer means nothing unless the referee has offered the walk.
        if (match->flowControl.waitingForFreeWalkDecision != 1) {
            return (Permission){0, RULE_FREE_WALK_NEEDS_AN_OFFER};
        }
        return (Permission){1, RULE_NONE};

    case INTENT_PITCH:
    case INTENT_THROW:
        // Deliberately unrestricted here, for now. Whether a pitch or a throw may begin is asked today
        // by the producer before it declares and by the actualizer before it acts, and those two
        // readings are not yet provably the same as one taken here — they will be when the
        // declarations merge into the actualizations and the phase stops being something a producer
        // names at all. That merge is where these rules move, in one piece.
        return (Permission){1, RULE_NONE};

    case INTENT_BASE_RUN:
        // Deliberately unrestricted here. Who may run is decided by which player controls the base,
        // and that reading can still change inside a tick (a batter taken to the plate, a free walk
        // started) between this gate and the moment the command is actualized. Moving it here would
        // be a behaviour change wearing a refactor's clothes, so it stays at the actualization site
        // until the slice that rewrites base running can move it honestly.
        return (Permission){1, RULE_NONE};

    case INTENT_NONE:
    default:
        // Unreachable: the drain rejects a kindless message as malformed before asking permission.
        return (Permission){0, RULE_NONE};
    }
}

// Which team may command what. Not a rule of pesäpallo and not a permission the gate can refuse — it
// is the authority boundary: a controller speaks for its own team and for nothing else. Today the
// worst a violation could do is act on the wrong team's behalf; the moment a channel arrives over a
// wire it is the difference between a peer playing its team and a peer playing yours.
static int intent_belongs_to_batting_team(IntentKind kind)
{
    return kind == INTENT_BASE_RUN || kind == INTENT_TAKE_FREE_WALK;
}

// Drain one channel into the command block. Same-kind duplicates are last-write-wins — a controller
// that changes its mind within a tick means the later value, and per-base commands never collide
// because each writes its own slot.
static void ingest_channel(MatchSession* match, IntentChannel* channel, int is_batting_channel, IngestedCommands* out)
{
    for (int m = 0; m < channel->count; m++) {
        const IntentMessage* message = &channel->message[m];

        // A kindless message and a message on the wrong team's channel are the same class of thing:
        // not a rule refusing an action, but a producer that is broken. Neither is "denied" — both
        // fail the frame through the state validator, because a rule refusing you is ordinary play
        // and a malformed message never is.
        int belongs_to_batting = intent_belongs_to_batting_team(message->kind) ? 1 : 0;
        if (message->kind == INTENT_NONE || belongs_to_batting != is_batting_channel) {
            channel->malformed = 1;
            continue;
        }

        Permission permission = permit(match, message);
        if (!permission.allowed) {
            continue; // the reason dies here for now — the hint system is where it will be read
        }

        switch (message->kind) {
        case INTENT_CHANGE_PLAYER:
            out->change_player = 1;
            break;
        case INTENT_DROP_BALL:
            out->drop_ball = 1;
            break;
        case INTENT_TAKE_FREE_WALK:
            out->take_free_walk = message->as.free_walk.accept ? FREE_WALK_ACCEPT : FREE_WALK_REJECT;
            break;
        case INTENT_BASE_RUN:
            if (message->as.base_run.base >= 0 && message->as.base_run.base < BASE_COUNT) {
                out->base_run[message->as.base_run.base] = message->as.base_run.command;
            }
            break;
        case INTENT_MOVE_TARGET:
            // A destination has a lifetime too, so like the declarations it is written straight into
            // the engine state that owns it rather than into this tick's command block — and unlike
            // them it is never consumed: it is held until a producer replaces it. That is what lets
            // the fielder resume by itself after the engine interrupts its walk, and what makes
            // re-delivering the same message (a rollback repeating its last input) a no-op.
            match->catchingState.controlledMoveTarget = message->as.move_target.point;
            match->catchingState.controlledMoveTargetActive = 1;
            break;
        case INTENT_PITCH:
            // A declaration has a lifetime, so it is not a command in the block above: it is written
            // straight into the engine state that owns it, and the engine clears it when it resolves.
            // Ingestion is the ONLY place a controller's declaration reaches that state.
            match->pendingActionState.pitchDeclaration = message->as.pitch;
            break;
        case INTENT_THROW:
            match->pendingActionState.throwDeclaration = message->as.throw;
            break;
        case INTENT_NONE:
        default:
            break;
        }
    }
    channel->count = 0;
}

// THE INGEST GATE. Both channels are drained here and nowhere else, and they are left empty — which
// is what makes the channel a parameter of the tick rather than a place intent can accumulate. The
// state validator checks that emptiness at the end of every frame.
static IngestedCommands ingest_intents(MatchSession* match, IntentChannels* channels)
{
    IngestedCommands commands = {0};

    ingest_channel(match, &channels->batting, 1, &commands);
    ingest_channel(match, &channels->catching, 0, &commands);

    return commands;
}

void execute_actions(
    MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions, IntentChannels* channels,
    int* playSoundEffect
)
{
    int i;

    // INGEST first: every controller's declarations for this tick, judged against one settled world,
    // become engine commands here. After this line no intent-shaped thing is left to read.
    IngestedCommands commands = ingest_intents(match, channels);

    /*
     * CATCHING TEAM
     */

    // THROW — the engine-owned actualizer reads the phased ThrowDeclaration the gate stored, runs the
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
    // MOVE — the engine walks the controlled fielder toward the destination the gate stored. One
    // behaviour for every producer, idempotent, and the thing the four key flags above are being
    // dissolved into: while a producer still steers by key stream its destination is never set, so
    // this is inert for it. It runs after the throw actualizer on purpose — a throw declared this
    // tick has already claimed the fielder's feet by the time we get here.
    update_controlled_fielder_movement(match);
    // (No charging-thrower facing pass: the throw windup begins the instant a throw is declared —
    // begin_throw_windup faces the thrower at the target base and movement is suppressed for the whole
    // windup — so there is no pre-windup charge window whose orientation needs a separate writer.)

    // Hand control to the next fielder. The gate has already established that nobody holds the ball,
    // so this is the move itself and nothing else.
    if (commands.change_player) {
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
    // Put the held ball on the ground. Permission (someone is holding it, and no throw or pitch has a
    // claim on it) was settled at the gate; nothing between there and here can have taken the ball
    // away, because a release requires exactly the throw the gate refuses to drop through.
    if (commands.drop_ball) {
        drop_ball(match);
    }
    // pitching — the engine-owned actualizer reads the phased declaration the gate stored, runs the
    // deterministic windup clock, and releases on AIMED (or fakes / valesyöttö otherwise). No meter read,
    // no lock machine, no stuck-state auto-clear: the declaration is consumer-cleared at resolution.
    update_pitch_actualization(match, &rules->referee, fieldPositions, rules->halfInningState.strikes);

    /*
     * BATTING TEAM
     */
    // when there's no batter, user is prompted to select the next batter
    if (match->aF.bTAF.choose_batter == CHOOSE_BATTER_NEXT) {
        change_batter(match, &rules->scoreboard, &rules->halfInningState);
    } else if (match->aF.bTAF.choose_batter == CHOOSE_BATTER_SELECT) {
        select_batter(match, &rules->referee, fieldPositions);
    }
    // The offered free walk was answered this tick — taking it starts the run, declining it just
    // closes the offer. Either answer ends the prompt.
    if (commands.take_free_walk != FREE_WALK_IDLE) {
        take_free_walk_decision(match, &rules->scoreboard, fieldPositions, commands.take_free_walk == FREE_WALK_ACCEPT);
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
        base_run(match, &rules->referee, fieldPositions, i, commands.base_run[i]);
    }
    // this is used to handle a lot of stuff happening between and after the decisions.
    update_batting(match, &rules->referee, &rules->betweenPitchState, fieldPositions, playSoundEffect);
}

static void take_free_walk_decision(
    MatchSession* match, const Scoreboard* scoreboard, const FieldPositions* fieldPositions, int accepted
)
{
    if (accepted) {
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
    // it is a joker player. §12: the regular slot is always on offer until the batting order has come
    // round to the designated last batter (halfInningState.lastBatter.regularOrderSpent), after which
    // only jokers are. That flag and not turnExhausted: the latter also carries §12(3)'s "has finally
    // become a runner", which is transient and says nothing about who may be seated.
    // there must be at least one player as this function cannot get called without
    // waitingForBatterDecision-flag, and that can flag cant be true if
    // there is not at least one player.
    while (done == 0) {
        if (match->pendingActionState.batter_select == 0) {
            if (his->lastBatter.regularOrderSpent == 0)
                done = 1;
            else
                match->pendingActionState.batter_select = 1;
        } else if (match->pendingActionState.batter_select == 4) {
            if (his->lastBatter.regularOrderSpent == 0) {
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
static void base_run(
    MatchSession* match, const RefereeState* referee, const FieldPositions* fieldPositions, BaseID base, RunIntent cmd
)
{
    int index = get_base_controller(match, referee, base);

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