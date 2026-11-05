/*
    ingest.c — the INGEST gate, stage 2 of the tick.

    Drain both teams' intent channels — everything the controllers declared during the CONTROL stage at
    the frame top — decide what each message is PERMITTED to do against the world as it stands at the
    top of this tick, and interpret the survivors into engine-shaped commands. Nothing intent-shaped
    survives ingestion: what comes out is a plain per-tick command block, and the channels are empty from
    here to the end of the frame. Every permission in the game is asked here, and every refusal names the
    rule that made it.
*/

#include "ingest.h"
#include "actions/batting_system.h"
#include "common_logic.h"
#include "base_control.h"
#include "rules_pure/player_utils.h"
#include "rules_pure/rules_batting_order.h"

static Permission permit(
    const MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions,
    const IntentMessage* message
);

// The permission rules — the one place a declared intent is told no.
//
// Each answer carries the rule that refused it, not just a bool. Nothing consumes the reason yet; the
// point is that the refusal is not thrown away at the moment it is made, because every later consumer
// (an on-screen hint, a replay that reads back, a rule set that can be asked what it still forbids)
// needs the rule identity and cannot recover it afterwards.
//
// These read the world as it stands at the TOP of the tick, which is the whole reason ingestion runs
// before actualization: every producer's messages are judged against the same settled world, so no
// producer can be advantaged by the engine's internal ordering. Each rule moved here only once the
// frame-top reading was shown to be the one its old site made — measured, not assumed.
//
// The two values that CONTRIBUTE to an action rather than begin one — a pitch's aim, a throw's power —
// are not asked here but after the whole channel's beginnings are known (permit_contribution), because
// what they may contribute to can be begun by another message in the very same tick.
static Permission permit(
    const MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions,
    const IntentMessage* message
)
{
    switch (message->kind) {
    case INTENT_CHANGE_PLAYER:
        // Control follows the ball: while somebody holds it, it is theirs.
        if (match->ballInfo.holder != -1) {
            return (Permission){0, RULE_CHANGE_PLAYER_NEEDS_EMPTY_HANDS};
        }
        return (Permission){1, RULE_NONE};

    case INTENT_DROP_BALL:
        if (match->ballInfo.holder == -1) {
            return (Permission){0, RULE_DROP_NEEDS_BALL_IN_HAND};
        }
        // A gathered throw owns the ball until the engine releases it. (One declared in this same tick
        // is settled after the drain, with the other claims on the ball — see settle_catching_claims.)
        if (match->catching_action.kind == CATCHING_ACTION_THROWING) {
            return (Permission){0, RULE_DROP_NOT_WHILE_THROWING};
        }
        // A pitch has a claim on the ball from the first frame of its windup.
        if (pitch_is_being_delivered(match)) {
            return (Permission){0, RULE_DROP_NOT_WHILE_PITCHING};
        }
        return (Permission){1, RULE_NONE};

    case INTENT_MOVE_TARGET:
        // The one question about this message that the gate can answer: is there anybody to send?
        // Everything else that stops a fielder moving — a throw gathering, a pitch in progress, a
        // throw's recoil still playing — is a physical claim on the fielder's feet that holds no
        // matter who declared what, so it lives with the walking (fielder_movement.c) and is asked
        // once, there. The point itself is never validated: values are trusted, state is checked.
        if (match->catchingState.controlled_fielder == -1) {
            return (Permission){0, RULE_MOVE_NEEDS_A_CONTROLLED_FIELDER};
        }
        return (Permission){1, RULE_NONE};

    case INTENT_TAKE_FREE_WALK:
        // §26: an answer means nothing unless the referee has offered the walk.
        if (match->offers.free_walk != 1) {
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
        if (match->offers.batter_seat != 1) {
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

    case INTENT_PITCH_POWER:
        // The power is what begins a pitch, so this is the pitch's whole permission: the ball at the
        // plate in the pitcher's hand, a batter ready, nothing else under way — and §18(1). Asked once,
        // here, against the world at the top of the tick; the actualizer only acts.
        return pitch_may_begin(match, rules->halfInningState.strikes);

    case INTENT_THROW_TARGET:
        // Likewise the target is what begins a throw.
        return throw_may_begin(match, fieldPositions, message->as.throw_target.base);

    case INTENT_PITCH_AIM:
    case INTENT_THROW_POWER:
        // Unreachable: contributions are asked by permit_contribution, in the drain's second pass.
        return (Permission){0, RULE_NONE};

    case INTENT_BASE_RUN:
        // A command for "the runner on base B" means the runner the producer saw there — the one
        // holding the base at the top of the tick — and if nobody does, there is nobody to command.
        // The gate binds it to that player (ingest_message), the way a destination is bound to the
        // fielder controlled at the top of the tick, so the actualizer moves exactly that player even
        // if something later in the same tick were to change who holds the base. Nothing can: holding a
        // base is the referee's safety plus the player's baseId, the legal world is const for the whole
        // of INGEST and ACTUALIZE, and the only baseId written before base running is a newly seated
        // batter's, who holds no safety until the referee grants it. Measured as well, over 42 golden
        // runs: all 1,656 base-run commands found the same runner at the gate as at the actualizer.
        if (get_base_controller(match, &rules->referee, message->as.base_run.base) == -1) {
            return (Permission){0, RULE_BASE_RUN_NEEDS_A_RUNNER};
        }
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

// The values that add to an action rather than begin one. They are asked last, once everything in the
// channel that could begin an action has been asked, so their answer cannot depend on where they happen
// to sit in the ring.
static int contributes_to_an_action(IntentKind kind)
{
    return kind == INTENT_PITCH_AIM || kind == INTENT_THROW_POWER;
}

// An aim needs a pitch being wound up — one already running, or one this tick begins. A throw's power
// needs a throw gathering, in the same two ways. That is how the AI says a whole pitch in one tick and a
// human says its aim halfway through the windup, and the gate cannot tell which is which.
static Permission
permit_contribution(const MatchSession* match, const IntentMessage* message, const IngestedCommands* out)
{
    const CatchingActionKind running = match->catching_action.kind;
    if (message->kind == INTENT_PITCH_AIM) {
        if (running == CATCHING_ACTION_PITCHING || out->pitch.powerDeclared) return (Permission){1, RULE_NONE};
        return (Permission){0, RULE_PITCH_AIM_NEEDS_A_WINDUP};
    }
    if (running == CATCHING_ACTION_THROWING || out->throw.targetDeclared) return (Permission){1, RULE_NONE};
    return (Permission){0, RULE_THROW_POWER_NEEDS_A_THROW};
}

// A throw, a drop and a pitch all claim the ball-holder's hands, and each was judged against the world
// at the top of the tick — so all three can have been allowed, each on its own. Only one can happen.
// The engine's own actualization order decides — throw, then drop, then pitch, the order actualize
// has always run them in — so the outcome never depends on where the messages sat in the ring. The
// losers are refused: a drop by RULE_DROP_NOT_WHILE_THROWING, a pitch by RULE_ONE_CATCHING_ACTION_PER_TICK.
static void settle_catching_claims(IngestedCommands* out)
{
    if (out->throw.targetDeclared) {
        out->drop_ball = 0;
        out->pitch.powerDeclared = 0;
    } else if (out->drop_ball) {
        out->pitch.powerDeclared = 0;
    }
}

static int message_is_malformed(const IntentMessage* message, int is_batting_channel)
{
    // A kindless message and a message on the wrong team's channel are the same class of thing:
    // not a rule refusing an action, but a producer that is broken. Neither is "denied" — both
    // fail the frame through the state validator, because a rule refusing you is ordinary play
    // and a malformed message never is.
    int belongs_to_batting = intent_belongs_to_batting_team(message->kind) ? 1 : 0;
    if (message->kind == INTENT_NONE || belongs_to_batting != is_batting_channel) return 1;
    // A SELECT_BATTER naming somebody who is not on the batting side is the same class again: a
    // rule can refuse a player, but it cannot refuse a number that is not a player. Flagged here
    // rather than quietly denied in permit() — the two look identical from the outside and mean
    // opposite things — and checked before permission so everything downstream, the gate's own
    // §27 lookup included, can subscript playerInfo without asking twice.
    if (message->kind == INTENT_SELECT_BATTER &&
        (message->as.select_batter.index < 0 || message->as.select_batter.index >= PLAYERS_IN_TEAM + JOKER_COUNT)) {
        return 1;
    }
    // A command for a base that does not exist, a command that is not a command, or a throw at
    // something that is not a base, is the same class again.
    if (message->kind == INTENT_BASE_RUN &&
        (message->as.base_run.base < BASE_HOME || message->as.base_run.base >= BASE_COUNT ||
         message->as.base_run.command < RUN_NONE || message->as.base_run.command > RUN_BACK)) {
        return 1;
    }
    if (message->kind == INTENT_THROW_TARGET &&
        (message->as.throw_target.base < BASE_HOME || message->as.throw_target.base >= BASE_COUNT)) {
        return 1;
    }
    return 0;
}

static void
ingest_message(MatchSession* match, const RefereeState* referee, const IntentMessage* message, IngestedCommands* out);

// Drain one channel into the command block, in two passes: everything that stands on its own or begins
// an action, then — once the channel's competing claims on the ball are settled — the values that
// contribute to an action. Same-kind duplicates are last-write-wins — a controller that changes its mind
// within a tick means the later value, and per-base commands never collide because each writes its own
// slot. The reasons for a refusal die here for now — the hint system is where they will be read.
static void ingest_channel(
    MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions, IntentChannel* channel,
    int is_batting_channel, IngestedCommands* out
)
{
    for (int m = 0; m < channel->count; m++) {
        const IntentMessage* message = &channel->message[m];
        if (message_is_malformed(message, is_batting_channel)) {
            channel->malformed = 1;
            continue;
        }
        if (contributes_to_an_action(message->kind)) continue;
        if (permit(match, rules, fieldPositions, message).allowed) ingest_message(match, &rules->referee, message, out);
    }
    settle_catching_claims(out);
    for (int m = 0; m < channel->count; m++) {
        const IntentMessage* message = &channel->message[m];
        if (message_is_malformed(message, is_batting_channel) || !contributes_to_an_action(message->kind)) continue;
        if (permit_contribution(match, message, out).allowed) ingest_message(match, &rules->referee, message, out);
    }
    channel->count = 0;
}

// Interpret one permitted message into engine terms.
static void
ingest_message(MatchSession* match, const RefereeState* referee, const IntentMessage* message, IngestedCommands* out)
{
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
        match->at_bat.swing.power = message->as.swing_power.power;
        match->at_bat.swing.powerActive = 1;
        break;
    case INTENT_SWING_VERTICAL:
        match->at_bat.swing.vertical = message->as.swing_vertical.vertical;
        match->at_bat.swing.verticalActive = 1;
        break;
    case INTENT_SWING_PASS:
        // A per-tick command and not held state: withdrawing is a thing that HAPPENS, and what it
        // leaves behind (swing_stopped) is the engine's own conclusion rather than the message.
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
        out->base_run[message->as.base_run.base] = message->as.base_run.command;
        out->base_runner[message->as.base_run.base] = get_base_controller(match, referee, message->as.base_run.base);
        break;
    case INTENT_MOVE_TARGET:
        // A destination has a lifetime too, so like the swing's values it is written straight into
        // the engine state that owns it rather than into this tick's command block — and unlike
        // them it is never consumed: it is held until a producer replaces it. That is what lets
        // the fielder resume by itself after the engine interrupts its walk, and what makes
        // re-delivering the same message (a rollback repeating its last input) a no-op.
        match->catchingState.controlledMoveTarget = message->as.move_target.point;
        match->catchingState.controlledMoveTargetActive = 1;
        match->catchingState.controlledMoveTargetFor = match->catchingState.controlled_fielder;
        break;
    case INTENT_PITCH_POWER:
        out->pitch.powerDeclared = 1;
        out->pitch.power = message->as.pitch_power.power;
        break;
    case INTENT_PITCH_AIM:
        out->pitch.aimDeclared = 1;
        out->pitch.direction = message->as.pitch_aim.direction;
        break;
    case INTENT_THROW_TARGET:
        out->throw.targetDeclared = 1;
        out->throw.target = message->as.throw_target.base;
        break;
    case INTENT_THROW_POWER:
        out->throw.powerDeclared = 1;
        out->throw.power = message->as.throw_power.power;
        break;
    case INTENT_NONE:
    default:
        break;
    }
}

// THE INGEST GATE. Both channels are drained here and nowhere else, and they are left empty — which
// is what makes the channel a parameter of the tick rather than a place intent can accumulate. The
// state validator checks that emptiness at the end of every frame.
IngestedCommands ingest_intents(
    MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions, IntentChannels* channels
)
{
    IngestedCommands commands = {0};
    commands.seat_batter_index = -1; // "nobody was named" is not player 0

    ingest_channel(match, rules, fieldPositions, &channels->batting, 1, &commands);
    ingest_channel(match, rules, fieldPositions, &channels->catching, 0, &commands);

    return commands;
}
