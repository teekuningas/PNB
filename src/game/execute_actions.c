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

    Every action now arrives on the channel. The persistent flag struct that stages used to read
    instead is gone: its last field was the swing's phase, and the swing became two declared values.
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
#include "rules_pure/rules_batting_order.h"

#define ANIMATION_FREQUENCY 3

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
    int seat_batter_index; // who to seat as the next batter; -1 when nobody was named this tick
    int swing_angle_declared; // 0 when the batting side said nothing about its aim this tick
    float swing_angle;
    int swing_pass; // the batting side withdrew its swing this tick
} IngestedCommands;

static Permission permit(const MatchSession* match, const GameRulesState* rules, const IntentMessage* message);
static IngestedCommands ingest_intents(MatchSession* match, const GameRulesState* rules, IntentChannels* channels);

void init_execute_actions(MatchSession* match, ClientInputState* clientInput)
{
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
    clientInput->moveWidget.held = 0;
    clientInput->moveWidget.controlIndex = -1;
    clientInput->moveWidget.declared = 0;
    clientInput->moveWidget.framesSinceDeclared = 0;
    clientInput->moveWidget.point = (Vector3D){0};
    clientInput->batterWidget.highlight = 0;
    clientInput->batterWidget.selected = 0;
    clientInput->batterAim = 0.0f;
    clientInput->swingWidget.meter.counter = 0;
    clientInput->swingWidget.meter.counter_max = 0;
    clientInput->swingWidget.meter.dir = 0;
    clientInput->swingWidget.meter.mode = WIDGET_IDLE;
    clientInput->swingWidget.flightFrames = -1;
    clientInput->swingWidget.beat = SWING_BEAT_POWER;
    clientInput->swingWidget.power = 0.0f;

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
static Permission permit(const MatchSession* match, const GameRulesState* rules, const IntentMessage* message)
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

    case INTENT_SELECT_BATTER: {
        // §27, §12, §7 — who may take the bat. The rule itself is batter_seat_verdict, and a client
        // asks the same function to build the list it offers a human; this is the side that does not
        // trust the answer. The gate can hold all of it because every input is settled at the top of
        // the tick: the batting order index and regularOrderSpent are referee-written (a later stage
        // entirely), and a joker's status changes only when one is seated, which is this rule's own
        // consequence.
        //
        // What is NOT here is the seat being physically free — the previous batter can still hold
        // home for some frames after leaving the plate. That is a claim on a body rather than a fact
        // about the message: it holds whether or not anyone declared anything, so it lives with the
        // seating and is asked once, there. Splitting it the other way would have the gate asking a
        // question the actualizer must ask again anyway, and two homes for one rule drift.
        if (match->flowControl.waitingForBatterDecision != 1) {
            return (Permission){0, RULE_BATTER_SEAT_NEEDS_AN_OFFER};
        }
        const int index = message->as.select_batter.index;
        const int battingTeamIndex = get_batting_team_index(&rules->scoreboard);
        const int inTurn = rules->scoreboard.teams[battingTeamIndex]
                               .batterOrder[rules->scoreboard.teams[battingTeamIndex].batterOrderIndex];
        switch (batter_seat_verdict(
            index, inTurn, rules->halfInningState.lastBatter.regularOrderSpent, (int)match->playerInfo[index].bTPI.joker
        )) {
        case SEAT_NOT_IN_BATTING_TURN:
            return (Permission){0, RULE_BATTER_NOT_IN_BATTING_TURN};
        case SEAT_REGULAR_ORDER_SPENT:
            return (Permission){0, RULE_BATTER_ORDER_SPENT};
        case SEAT_JOKER_ALREADY_USED:
            return (Permission){0, RULE_BATTER_JOKER_ALREADY_USED};
        case SEAT_ALLOWED:
        default:
            return (Permission){1, RULE_NONE};
        }
    }

    case INTENT_SWING_POWER:
    case INTENT_SWING_VERTICAL:
    case INTENT_SWING_PASS:
        // The whole of the ordering the deleted phase machine used to carry, in one question asked
        // once against the settled world: is there a pitch to swing at, and is this swing still open?
        //
        // That is the trade the slice is built on. A phased GESTURE is fine; a phase-shaped MESSAGE
        // is not. "Advance my swing to AIMED" means something different applied to a different world,
        // so a producer has to read its own declaration back before daring to send one. A power and
        // an elevation mean the same thing in every world, so what may follow what stops being
        // carried by the messages at all — it becomes state legality, which is what this gate is for.
        //
        // Notice what is NOT asked: whether a power was declared before an elevation. They are
        // independent — the physics says so, power having cancelled out of the elevation law
        // entirely — so an elevation declared alone is a held value that never gets used, not an
        // error. Refusing it would be inventing a rule the game does not have.
        if (!swing_may_be_declared(match)) {
            return (Permission){0, RULE_SWING_NEEDS_A_PITCH};
        }
        return (Permission){1, RULE_NONE};

    case INTENT_SWING_ANGLE:
        // Nothing to refuse. Where a batter may stand on his own arc is not a rule of pesäpallo, and
        // the arc's ends are physical rather than legal — the walk holds them, the way the fielder's
        // mover holds a fence. Whether there is a batter to aim at all is likewise a claim on a body
        // and is asked once, where the walking happens.
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
    return kind == INTENT_BASE_RUN || kind == INTENT_TAKE_FREE_WALK || kind == INTENT_SELECT_BATTER ||
           kind == INTENT_SWING_ANGLE || kind == INTENT_SWING_POWER || kind == INTENT_SWING_VERTICAL ||
           kind == INTENT_SWING_PASS;
}

// Drain one channel into the command block. Same-kind duplicates are last-write-wins — a controller
// that changes its mind within a tick means the later value, and per-base commands never collide
// because each writes its own slot.
static void ingest_channel(
    MatchSession* match, const GameRulesState* rules, IntentChannel* channel, int is_batting_channel,
    IngestedCommands* out
)
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
        // A SELECT_BATTER naming somebody who is not on the batting side is the same class again: a
        // rule can refuse a player, but it cannot refuse a number that is not a player. Flagged here
        // rather than quietly denied in permit() — the two look identical from the outside and mean
        // opposite things — and checked before permission so everything downstream, the gate's own
        // §27 lookup included, can subscript playerInfo without asking twice.
        if (message->kind == INTENT_SELECT_BATTER &&
            (message->as.select_batter.index < 0 || message->as.select_batter.index >= PLAYERS_IN_TEAM + JOKER_COUNT)) {
            channel->malformed = 1;
            continue;
        }

        Permission permission = permit(match, rules, message);
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
        case INTENT_SWING_ANGLE:
            out->swing_angle_declared = 1;
            out->swing_angle = message->as.swing_angle.angle;
            break;
        case INTENT_SWING_POWER:
            // A held value, like a destination and unlike a command: written straight into the engine
            // state that owns it, and kept until the contact frame consumes it or the next batter
            // clears it. That is what makes re-delivering it a no-op — the property rollback needs,
            // and the reason a producer never has to learn whether its last message landed.
            match->pendingActionState.swing.power = message->as.swing_power.power;
            match->pendingActionState.swing.powerActive = 1;
            break;
        case INTENT_SWING_VERTICAL:
            match->pendingActionState.swing.vertical = message->as.swing_vertical.vertical;
            match->pendingActionState.swing.verticalActive = 1;
            break;
        case INTENT_SWING_PASS:
            // A per-tick command and not held state: withdrawing is a thing that HAPPENS, and what it
            // leaves behind (batting_stopped) is the engine's own conclusion rather than the message.
            out->swing_pass = 1;
            break;
        case INTENT_SELECT_BATTER:
            // A per-tick command and not held engine state, unlike the destination below. It can be,
            // because a producer that has chosen restates the choice every frame the decision is
            // open — so "the seat was not free this tick" needs no memory anywhere in the World to
            // survive: the same answer arrives again next tick. The producer's unfinished gesture
            // lives in the producer, which is where §27 puts it too ("Lyömään asettunutta pelaajaa
            // ei voi vaihtaa pois lyömästä" — the choice is the team's until the bat is taken).
            out->seat_batter_index = message->as.select_batter.index;
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
            match->catchingState.controlledMoveTargetFor = match->pII.controlIndex;
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
static IngestedCommands ingest_intents(MatchSession* match, const GameRulesState* rules, IntentChannels* channels)
{
    IngestedCommands commands = {0};
    commands.seat_batter_index = -1; // "nobody was named" is not player 0

    ingest_channel(match, rules, &channels->batting, 1, &commands);
    ingest_channel(match, rules, &channels->catching, 0, &commands);

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
    IngestedCommands commands = ingest_intents(match, rules, channels);

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
    // §27: somebody was named to take the bat. The seating owns the one question the gate left it —
    // whether the seat is physically free — because a batter who has left the plate keeps safety at
    // home for a while yet, and that is true of the body whoever asked for what.
    if (commands.seat_batter_index != -1) {
        seat_batter(match, &rules->referee, fieldPositions, commands.seat_batter_index);
    }
    // The offered free walk was answered this tick — taking it starts the run, declining it just
    // closes the offer. Either answer ends the prompt.
    if (commands.take_free_walk != FREE_WALK_IDLE) {
        take_free_walk_decision(match, &rules->scoreboard, fieldPositions, commands.take_free_walk == FREE_WALK_ACCEPT);
    }

    // baserunners must be able to run!
    for (i = 0; i < BASE_COUNT; i++) {
        base_run(match, &rules->referee, fieldPositions, i, commands.base_run[i]);
    }
    // SWING — the batter's frame. The two declared VALUES are not passed: the gate wrote them into
    // pendingActionState.swing, where they are held until the contact frame consumes them. Only the
    // aim and the withdrawal come through as per-tick commands, because only they are things that
    // HAPPEN; a power and an elevation are things that ARE.
    update_batting(
        match, &rules->referee, fieldPositions, commands.swing_angle_declared, commands.swing_angle,
        commands.swing_pass, playSoundEffect
    );
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
        match->pRAI.meter_value = 0.0f;
    }

    // (There is no batting meter here any more. It used to be copied out of the batter's widget into
    // pRAI so the renderer could find it — a client display value riding in the blittable World, and
    // one that could not say whether there was a cursor at all. The renderer asks the widget itself
    // now, through swing_widget_view. The catching meter above is still on the old route and moves
    // when the pitch widget stops being tied to the engine's declaration.)
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