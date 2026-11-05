#include "actions/batting_system.h"
#include "actions_pure/pitching_physics.h"
#include "field.h"
#include "tick_outputs.h"
#include "rules_state.h"
#include "world.h"
#include "player_motion.h"
#include "runner_paths.h"
#include "fielder_behaviours.h"
#include "vector_math.h"
#include "ball_flight.h"
#include "actions/pitching_system.h"
#include "actions_pure/batting_physics.h"
#include "actions_pure/swing_geometry.h"
#include <math.h>
#include "base_control.h"
#include "rules_pure/player_utils.h"

// How the batter's body takes this swing: a full swing, a bunt, or no swing at all.
typedef enum { BATTING_MODE_SWING = 0, BATTING_MODE_BUNT = 1, BATTING_MODE_STOP = 2 } BattingMode;

// The batting actualizer's own constants (the arc constants live in the header — the aim widget
// nudges its cursor across the same arc at the same rate).
#define GENERIC_BATTER_ADVANCE_SPEED_CONSTANT 0.7f

#define ANIMATION_FREQUENCY 3
#define BAT_ANIMATION_FRAME_HIT_COUNT (20 * ANIMATION_FREQUENCY)
#define BAT_ANIMATION_FRAME_TOTAL_COUNT (34 * ANIMATION_FREQUENCY)
#define BUNT_THRESHOLD 20

#define BUNT_ADVANCE 1.0f
#define SWING_ADVANCE 0.5f
#define SPREAD_ADVANCE 0.3f

#define BATTER_ANGLE_FIX (2 * PI / 4)

void init_batting_system(MatchSession* match)
{
    match->at_bat.flight_frame = 0;
    match->at_bat.flight_clock_running = 0;
    match->at_bat.swing = (SwingActualization){0};
    match->at_bat.swing.contactFrame = -1;
    match->at_bat.arc_angle = 0;
    match->at_bat.step_in_speed = 0;
    match->at_bat.step_in = 0;
    match->at_bat.step_in_limit = 0;
    match->at_bat.swing_stopped = 0;
    match->at_bat.swing_started = 0;
    match->at_bat.pose_dirty = 0;
}

int batter_stands_ready(const AtBat* at_bat)
{
    return at_bat->cycle == BATTING_CYCLE_ARRIVED || at_bat->cycle == BATTING_CYCLE_AWAITING_PITCH;
}

int batting_cycle_is_open(const AtBat* at_bat)
{
    return at_bat->cycle == BATTING_CYCLE_AWAITING_PITCH || at_bat->cycle == BATTING_CYCLE_FACING_PITCH;
}

// §27 — a named player takes the bat ("asettuu lyömään"), if the seat is free.
//
// WHO may be seated was already settled by the INGEST gate against §27, §12 and §7. What is left
// here is the one thing the gate deliberately did not ask: whether the seat is physically free. A
// batter who has set off keeps safety at home until he is established elsewhere or the ball comes
// home, so for some frames — measured at 54 in bug #9 — the plate is empty while its safety is not.
// Seating over that produced a base controlled by one player and occupied by another.
//
// It is a claim on a body, not a fact about the message: it holds whether or not anyone declared
// anything this tick, so putting it at the gate would only mean the same question asked twice. The
// producer is not told to wait and does not need to be — it restates its choice while the decision
// is open, so the seating simply happens on the first tick it can.
void seat_batter(MatchSession* match, const RefereeState* referee, const FieldPositions* fieldPositions, int index)
{
    if (match->offers.batter_seat != 1) return;
    if (get_base_controller(match, referee, BASE_HOME) != -1) return;
    // No range check: an index that is not a player never reaches here, because the drain treats one
    // as a malformed message and fails the frame. A guard here could not fire, and a guard that
    // cannot fire reads exactly like a guard that never has (bug #9).

    Vector3D target;
    // The decision is made and, per §27, is not revisited: "Lyömään asettunutta pelaajaa ei voi
    // vaihtaa pois lyömästä."
    match->offers.batter_seat = 0;

    // new batting team player on the field.
    // has base of zero, is on base and original base is zero too.
    match->playerInfo[index].bTPI.baseId = BASE_HOME;
    match->playerInfo[index].bTPI.state = PLAYER_STATE_AT_BAT;

    // Event-driven: Signal referee that a new batter has entered
    // This triggers strike/ball reset and safety initialization (referee.c)
    match->gameEvents.batterEntered = 1;

    // and they are safe on home base
    // cant advance yet
    match->at_bat.may_leave_home = 0;
    // just set default values so that the player can have a fresh start at
    // the field.
    match->playerRuntime[index].goingForward = 0;
    match->playerRuntime[index].passedPathPoint = 0;
    match->playerRuntime[index].hasMadeRunOnThirdBase = 0;
    // if he is a (unused) joker player, mark him as used.
    // The referee spends the joker (halfInningState.jokersLeft) on the batterEntered event.
    if (match->playerInfo[index].bTPI.joker == JOKER_AVAILABLE) {
        match->playerInfo[index].bTPI.joker = JOKER_USED;
    }
    // move player to default batter ready position
    target.x = (float)(fieldPositions->pitchPlate.x + cos(ZERO_BATTING_ANGLE) * BATTING_RADIUS);
    target.z = (float)(fieldPositions->pitchPlate.z - sin(ZERO_BATTING_ANGLE) * BATTING_RADIUS);
    // move to target can take care of the rest.
    move_to_target(match->playerInfo, index, &target);
}

// Is there a pitch to swing at, and has this swing not already been resolved? The whole of the
// ordering that the deleted phase machine used to carry, as one question over durable world state.
//
// The power window opens at the WINDUP and not at the release, which is what makes the four-beat
// dance legal: the batter loads while the pitcher is still crouching, reading the crouch and nothing
// else. That is information the actualized world makes physical, never a message — the pitcher's
// declared power is not readable from here and must not become so.
int swing_may_be_declared(const MatchSession* match)
{
    if (!batting_cycle_is_open(&match->at_bat)) return 0; // nobody is at the plate swinging
    // There has to be a pitch on its way — the windup or the flight. The two-human soak caught this
    // asked of the old `pitch_state == WINDUP`, which was wrong from the second pitch of every
    // half-inning onward and cost the batter his entire power beat: declarable in 87 of the first
    // windup's 87 frames, and in 0 of 87 on every windup after it.
    if (!pitch_is_being_delivered(match)) return 0;
    if (match->at_bat.swing_stopped == 1) return 0; // withdrawn, or never begun
    // Spent: the bat has already met the ball. Asked of THIS swing's own record of it and not of the
    // referee's batOutcome, which is the correction the sim caught. batOutcome is sticky and
    // survives into the next pitch's windup, so a gate consulting it refused every power a batter
    // tried to commit during a crouch — and let the elevation through once the flight had cleared
    // it. Eleven pitches in twelve went unswung at, and the eleven looked like a batting problem
    // rather than a stale read. The rule that came out of that: a producer-facing gate must not
    // consult state that outlives the play it describes.
    //
    // The actualizer sets this at the instant it fires and reads the same field to refuse a second
    // firing, so the question has ONE answer rather than two that happen to agree.
    if (match->at_bat.swing.consumed) return 0;
    return 1;
}

// The shape the body should be making, given what has been declared so far. Called every frame while
// the batter is live, so a power that arrives after the animation has visibly begun re-shapes it
// mid-motion rather than being ignored — the animation follows the declaration, never gates it.
//
// NOTHING DECLARED IS ITS OWN SHAPE, and it is spread hands rather than a swing. The default used to
// be a full swing, so a batter who had said nothing wound up and swung at the ball anyway, and only
// on the contact frame — after the motion was already committed — did the engine's backstop conclude
// there had been no swing and spread his hands. Read as a gesture it was a lie: the body claimed a
// decision the producer had never made. Read against this function's own first line, it was simply
// wrong — the shape is supposed to follow what has been declared, and what had been declared was
// nothing.
//
// It costs nothing to say so, because this is re-asked every frame: a batter stands ready, and the
// instant a power arrives — during the crouch, or late in the flight — he takes the swing or the
// bunt. The backstop at contact still records the DECISION (swing_stopped, so the referee judges
// the pitch on where it landed rather than on a swing that never came); this only decides what the
// body does while the decision is still open, which is a different question with a different owner.
static BattingMode swing_shape(const MatchSession* match)
{
    if (match->at_bat.swing_stopped == 1) return BATTING_MODE_STOP;
    if (!match->at_bat.swing.powerActive) return BATTING_MODE_STOP;
    if (match->at_bat.swing.power < SWING_BUNT_MAX_POWER) return BATTING_MODE_BUNT;
    return BATTING_MODE_SWING;
}

static int animation_for_shape(BattingMode shape)
{
    if (shape == BATTING_MODE_BUNT) return PLAYER_ANIM_BAT_SWING_2;
    if (shape == BATTING_MODE_STOP) return PLAYER_ANIM_BAT_SWING_3;
    return PLAYER_ANIM_BAT_SWING_1;
}

static float advance_for_shape(BattingMode shape)
{
    if (shape == BATTING_MODE_BUNT) return BUNT_ADVANCE; // the bunt needs the body nearer the ball
    if (shape == BATTING_MODE_STOP) return SPREAD_ADVANCE; // hands spread, barely a step
    return SWING_ADVANCE;
}

// Put the body into a shape: the mode, how far it may step in, and the animation it plays. Three
// values that are all functions of the same one, so they are set in ONE place and never apart.
//
// They used to be set in three, and the third only set two of them — which is how a withdrawal came
// to stop the swing everywhere except on screen. There was a `batting_mode` field holding the shape,
// stop_the_swing assigned it directly, and the re-shape below applied the model only
// `if (shape != batting_mode)` — so it found them already equal and left the bat swinging. The state
// was right, the rules were right, and the player watched himself swing at a ball he had just called
// off.
//
// The field is gone. Nothing ever read it except that guard, so it was a cache of a pure function of
// the declaration that could disagree with the declaration — and the one time it did, it cost the
// withdrawal its whole visible effect. The shape is derived where it is needed and stored nowhere.
static void apply_shape(MatchSession* match, int batterIndex, BattingMode shape)
{
    match->at_bat.step_in_limit = advance_for_shape(shape);
    if (batterIndex != -1) match->playerInfo[batterIndex].cPI.model = animation_for_shape(shape);
}

// Withdraw the swing: the bat is not coming, so there is nothing to miss with. Reached by an explicit
// INTENT_SWING_PASS — the "väärä!" call, which is worth a ball where a swing and a miss is worth a
// strike — or by the engine's own backstop at contact when a producer simply never declared a power.
//
// It records the FACT and nothing else. Everything the body does about it is derived from that fact
// by swing_shape/apply_shape, on this frame and every frame after, which is what stops the two from
// disagreeing about whose job the animation was.
static void stop_the_swing(MatchSession* match)
{
    match->at_bat.swing_stopped = 1;
}

void update_batting(
    MatchSession* match, const RefereeState* referee, const FieldPositions* fieldPositions, int aimDeclared, float aim,
    int passDeclared, int* playSoundEffect
)
{
    int batterIndex = get_active_batter_index(match);

    // ONE SWING PER PITCH, cleared at the START of each batting cycle — before the cycle opens, so
    // before anything can legally be declared into it (the gate requires an open cycle).
    //
    // Keyed here rather than on the arrival, and that is a correction the sim caught: a batter is
    // re-initialised whenever he is put back at the plate, which can happen after a windup has
    // already begun, so a power committed during that windup was wiped by a batter-side reset that
    // had nothing to do with it — eleven pitches in twelve went unswung at. Keying it on "no pitch
    // in progress" instead was worse: it cleared the swing mid-follow-through, and the at-bat never
    // ended. The swing belongs to the batting cycle, so the cycle's own start is where it clears.
    if (!batting_cycle_is_open(&match->at_bat)) {
        match->at_bat.swing = (SwingActualization){0};
        match->at_bat.swing.contactFrame = -1;
    }

    // The cycle opens once the batter has arrived and no pitch is in the air.
    int opens_now = 0;
    if (match->at_bat.cycle == BATTING_CYCLE_ARRIVED && !match->ballInfo.pitch_in_flight) {
        match->at_bat.cycle = BATTING_CYCLE_AWAITING_PITCH;
        opens_now = 1;
    }
    // The batting side withdrew a swing it had already committed a power to — the "väärä!" call,
    // made after the ball is in the air and worth a ball where swinging and missing is worth a
    // strike. Actualized before the frame's own work so the body re-shapes on this very tick.
    if (passDeclared) {
        stop_the_swing(match);
    }
    if (batting_cycle_is_open(&match->at_bat)) {
        // flight_frame counts from the first frame of the cycle that sees the pitch in the air.
        // used for animation and advancing.
        if (match->at_bat.flight_clock_running == 1) {
            match->at_bat.flight_frame += 1;
        }

        // The body is set up on the frame the cycle opens. Nothing can have been declared into it yet: the
        // gate asked swing_may_be_declared at the top of this tick, when the cycle was still shut.
        if (opens_now) {
            // so the default is to swing, we change this to BUNT_ADVANCE or SPREAD_ADVANCE
            // if the circumstances need so
            match->at_bat.step_in_limit = SWING_ADVANCE;
            // we start with natural speeds and positions
            match->at_bat.arc_angle = 0.0f;
            match->at_bat.step_in = 0.0f;
            match->at_bat.step_in_speed = 0.0f;
            // aren't moving yet
            match->at_bat.swing_started = 0;
            // animation is yet to start
            match->at_bat.flight_frame = 0;
            // starting of the animation is yet to start
            match->at_bat.flight_clock_running = 0;
            // not stopped yet
            match->at_bat.swing_stopped = 0;
            // update location and orientation once here
            match->at_bat.pose_dirty = 1;
            if (batterIndex != -1) {
                match->playerInfo[batterIndex].cPI.lastLastLocationUpdate = 1;
            }
        }
        // update batters location and orientation when either advancing speed or angular speed
        // is nonzero. moving is 0 so location wont be updated with other players in different parts of code
        // and orientation is not updated elsewhere either as in the other part of code it is checked
        // if player's index is batterIndex before updating orientation. so its done here exclusively.
        // AIM — an engine behaviour, not a controller and not "AI". A producer says WHERE on the arc
        // it wants to stand and this walks the body there, identically for every producer, exactly
        // as the fielder's mover walks to a destination. It is idempotent by construction: the step
        // is a function of (where he is, where he was sent), so restating the same angle changes
        // nothing and a batter standing on his aim is left alone.
        //
        // The arc's ends are enforced here rather than refused at the gate, because they are a fact
        // about the arc and not a rule of pesäpallo — a producer may ask for anything and simply
        // walks to the end and stands there. Arrival is EXACT: the old key-driven version added a
        // fixed step only while the result stayed strictly inside the limit, so it stopped a step
        // short of the end and, with the AI holding its key down to a target, overshot it by up to a
        // full step. Both of those are the slop the fielder-movement slice found and removed on the
        // other side of the field, and both are doubled by the launch heading.
        if (aimDeclared) {
            float target = aim;
            if (target > BATTER_ANGLE_LIMIT) target = BATTER_ANGLE_LIMIT;
            if (target < -BATTER_ANGLE_LIMIT) target = -BATTER_ANGLE_LIMIT;

            float delta = target - match->at_bat.arc_angle;
            if (delta > BATTER_ANGLE_SPEED_CONSTANT) {
                delta = BATTER_ANGLE_SPEED_CONSTANT;
            } else if (delta < -BATTER_ANGLE_SPEED_CONSTANT) {
                delta = -BATTER_ANGLE_SPEED_CONSTANT;
            }
            if (delta != 0.0f) {
                match->at_bat.arc_angle += delta;
                match->at_bat.pose_dirty = 1;
            }
        }
        // if updated advanced location would be within limit that is originally SWING_ADVANCE but could
        // be changed to BUNT_ADVANCE given small power, then can proceed updating.
        if (match->at_bat.step_in_speed != 0 &&
            match->at_bat.step_in + match->at_bat.step_in_speed < match->at_bat.step_in_limit) {
            match->at_bat.step_in += match->at_bat.step_in_speed;
            match->at_bat.pose_dirty = 1;
        }
        // A batter standing still must have lastLocation caught up to location, or the renderer goes
        // on interpolating across the last step he took — for ever, since nothing moves him again
        // until the next aim. Drawn, that is a body vibrating between two points one arc step apart.
        //
        // Keyed on "he did not move this frame" rather than on a producer's stop, which is what the
        // deleted stop_increase_batter_angle/stop_decrease_batter_angle used to be keyed on. Those
        // fired on a KEY RELEASE, so they missed the case where the key was held into the end of the
        // arc and the angle simply stopped changing — the batting side's version of the fielder
        // running on the spot at a fence. Asking the body instead of the producer covers that, the
        // release, and a producer that just stops declaring (the AI, once the pitch is over).
        //
        // Idempotent: when the two are already equal the catch-up is a copy of a value onto itself.
        // fielder_movement.c does the same thing at its own arrival, for the same reason.
        if (match->at_bat.pose_dirty == 0 && batterIndex != -1) {
            match->playerInfo[batterIndex].cPI.lastLastLocationUpdate = 1;
        }
        // if need for update of location and orientation
        if (match->at_bat.pose_dirty == 1 && batterIndex != -1) {
            float dx;
            float dz;
            float dx2;
            float dz2;
            // update lastLocation for smooth movement
            vec3_set_xz(
                &(match->playerInfo[batterIndex].tPI.lastLocation), match->playerInfo[batterIndex].tPI.location.x,
                match->playerInfo[batterIndex].tPI.location.z
            );
            // update location with sine and cosine to new location on the circle centered at pitch plate.
            // radius will be given by step_in relative to batting radius
            // angle is given by arc_angle and the default ZERO_BATTING_ANGLE
            vec3_set_xz(
                &(match->playerInfo[batterIndex].tPI.location),
                (float)(fieldPositions->pitchPlate.x + cos(ZERO_BATTING_ANGLE + match->at_bat.arc_angle) *
                                                           (BATTING_RADIUS * (1 - match->at_bat.step_in))),
                (float)(fieldPositions->pitchPlate.z - sin(ZERO_BATTING_ANGLE + match->at_bat.arc_angle) *
                                                           (BATTING_RADIUS * (1 - match->at_bat.step_in)))
            );
            // and then set the orientation of batter. here we just first select the base direction to be
            // vector from pitchplate to batter and then fix it a bit to make it look more realistic.
            dx = fieldPositions->pitchPlate.x - match->playerInfo[batterIndex].tPI.location.x;
            dz = fieldPositions->pitchPlate.x - match->playerInfo[batterIndex].tPI.location.z;
            dx2 = (float)(cos(BATTER_ANGLE_FIX) * dx - sin(BATTER_ANGLE_FIX) * dz);
            dz2 = (float)(sin(BATTER_ANGLE_FIX) * dx + cos(BATTER_ANGLE_FIX) * dz);
            vec3_set_xz(&(match->playerInfo[batterIndex].tPI.orientation), dx2, dz2);

            match->at_bat.pose_dirty = 0;
        }

        // so now can actually start thinking about advancing as the ball is in air.
        if (match->ballInfo.pitch_in_flight) {
            // start the animation and advancing.
            if (match->at_bat.flight_clock_running == 0) {
                match->at_bat.flight_clock_running = 1;
            }
            // so at the beginning swing==BAT_ACTION_IDLE. advancing and animation doesnt necessarily start
            // immediately. if pitch is very high the batting animation will take a lot shorter time
            // than what it takes for ball to get down, so the animation and advancing will start a bit
            // later. meter updating on the hand will start immediately and power selection
            // will be available too.
            if (match->at_bat.swing.contactFrame < 0) {
                // The frame the bat meets the ball, solved once from the ball itself: 0 = s + vt +
                // (1/2)at^2 with s = 0, plus an experience-based nudge. Everything the swing is timed
                // against hangs off this one number — the engine's contact test, the animation's
                // start, and (computed independently, from the same physical ball) the human's
                // descent. It is known the moment the ball is in the air and never revised.
                float v = match->ballInfo.velocity.y;
                match->at_bat.swing.contactFrame =
                    calculate_pitch_frame_time(v, GRAVITY, 0.0f, SWING_CONTACT_TWEAK_FRAMES);
                // He is no longer standing ready: the ball is not waited for, it is on its way to him.
                match->at_bat.cycle = BATTING_CYCLE_FACING_PITCH;
            }
            // so if the batter is still not moving, we'll try to figure out if we should be moving.
            if (match->at_bat.swing_started == 0 && batterIndex != -1) {
                // so at the moment there is just enough frames left that if we start animation and advancing now the
                // ball will be at right height for the animation look correct.
                if (match->at_bat.flight_frame > match->at_bat.swing.contactFrame - BAT_ANIMATION_FRAME_HIT_COUNT) {
                    // set swing_started to 1 so that we can better handle starting points of the
                    // animations
                    match->at_bat.swing_started = 1;
                    apply_shape(match, batterIndex, swing_shape(match));
                    match->playerInfo[batterIndex].cPI.animationStage = 0;
                    match->playerInfo[batterIndex].cPI.animationStageCount = 34;
                    match->playerInfo[batterIndex].cPI.animationFrequency = 3;
                    // give player some speed to use for advancing.
                    // basically we are just guessing some reasonable speed.
                    // but it depends on frame hit count so its explicitly there
                    // for clarity
                    match->at_bat.step_in_speed =
                        (float)GENERIC_BATTER_ADVANCE_SPEED_CONSTANT / BAT_ANIMATION_FRAME_HIT_COUNT;
                }
            }
            // The body follows whatever is declared NOW, so a declaration that lands after the swing
            // has visibly started still re-shapes it: a late power turns a swing into a bunt, and a
            // withdrawal turns either into spread hands, mid-motion. The animation is downstream of
            // the declaration and never a gate on it — the same rule the pitch's windup follows.
            //
            // Applied unconditionally rather than only when the shape CHANGES. The guard that used to
            // stand here was comparing against a value a second writer had already moved, so the one
            // case it was meant to serve — a withdrawal, mid-motion — was the exact case it skipped.
            // Re-deriving a shape that has not changed writes the same three values it already holds,
            // which costs nothing and cannot be wrong; a guard that can be fooled about whether
            // something changed can.
            if (match->at_bat.swing_started == 1 && batterIndex != -1) {
                apply_shape(match, batterIndex, swing_shape(match));
            }
        }
        // Everything below is timed against the contact frame, and there is no contact frame until a
        // ball is actually on its way — the negative is "not known yet" and not an early deadline.
        // Without this guard the frame count starts at zero already past it, so the "nobody declared
        // a swing" backstop fires before the pitch exists and no batter ever swings again.
        if (match->at_bat.swing.contactFrame < 0) {
            return;
        }
        // so here we check if the animation has ended and if the cycle is still open,
        // so that we dont do this but once. if it is, we close the cycle
        // and move the player towards ready position agian.
        if ((match->at_bat.flight_frame >
             match->at_bat.swing.contactFrame - BAT_ANIMATION_FRAME_HIT_COUNT + BAT_ANIMATION_FRAME_TOTAL_COUNT) &&
            batting_cycle_is_open(&match->at_bat)) {
            Vector3D target;
            match->at_bat.cycle = BATTING_CYCLE_CLOSED;

            target.x = (float)(fieldPositions->pitchPlate.x + cos(ZERO_BATTING_ANGLE) * BATTING_RADIUS);
            target.z = (float)(fieldPositions->pitchPlate.z - sin(ZERO_BATTING_ANGLE) * BATTING_RADIUS);
            if (batterIndex != -1) {
                move_to_target(match->playerInfo, batterIndex, &target);
            }
        }
        // so here we check if the bat hits. this event happens always the pitch has been in air
        else if (match->at_bat.flight_frame > match->at_bat.swing.contactFrame) {
            // Silence is not a swing. A producer that never declared a power has not swung, so there
            // is nothing here to miss with — no ballMissedByBat, and the referee counts a ball rather
            // than a strike. The human's meter simply loops unpressed and the AI declines outright,
            // so this backstop is what makes "say nothing" a complete and safe answer rather than a
            // hole. It is the same outcome an explicit pass reaches, arrived at without a message.
            if (match->at_bat.swing.powerActive == 0) {
                stop_the_swing(match);
            }
            // Continue only if the batter has not withdrawn and this swing has not already fired.
            // The re-entry guard is the swing's own `consumed`, set on the way in: the condition that
            // brought us here (the animation is past the contact frame) stays true for many frames
            // afterwards, so something has to say "already done". It used to be the referee's sticky
            // batOutcome — legal state read by a physical stage, and legal state that outlives the
            // pitch it describes. Now it is a fact about the swing, with the swing's own lifetime,
            // and it is the same field the INGEST gate refuses on.
            if (match->at_bat.swing_stopped == 0 && match->at_bat.swing.consumed == 0) {
                match->at_bat.swing.consumed = 1;
                // if ball doesnt go too far away to left or right
                if (match->ballInfo.location.x < BALL_MAX_OFFSET && match->ballInfo.location.x > -BALL_MAX_OFFSET) {
                    float verticalAngle;
                    float horizontalAngle;
                    float power;

                    // The three declared values meet the ball here, and nowhere else does the swing
                    // read anything a producer said. A vertical nobody declared is the bottom of the
                    // sweep — the same value a human gets by letting the meter run out, and far
                    // enough off the sweet spot to miss, which is what "swung and never committed"
                    // should mean.
                    verticalAngle = swing_vertical_angle(
                        match->at_bat.swing.verticalActive ? match->at_bat.swing.vertical : 0.0f,
                        match->ballInfo.velocity.y
                    );

                    // 2 to make it possible to bat to every direction on the field and a bit over.
                    horizontalAngle = -match->at_bat.arc_angle * 2;
                    power = match->at_bat.swing.power * SWING_POWER_UNITS;
                    // so now we have the two angles and the power. now we just need to find a way to
                    // convert them nicely to velocities for the ball. there should be other factors
                    // to influence velocity too.
                    if (verticalAngle > VERTICAL_ANGLE_LIMIT || verticalAngle < -VERTICAL_ANGLE_LIMIT) {
                        // we can also just miss, its not so uncommon!
                        match->gameEvents.ballMissedByBat = 1;
                    } else {
                        int powerFactor;
                        Vector3D velocity;

                        // verticalAngle in interval -5..5
                        // power in interval 0..36
                        // horizontalAngle -0.38..0.38
                        // ball location x  -1.0 ... 1.0
                        // power depends on player's power attribute.
                        if (batterIndex != -1) {
                            powerFactor = match->playerInfo[batterIndex].bTPI.power;

                            velocity = calculate_batted_ball_velocity(
                                verticalAngle, horizontalAngle, power, powerFactor, match->ballInfo.location.x
                            );

                            // make the ball fly in the air with new velocity
                            generic_sling_ball(&(match->ballInfo), velocity.x, velocity.y, velocity.z);
                            // Trigger fielder selection update after ball is hit
                            request_chase_replan(match, CHASE_REPLAN_AND_HAND_OVER);
                            // and the sound
                            *playSoundEffect = SOUND_SWING;
                            // bat hits — event for referee to promote
                            match->gameEvents.ballHitByBat = 1;
                            // not a pitch anymore
                            match->ballInfo.pitch_in_flight = 0;
                            // no throw going on now
                            match->ballInfo.thrown_to = BASE_NONE;

                            // move the batter if wanted

                            if (match->runner_commitments.armed[0] == 1) {
                                int base_index = get_base_controller(match, referee, BASE_HOME);
                                match->runner_commitments.armed[0] = 0;
                                if (base_index != -1) {
                                    run_to_next_base(match, fieldPositions, base_index, BASE_HOME);
                                }
                            }
                        }
                    }
                }
                // if the ball went too far away and we still continued our batting
                // we just miss.
                else {
                    match->gameEvents.ballMissedByBat = 1;
                }
            }
        }
    }
}

void prepare_batter(MatchSession* match)
{
    int batterIndex = get_active_batter_index(match);
    if (batterIndex != -1) {
        // batter ready model
        match->playerInfo[batterIndex].cPI.model = PLAYER_ANIM_BATTER_READY;
        // He has arrived: the pitch may come, and the cycle opens (and sets the body up) on the first frame
        // with no pitch in the air. An arrival only begins a cycle when none is open — the batter is walked
        // to the ready spot when a cycle has closed or before the first, and never while one is running.
        if (!batting_cycle_is_open(&match->at_bat)) match->at_bat.cycle = BATTING_CYCLE_ARRIVED;
    }
}
// so here we calculate index and base of the player who is the leadrunner so that
// we can move him if thats the decision.
