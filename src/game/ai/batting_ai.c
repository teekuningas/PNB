#include <math.h>
#include <stdlib.h>

#include "ai/batting_ai.h"
#include "batting_ai_strategy.h"
#include "execute_actions.h"
#include "actions/batting_system.h"
#include "actions/pitching_system.h" // pitch_is_being_delivered — the plan lives exactly as long as the pitch
#include "rng.h"
#include "base_logic.h"
#include "base_control.h"
#include "rules_pure/player_utils.h"

// The batting controller's one-line way of saying "this base, this command". Every run this AI
// declares goes through here, so the message shape is written once rather than at five call sites.
static void declare_run(IntentChannel* channel, BaseID base, RunIntent command)
{
    intent_push(channel, (IntentMessage){.kind = INTENT_BASE_RUN, .as.base_run = {.base = base, .command = command}});
}

// How crowded the bases are, in the three grades the batting strategy distinguishes: 2 with a runner
// on first, 1 with runners only further round, 0 with the bases empty.
static int field_status(const MatchSession* match, const RefereeState* referee)
{
    if (get_base_controller(match, referee, (BaseID)1) != -1) return 2;
    if (get_base_controller(match, referee, (BaseID)2) != -1 || get_base_controller(match, referee, (BaseID)3) != -1)
        return 1;
    return 0;
}

// The batting strategy for the situation as it stands. It is a pure function of durable world state
// and draws nothing random, so it is asked afresh wherever it is needed instead of being remembered:
// a remembered copy is a second thing to keep true, and keeping it true is what once had the engine
// reaching into this controller to clear a "plan calculated" bit after every reset.
static BattingStrategy batting_strategy_now(const MatchSession* match, const GameRulesState* rules)
{
    int batterIndex = get_active_batter_index(match);
    int power = (batterIndex != -1) ? match->playerInfo[batterIndex].bTPI.power : 0;
    int speed = (batterIndex != -1) ? match->playerInfo[batterIndex].bTPI.speed : 0;
    return calculate_batting_strategy(
        &rules->halfInningState, field_status(match, &rules->referee), power, speed, rules->scoreboard.period
    );
}

// §27 gives the list, the strategy picks from it, and the pick is declared as a value. What used to be
// here was a walk: evaluate whoever the engine was showing, and if unhappy, push it along one and look
// again — deliberation carried out IN the world, two frames per step through a pair of click-simulation
// locks, guarded by a cycle detector that a skipped joker slot could slip past (bug #5) and a counter to
// catch it when it did. None of that has anything to say now: there is no cursor to walk and no loop to
// bound.
//
// Restated every frame the decision is open rather than sent once. It costs nothing to repeat — the
// value means the same thing however often it arrives — and it means the AI never has to know that the
// seat is not free until the previous batter has lost home.
static void declare_batter_choice(const MatchSession* match, const GameRulesState* rules, IntentChannel* channel)
{
    int candidates[BATTER_CANDIDATE_MAX];
    const int count = list_batter_candidates(match, &rules->scoreboard, &rules->halfInningState, candidates);
    if (count == 0) return;

    BatterCandidate scored[BATTER_CANDIDATE_MAX];
    for (int c = 0; c < count; c++) {
        scored[c].index = candidates[c];
        scored[c].power = match->playerInfo[candidates[c]].bTPI.power;
        scored[c].speed = match->playerInfo[candidates[c]].bTPI.speed;
    }

    const int choice = choose_batter(scored, count, field_status(match, &rules->referee));
    if (choice != -1) {
        intent_push(channel, (IntentMessage){.kind = INTENT_SELECT_BATTER, .as.select_batter = {.index = choice}});
    }
}

// Before the pitch, with the ball home: arm whoever the strategy sends. Both are self-limiting, so
// neither needs bookkeeping. The batter's command sets will_start_running[BASE_HOME] when it is
// actualized, which makes its guard false next frame; an armed runner (who, on 1st/2nd, leads off) is
// no longer ON_BASE-and-unarmed, so it is not re-commanded.
static void arm_runners_for_the_pitch(const MatchSession* match, const GameRulesState* rules, IntentChannel* channel)
{
    const BattingStrategy strategy = batting_strategy_now(match, rules);
    if (strategy.runBatter == 1 && match->pRAI.will_start_running[BASE_HOME] == 0) {
        declare_run(channel, BASE_HOME, RUN_FORWARD);
    }
    if (strategy.runBaseRunners == 1) {
        for (int i = 1; i < BASE_COUNT; i++) {
            int index = get_base_controller(match, &rules->referee, (BaseID)i);
            if (index != -1 && match->playerInfo[index].bTPI.state == PLAYER_STATE_ON_BASE &&
                match->pRAI.will_start_running[i] == 0) {
                declare_run(channel, (BaseID)i, RUN_FORWARD);
            }
        }
    }
}

// Before the pitch, with the ball away from home: pull any leading 1st/2nd runner back onto its base.
// RUN_BACK on a leading runner makes it run back; once it is no longer LEADING the command stops.
static void recall_leading_runners(const MatchSession* match, const GameRulesState* rules, IntentChannel* channel)
{
    if (batting_strategy_now(match, rules).runBaseRunners != 1) return;
    for (int i = 1; i < 3; i++) {
        int index = get_base_controller(match, &rules->referee, (BaseID)i);
        if (index != -1 && match->playerInfo[index].bTPI.state == PLAYER_STATE_LEADING) {
            declare_run(channel, (BaseID)i, RUN_BACK);
        }
    }
}

// The flight: judge the pitch, and restate the swing for as long as the ball is up.
//
// THE SWING is three complete values, decided once and restated for as long as the pitch lasts. What
// stood here was a click simulation: three copies of the same press-hold-release sequence, one per
// batting style, each timed against the engine's live meter and each sequenced by a lock. That meter
// read was the last one in the game, and the reason the engine had to keep a meter at all.
//
// Restating costs nothing and buys the property the whole design turns on: a value means the same thing
// however often it arrives, so a duplicate is a no-op and a message that goes missing is repaired by the
// next tick rather than by a retry.
static void
play_the_flight(const MatchSession* match, const GameRulesState* rules, AISwingPlan* plan, IntentChannel* channel)
{
    if (plan->wrongPitch == 0 &&
        is_wrong_pitch(match->ballInfo.velocity.x, match->ballInfo.velocity.y, GRAVITY, PLATE_WIDTH)) {
        plan->wrongPitch = 1;
    }

    if (plan->wrongPitch == 1) {
        // Väärä — and nobody can say so until the ball is in the air, which is exactly why the
        // withdrawal exists. Any runner who already broke for the next base is called back, since the
        // pitch is going to be a ball; RUN_BACK turns a forward run into a return, and once it is no
        // longer going forward the command stops.
        for (int i = 1; i < BASE_COUNT; i++) {
            int index = get_base_controller(match, &rules->referee, (BaseID)i);
            if (index != -1 && match->playerRuntime[index].goingForward == 1) {
                declare_run(channel, (BaseID)i, RUN_BACK);
            }
        }
        // The power was committed during the windup; taking it back now is worth a ball, where
        // swinging and missing would be worth a strike. Whether there is a power to take back is the
        // plan's own memory — it was decided, and said, when the swing was open — never the engine's
        // copy of what this controller declared.
        if (plan->decided) {
            intent_push(channel, (IntentMessage){.kind = INTENT_SWING_PASS});
        }
    } else if (plan->decided == 1) {
        // The elevation is the flight's beat: where on the ball the bat meets it, which is a question
        // the batter answers while watching the pitch rather than before it exists.
        intent_push(
            channel, (IntentMessage){.kind = INTENT_SWING_VERTICAL, .as.swing_vertical = {.vertical = plan->vertical}}
        );
    }

    // AIM — declared as the angle itself, restated every frame the pitch is in the air.
    //
    // What was here held a key down until the body reached the decided angle and then released it: a
    // controller reading the world back to decide whether to keep pressing, which meant it could only
    // ever approach from the side it started on and always stopped a step past. The engine now walks the
    // body to the declared angle and arrives on it exactly. The wounding swing's deliberately-unreachable
    // angle still means "as far as this batter can go" — the arc's end is enforced by the walk, so an
    // impossible aim is a bounded one.
    if (plan->decided == 1) {
        intent_push(channel, (IntentMessage){.kind = INTENT_SWING_ANGLE, .as.swing_angle = {.angle = plan->angle}});
    }
}

// COMMIT THE POWER while the pitcher is still crouching — the batter's beat of the dance, and the AI
// takes it on the same terms a human does. What it may read here is the windup itself, which lengthens
// with the toss: the pitcher's decision made physical. It cannot read the pitch, because there is no
// ball yet, so committing is a real commitment — which is what gives a valesyöttö something to cost the
// batter. The whole swing is decided here, once per pitch, and the style it was decided as with it.
//
// Its own beat, deliberately, and not a rider on the pre-pitch arming. That is guarded on the ball being
// HOME, which stops holding the moment the windup lifts the ball onto the plate — so a power declared
// from inside it was declared for a frame or two and then never again, and eleven pitches in twelve went
// unswung at.
//
// "As early as it legally can", not "only during the windup". Normally that IS the windup — the crouch
// is long and the batter's cycle is open throughout it, which is the beat the dance wants. But a windup
// can begin while the previous swing is still following through, and a batter who insisted on the
// windup simply never committed on those pitches: a third of them went unswung at, and it read as a
// batting problem rather than a missed window.
static void commit_swing_power(
    const MatchSession* match, const GameRulesState* rules, AIControllerState* aiController, IntentChannel* channel
)
{
    AISwingPlan* plan = &aiController->swing;
    if (plan->decided == 0) {
        const int style = batting_strategy_now(match, rules).style;
        // Two draws, written as two statements because their ORDER is part of the stream. As arguments
        // to one call they had no order in C at all: gcc evaluated them right to left, so the recorded
        // determinism baseline quietly depended on that compiler. This is the order gcc chose.
        const int randVertical = seeded_rand(&aiController->rngSeed, 201);
        const int randPower = seeded_rand(&aiController->rngSeed, 19);
        SwingDecision decision = decide_swing(style, randPower, randVertical);
        plan->style = style;
        plan->power = decision.power;
        plan->vertical = decision.vertical;
        plan->angle = calculate_ai_batting_angle(style, seeded_rand(&aiController->rngSeed, RAND_MAX));
        plan->decided = 1;
    }
    intent_push(channel, (IntentMessage){.kind = INTENT_SWING_POWER, .as.swing_power = {.power = plan->power}});
}

// Once a hit is safely loose in the field, send every eligible runner to the next base. "Safely loose":
// hit, not caught, nobody holds it, no throw in progress, already bounced, physically outside the field
// and far enough from home that it is not still at the plate (an out-of-bounds ball still in the air is
// a foul — a läpilyönti — not a loose ball).
//
// This is a deliberate "run now", so it declares RUN_COMMIT (the AI's equivalent of a human
// double-press), not RUN_FORWARD, which only arms. Once a runner is goingForward it is not re-issued, so
// there is no double-click simulation to sequence.
static void advance_runners_on_loose_hit(
    const MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions, IntentChannel* channel
)
{
    const BetweenPitchState* bps = &rules->betweenPitchState;
    if (!(bps->batOutcome == BAT_OUTCOME_HIT && bps->catchHasBeenMade == 0 && match->pRAI.throw_going_to_base == -1 &&
          match->pII.hasBallIndex == -1 && match->ballInfo.moving == 1 && bps->hasBallHitGround == 1 &&
          match->ballInfo.location.z < -10.0f && is_ball_out_of_bounds(&match->ballInfo, fieldPositions))) {
        return;
    }
    for (int i = 0; i < BASE_COUNT; i++) {
        int index = get_base_controller(match, &rules->referee, (BaseID)i);
        int shouldRun = 1;
        if (i == 0 && match->pRAI.batter_can_advance == 0) continue;
        if (index == -1 || match->playerRuntime[index].goingForward == 1) continue;
        // don't send a runner into a base interval someone else already occupies.
        for (int j = 0; j < PLAYERS_IN_TEAM + JOKER_COUNT; j++) {
            BaseID bid = match->playerInfo[j].bTPI.baseId;
            if (bid != BASE_NONE && base_to_int_index(bid) == i && j != index) {
                shouldRun = 0;
            }
        }
        if (shouldRun) {
            declare_run(channel, (BaseID)i, RUN_COMMIT);
        }
    }
}

// The batting controller, one tick: the dance's beats in the order they can arise. The first four are
// exclusive — the batter question, the two pre-pitch situations and the flight — and the rest are
// asked every tick.
void update_batting_ai(
    const MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions,
    AIControllerState* aiController, IntentChannel* channel
)
{
    const int airborne = match->pRAI.pitch_state == PITCH_STAGE_AIRBORNE;

    // Take the walk, restated for as long as it is offered: an answer that means the same thing every
    // time it arrives needs no edge and no memory.
    if (match->flowControl.waitingForFreeWalkDecision == 1) {
        intent_push(channel, (IntentMessage){.kind = INTENT_TAKE_FREE_WALK, .as.free_walk = {.accept = 1}});
    }

    // The batter is chosen only once the ball is home, so that in the normal situation the strategy has
    // more of the play's outcome to choose from.
    if (match->flowControl.waitingForBatterDecision == 1 && match->gameFlowState.ballHome == 1) {
        declare_batter_choice(match, rules, channel);
    } else if (match->pRAI.batter_ready == 1 && !airborne && match->gameFlowState.ballHome == 1) {
        arm_runners_for_the_pitch(match, rules, channel);
    } else if (match->pRAI.batter_ready == 1 && !airborne && match->gameFlowState.ballHome == 0) {
        recall_leading_runners(match, rules, channel);
    } else if (airborne) {
        play_the_flight(match, rules, &aiController->swing, channel);
    }

    if (swing_may_be_declared(match)) {
        commit_swing_power(match, rules, aiController, channel);
    }

    // The plan belongs to one pitch, and a pitch spans the WINDUP as well as the flight — the power is
    // committed during the crouch and the elevation while the ball is up. So it is cleared when there is
    // no pitch at all, not merely when the ball is not in the air: an earlier reading wiped the plan on
    // the very frame it was made, and the elevation never arrived.
    //
    // "No pitch at all" is pitch_is_being_delivered — the windup or the flight — the same correction the
    // batter's window needed. Asked of `pitch_state` alone, back when it lied about windups, this cleared
    // the plan on EVERY frame of every windup after a half-inning's first — so the moment that window
    // opened, the controller re-decided its power, elevation and aim afresh each frame, burning three RNG
    // draws a frame and keeping only whatever the last one happened to be.
    //
    // The wrong-pitch judgement goes with it, being about the same pitch. Clearing it is this
    // controller's job alone: the engine used to clear it at release, which was the engine writing into a
    // controller's memory — and the one such write that was actually load-bearing.
    if (!pitch_is_being_delivered(match)) {
        aiController->swing = (AISwingPlan){0};
    }

    advance_runners_on_loose_hit(match, rules, fieldPositions, channel);
}
