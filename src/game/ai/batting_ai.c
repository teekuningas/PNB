#include <math.h>
#include <stdlib.h>

#include "globals.h"
#include "ai/batting_ai.h"
#include "batting_ai_strategy.h"
#include "execute_actions.h"
#include "actions/batting_system.h"
#include "actions_pure/swing_geometry.h"
#include "game_manipulation.h"
#include "rng.h"
#include "base_logic.h"
#include "base_control.h"
#include "rules_pure/player_utils.h"

void init_batting_ai(AIState* aiState)
{
    aiState->battingStyle = 0;
    aiState->runningBatter = 0;
    aiState->runningBaseRunners = 0;

    aiState->swingDecided = 0;
    aiState->decidedAngle = 0.0f;
    aiState->decidedPower = 0.0f;
    aiState->decidedVertical = SWING_VERTICAL_FOCAL;
    aiState->aiWrongPitch = 0;
    aiState->planCalculated = 0;
}

// The batting controller's one-line way of saying "this base, this command". Every run this AI
// declares goes through here, so the message shape is written once rather than at five call sites.
static void declare_run(IntentChannel* channel, BaseID base, RunIntent command)
{
    intent_push(channel, (IntentMessage){.kind = INTENT_BASE_RUN, .as.base_run = {.base = base, .command = command}});
}

void update_batting_ai(
    MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions,
    AIControllerState* aiController, IntentChannel* channel
)
{
    int i;
    int okToAdvanceAfterHit = 0;

    // No lock cleanup, because there are no locks. Both self-heals that used to stand here existed
    // to unstick a click simulation from a state it had been carried out of — an at-bat that ended
    // mid-swing, a free-walk offer that closed while the answer was being "held". Without the first
    // of them the AI batting team deadlocked the whole game. A controller that declares values and
    // restates them has nothing that can be left half-done, so there is nothing to unstick.

    // A fresh pitch cycle forces the swing plan to be recomputed. The per-base run decisions
    // need no bookkeeping reset: the AI re-derives them every frame from live game state
    // (will_start_running + each runner's PlayerUnitState), so they cannot go stale.
    if (match->pRAI.batter_ready == 0 && match->aiState.planCalculated == 1) {
        match->aiState.planCalculated = 0;
    }
    // make free walk decision == accept
    // Take the walk, restated for as long as it is offered. The click simulation that used to wrap
    // this — press, hold a lock, release — outlived the slice that made the answer a message: an
    // answer that means the same thing every time it arrives needs no edge and no memory.
    if (match->flowControl.waitingForFreeWalkDecision == 1) {
        intent_push(channel, (IntentMessage){.kind = INTENT_TAKE_FREE_WALK, .as.free_walk = {.accept = 1}});
    }
    // we decide batter only after ball is at home so that in normal situation ai will have more information
    // to make its strategy decisions
    if (match->flowControl.waitingForBatterDecision == 1 && match->gameFlowState.ballHome == 1) {
        // §27 gives the list, the strategy picks from it, and the pick is declared as a value. What
        // used to be here was a walk: evaluate whoever the engine was showing, and if unhappy, push
        // it along one and look again — deliberation carried out IN the world, two frames per step
        // through a pair of click-simulation locks, guarded by a cycle detector that a skipped joker
        // slot could slip past (bug #5) and a counter to catch it when it did. None of that has
        // anything to say now: there is no cursor to walk and no loop to bound.
        //
        // Restated every frame the decision is open rather than sent once. It costs nothing to
        // repeat — the value means the same thing however often it arrives — and it means the AI
        // never has to know that the seat is not free until the previous batter has lost home.
        int candidates[BATTER_CANDIDATE_MAX];
        const int count = list_batter_candidates(match, &rules->scoreboard, &rules->halfInningState, candidates);
        if (count > 0) {
            int firstBaseIndex = get_base_controller(match, &rules->referee, (BaseID)1);
            int secondBaseIndex = get_base_controller(match, &rules->referee, (BaseID)2);
            int thirdBaseIndex = get_base_controller(match, &rules->referee, (BaseID)3);
            int fieldStatus;

            if (firstBaseIndex != -1)
                fieldStatus = 2;
            else if (secondBaseIndex != -1 || thirdBaseIndex != -1)
                fieldStatus = 1;
            else
                fieldStatus = 0;

            BatterCandidate scored[BATTER_CANDIDATE_MAX];
            for (int c = 0; c < count; c++) {
                scored[c].index = candidates[c];
                scored[c].power = match->playerInfo[candidates[c]].bTPI.power;
                scored[c].speed = match->playerInfo[candidates[c]].bTPI.speed;
            }

            const int choice = choose_batter(scored, count, fieldStatus);
            if (choice != -1) {
                intent_push(
                    channel, (IntentMessage){.kind = INTENT_SELECT_BATTER, .as.select_batter = {.index = choice}}
                );
            }
        }

    } else if (match->pRAI.batter_ready == 1 && match->pRAI.pitch_state != PITCH_STAGE_AIRBORNE &&
               match->gameFlowState.ballHome == 1) {
        // decision tree.. contents can be read within
        if (match->aiState.planCalculated == 0) {
            int batterIndex = get_active_batter_index(match);
            int firstBaseIndex = get_base_controller(match, &rules->referee, (BaseID)1);
            int secondBaseIndex = get_base_controller(match, &rules->referee, (BaseID)2);
            int thirdBaseIndex = get_base_controller(match, &rules->referee, (BaseID)3);
            int power = match->playerInfo[batterIndex].bTPI.power;
            int speed = match->playerInfo[batterIndex].bTPI.speed;
            int fieldStatus;

            if (firstBaseIndex != -1)
                fieldStatus = 2;
            else if (secondBaseIndex != -1 || thirdBaseIndex != -1)
                fieldStatus = 1;
            else
                fieldStatus = 0;

            BattingStrategy strategy = calculate_batting_strategy(
                &(rules->halfInningState), fieldStatus, power, speed, rules->scoreboard.period
            );

            match->aiState.battingStyle = strategy.style;
            match->aiState.runningBaseRunners = strategy.runBaseRunners;
            match->aiState.runningBatter = strategy.runBatter;

            match->aiState.planCalculated = 1;
        }
        // Arm the batter to advance — the run is committed on contact. Declared once: the
        // command's actualization sets will_start_running[BASE_HOME], which makes the guard
        // below false next frame (no click bookkeeping needed).
        if (match->aiState.runningBatter == 1 && match->pRAI.will_start_running[BASE_HOME] == 0) {
            declare_run(channel, BASE_HOME, RUN_FORWARD);
        }
        // Arm on-base runners to advance on the pitch. Same self-limiting: once a runner is
        // armed (and, on 1st/2nd, leads off) it is no longer ON_BASE-and-unarmed, so it is not
        // re-commanded.
        if (match->aiState.runningBaseRunners == 1) {
            for (i = 1; i < BASE_COUNT; i++) {
                int index = get_base_controller(match, &rules->referee, (BaseID)i);
                if (index != -1 && match->playerInfo[index].bTPI.state == PLAYER_STATE_ON_BASE &&
                    match->pRAI.will_start_running[i] == 0) {
                    declare_run(channel, (BaseID)i, RUN_FORWARD);
                }
            }
        }
    }
    // if ball is not home, we return players from first and second base to their bases
    else if (match->pRAI.batter_ready == 1 && match->pRAI.pitch_state != PITCH_STAGE_AIRBORNE &&
             match->gameFlowState.ballHome == 0) {
        // The ball came back home with no pitch in the air: pull any leading 1st/2nd runners
        // back onto their base. RUN_BACK on a leading runner makes it run back; once it is no
        // longer LEADING the command stops being issued.
        if (match->aiState.runningBaseRunners == 1) {
            for (i = 1; i < 3; i++) {
                int index = get_base_controller(match, &rules->referee, (BaseID)i);
                if (index != -1 && match->playerInfo[index].bTPI.state == PLAYER_STATE_LEADING) {
                    declare_run(channel, (BaseID)i, RUN_BACK);
                }
            }
        }
    }
    // COMMIT THE POWER while the pitcher is still crouching — the batter's beat of the dance, and the
    // AI takes it on the same terms a human does. What it may read here is the windup itself, which
    // lengthens with the toss: the pitcher's decision made physical. It cannot read the pitch,
    // because there is no ball yet, so committing is a real commitment — which is what gives a
    // valesyöttö something to cost the batter.
    //
    // Its own beat, deliberately, and not a rider on the planning branch above. That branch is
    // guarded on the ball being HOME, which stops holding the moment the windup lifts the ball onto
    // the plate — so a power declared from inside it was declared for a frame or two and then never
    // again, and eleven pitches in twelve went unswung at.
    // and here we bat
    else if (match->pRAI.pitch_state == PITCH_STAGE_AIRBORNE) {
        int i;
        // predict if pitch is going to be ball
        if (match->aiState.aiWrongPitch == 0 &&
            is_wrong_pitch(match->ballInfo.velocity.x, match->ballInfo.velocity.y, GRAVITY, PLATE_WIDTH)) {
            match->aiState.aiWrongPitch = 1;
        }
        if (match->aiState.aiWrongPitch == 1) {
            // The batter is handled below; here we abort any runner who already broke for the
            // next base, since the pitch is going to be a ball. RUN_BACK turns a forward run
            // into a return; once it is no longer going forward the command stops.
            for (i = 1; i < BASE_COUNT; i++) {
                int index = get_base_controller(match, &rules->referee, (BaseID)i);
                if (index != -1 && match->playerRuntime[index].goingForward == 1) {
                    declare_run(channel, (BaseID)i, RUN_BACK);
                }
            }
        }
        // THE SWING — three complete values, decided once and restated for as long as the pitch
        // lasts. What stood here was a click simulation: three copies of the same press-hold-release
        // sequence, one per batting style, each timed against the engine's live meter and each
        // sequenced by a lock. That meter read was the last one in the game, and the reason the
        // engine had to keep a meter at all.
        //
        // Restating costs nothing and buys the property the whole design turns on: a value means the
        // same thing however often it arrives, so a duplicate is a no-op and a message that goes
        // missing is repaired by the next tick rather than by a retry.
        if (match->aiState.aiWrongPitch == 1) {
            // Väärä — and nobody can say so until the ball is in the air, which is exactly why the
            // withdrawal exists. The power was committed during the windup; taking it back now is
            // worth a ball, where swinging and missing would be worth a strike.
            if (match->pendingActionState.swing.powerActive) {
                intent_push(channel, (IntentMessage){.kind = INTENT_SWING_PASS});
            }
        } else if (match->aiState.swingDecided == 1) {
            // The elevation is the flight's beat: where on the ball the bat meets it, which is a
            // question the batter answers while watching the pitch rather than before it exists.
            intent_push(
                channel, (IntentMessage){.kind = INTENT_SWING_VERTICAL,
                                         .as.swing_vertical = {.vertical = match->aiState.decidedVertical}}
            );
        }

        // AIM — declared as the angle itself, restated every frame the pitch is in the air.
        //
        // What was here held a key down until the body reached the decided angle and then released
        // it: a controller reading the world back to decide whether to keep pressing, which meant it
        // could only ever approach from the side it started on and always stopped a step past. The
        // engine now walks the body to the declared angle and arrives on it exactly. The wounding
        // swing's deliberately-unreachable angle still means "as far as this batter can go" — the
        // arc's end is enforced by the walk, so an impossible aim is a bounded one.
        if (match->aiState.swingDecided == 1) {
            intent_push(
                channel,
                (IntentMessage){.kind = INTENT_SWING_ANGLE, .as.swing_angle = {.angle = match->aiState.decidedAngle}}
            );
        }
    }
    // "As early as it legally can", not "only during the windup". Normally that IS the windup — the
    // crouch is long and the batter's cycle is open throughout it, which is the beat the dance wants.
    // But a windup can begin while the previous swing is still following through, and a batter who
    // insisted on the windup simply never committed on those pitches: a third of them went unswung
    // at, and it read as a batting problem rather than a missed window.
    if (swing_may_be_declared(match)) {
        if (match->aiState.swingDecided == 0) {
            SwingDecision decision = decide_swing(
                match->aiState.battingStyle, seeded_rand(&aiController->rngSeed, 19),
                seeded_rand(&aiController->rngSeed, 201)
            );
            match->aiState.decidedPower = decision.power;
            match->aiState.decidedVertical = decision.vertical;
            match->aiState.decidedAngle =
                calculate_ai_batting_angle(match->aiState.battingStyle, seeded_rand(&aiController->rngSeed, RAND_MAX));
            match->aiState.swingDecided = 1;
        }
        intent_push(
            channel,
            (IntentMessage){.kind = INTENT_SWING_POWER, .as.swing_power = {.power = match->aiState.decidedPower}}
        );
    }

    // The plan belongs to one pitch, and a pitch now spans the WINDUP as well as the flight — the
    // power is committed during the crouch and the elevation while the ball is up. So it is cleared
    // when there is no pitch at all, not merely when the ball is not in the air: the earlier reading
    // wiped the plan on the very frame it was made, and the elevation never arrived.
    if (match->pRAI.pitch_state == PITCH_STAGE_NONE && match->aiState.swingDecided == 1) {
        match->aiState.swingDecided = 0;
    }
    // AI: Check if it's safe to advance runners
    // Ball was hit, not caught, no one has it, no throw in progress, and ball is physically outside field
    // AND ball has traveled far enough from home (to avoid triggering when ball is still at home plate)
    // AND ball has already hit the ground (if still airborne and out of bounds, it's a foul/läpilyönti)
    if (rules->betweenPitchState.batOutcome == BAT_OUTCOME_HIT && rules->betweenPitchState.catchHasBeenMade == 0 &&
        match->pRAI.throw_going_to_base == -1 && match->pII.hasBallIndex == -1 && match->ballInfo.moving == 1 &&
        rules->betweenPitchState.hasBallHitGround == 1 && match->ballInfo.location.z < -10.0f &&
        is_ball_out_of_bounds(&match->ballInfo, fieldPositions)) {
        okToAdvanceAfterHit = 1;
    }
    // Once the hit is safely loose in the field, send every eligible runner to the next base.
    // This is a deliberate "run now", so we declare RUN_COMMIT (the AI's equivalent of a human
    // double-press) — NOT RUN_FORWARD, which only arms. Once a runner is goingForward we stop
    // re-issuing, so the old double-click simulation (amountOfClicks / clickBreak / locks) is gone.
    if (okToAdvanceAfterHit) {
        for (i = 0; i < BASE_COUNT; i++) {
            int j;
            int index = get_base_controller(match, &rules->referee, (BaseID)i);
            int shouldRun = 1;
            if (i == 0 && match->pRAI.batter_can_advance == 0) continue;
            if (index == -1 || match->playerRuntime[index].goingForward == 1) continue;
            // don't send a runner into a base interval someone else already occupies.
            for (j = 0; j < PLAYERS_IN_TEAM + JOKER_COUNT; j++) {
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
}
