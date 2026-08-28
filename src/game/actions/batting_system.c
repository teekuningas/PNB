#include "actions/batting_system.h"
#include "common_logic.h"
#include "vector_math.h"
#include "execute_actions.h"
#include "actions/pitching_system.h"
#include "actions_pure/batting_physics.h"
#include <math.h>
#include "base_logic.h"
#include "base_control.h"
#include "rules_pure/player_utils.h"

// Macros moved from execute_actions.c (the arc constants live in the header — the aim widget
// nudges its cursor across the same arc at the same rate).
#define GENERIC_BATTER_ADVANCE_SPEED_CONSTANT 0.7f

#define PITCH_FRAME_TIME_TWEAK 3
#define ANIMATION_FREQUENCY 3
#define BAT_ANIMATION_FRAME_HIT_COUNT (20 * ANIMATION_FREQUENCY)
#define BAT_ANIMATION_FRAME_TOTAL_COUNT (34 * ANIMATION_FREQUENCY)
#define BALL_MAX_OFFSET 1.0f
#define BUNT_THRESHOLD 20

#define BUNT_ADVANCE 1.0f
#define SWING_ADVANCE 0.5f
#define SPREAD_ADVANCE 0.3f

#define BATTER_ANGLE_FIX (2 * PI / 4)

void init_batting_system(MatchSession* match)
{
    match->pendingActionState.batting_frame_count = 0;
    match->pendingActionState.increase_batting_frame_count = 0;
    match->pendingActionState.selected_batting_power_count = 0;
    match->pendingActionState.selected_batting_angle_count = 0;
    match->pendingActionState.batter_angle = 0;
    match->pendingActionState.batter_advance_speed = 0;
    match->pendingActionState.batter_advance = 0;
    match->pendingActionState.batting_mode = BATTING_MODE_SWING;
    match->pendingActionState.batter_advance_limit = 0;
    match->pendingActionState.batting_stopped = 0;
    match->pendingActionState.batter_moving = 0;
    match->pendingActionState.update_batter_location_and_orientation = 0;
    match->pendingActionState.pitch_frame_time = 0;
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
    if (match->flowControl.waitingForBatterDecision != 1) return;
    if (get_base_controller(match, referee, BASE_HOME) != -1) return;
    // No range check: an index that is not a player never reaches here, because the drain treats one
    // as a malformed message and fails the frame. A guard here could not fire, and a guard that
    // cannot fire reads exactly like a guard that never has (bug #9).

    Vector3D target;
    // The decision is made and, per §27, is not revisited: "Lyömään asettunutta pelaajaa ei voi
    // vaihtaa pois lyömästä."
    match->flowControl.waitingForBatterDecision = 0;

    // new batting team player on the field.
    // has base of zero, is on base and original base is zero too.
    match->playerInfo[index].bTPI.baseId = BASE_HOME;
    match->playerInfo[index].bTPI.state = PLAYER_STATE_AT_BAT;

    // Event-driven: Signal referee that a new batter has entered
    // This triggers strike/ball reset and safety initialization (referee.c)
    match->gameEvents.batterEntered = 1;

    // and they are safe on home base
    // cant advance yet
    match->pRAI.batter_can_advance = 0;
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

// so this function is called when we decide the power
void select_power(MatchSession* match)
{
    // swing is set to BAT_ACTION_ANGLE_WAIT so that the meter indicator on the screen can start decreasing etc.
    match->aF.bTAF.swing = BAT_ACTION_ANGLE_WAIT;
    // we select meter_counter value as the power but we move it a bit so that the smallest value isnt 4/13 but instead
    // 0.
    match->pendingActionState.selected_batting_power_count =
        match->pendingActionState.meter_counter - (BAT_SWING_MAX - BAT_LOAD_MAX);
    // and then for angle we start again from zero and go to the max and later on we'll scale it a bit to look nice on
    // the screen. this is used so that more power we use, faster will the indicator come down when trying too select
    // the angle.
    match->pendingActionState.meter_counter = 0;
    match->pendingActionState.meter_counter_max = BAT_SWING_MAX;

    // if power is small, we use bunting animation.
    if (match->pendingActionState.selected_batting_power_count < BUNT_THRESHOLD) {
        // and its updated here only if the player is moving already so that the animation wont start too early.
        if (match->pendingActionState.batter_moving == 1) {
            int batterIndex = get_active_batter_index(match);
            if (batterIndex != -1) {
                match->playerInfo[batterIndex].cPI.model = PLAYER_ANIM_BAT_SWING_2;
            }
        }
        // will allow us to move bit further when bunting as the model needs to be closer to ball to look nice.
        match->pendingActionState.batter_advance_limit = BUNT_ADVANCE;
        // this will take care that if we indeed were too early for the animation to start yet, the correct animation
        // will start when the batter starts to move.
        match->pendingActionState.batting_mode = BATTING_MODE_BUNT;
    }
    // if power is great enough to give us good swing we'll go with it but thats the default so no need to do anything
    // here.
}
void select_angle(MatchSession* match)
{
    // simple enough, enter the state of waiting for animation to end
    match->aF.bTAF.swing = BAT_ACTION_DONE;
    // and set angle to be the meter_counter value, its processed further afterwards.
    match->pendingActionState.selected_batting_angle_count = match->pendingActionState.meter_counter;
}

void update_batting(
    MatchSession* match, const RefereeState* referee, const BetweenPitchState* betweenPitchState,
    const FieldPositions* fieldPositions, int aimDeclared, float aim, int* playSoundEffect
)
{
    int batterIndex = get_active_batter_index(match);
    if (match->pRAI.batter_ready == 1 && match->pRAI.pitch_state != PITCH_STAGE_AIRBORNE &&
        match->pRAI.batting_going_on == 0) {
        match->pRAI.batting_going_on = 1;
    }
    // so batting_going_on goes 1 when batter arrives to its ready position and pitch is not in air.
    if (match->pRAI.batting_going_on == 1) {
        // we increase batting_frame_count from the moment the batter movement started.
        // used for animation and advancing.
        if (match->pendingActionState.increase_batting_frame_count == 1) {
            match->pendingActionState.batting_frame_count += 1;
        }

        if (match->pRAI.init_batter == 1) {
            // so the default is to swing, we change this to BUNT_ADVANCE or SPREAD_ADVANCE
            // if the circumstances need so
            match->pendingActionState.batter_advance_limit = SWING_ADVANCE;
            // we start with natural speeds and positions
            match->pendingActionState.batter_angle = 0.0f;
            match->pendingActionState.batter_advance = 0.0f;
            match->pendingActionState.batter_advance_speed = 0.0f;
            // aren't moving yet
            match->pendingActionState.batter_moving = 0;
            // swinging mode
            match->pendingActionState.batting_mode = BATTING_MODE_SWING;
            // animation is yet to start
            match->pendingActionState.batting_frame_count = 0;
            // starting of the animation is yet to start
            match->pendingActionState.increase_batting_frame_count = 0;
            // not stopped yet
            match->pendingActionState.batting_stopped = 0;
            // update location and orientation once here
            match->pendingActionState.update_batter_location_and_orientation = 1;
            if (batterIndex != -1) {
                match->playerInfo[batterIndex].cPI.lastLastLocationUpdate = 1;
            }
            // init done, so no need to do that anymore.
            match->pRAI.init_batter = 0;
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

            float delta = target - match->pendingActionState.batter_angle;
            if (delta > BATTER_ANGLE_SPEED_CONSTANT) {
                delta = BATTER_ANGLE_SPEED_CONSTANT;
            } else if (delta < -BATTER_ANGLE_SPEED_CONSTANT) {
                delta = -BATTER_ANGLE_SPEED_CONSTANT;
            }
            if (delta != 0.0f) {
                match->pendingActionState.batter_angle += delta;
                match->pendingActionState.update_batter_location_and_orientation = 1;
            }
        }
        // if updated advanced location would be within limit that is originally SWING_ADVANCE but could
        // be changed to BUNT_ADVANCE given small power, then can proceed updating.
        if (match->pendingActionState.batter_advance_speed != 0 &&
            match->pendingActionState.batter_advance + match->pendingActionState.batter_advance_speed <
                match->pendingActionState.batter_advance_limit) {
            match->pendingActionState.batter_advance += match->pendingActionState.batter_advance_speed;
            match->pendingActionState.update_batter_location_and_orientation = 1;
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
        if (match->pendingActionState.update_batter_location_and_orientation == 0 && batterIndex != -1) {
            match->playerInfo[batterIndex].cPI.lastLastLocationUpdate = 1;
        }
        // if need for update of location and orientation
        if (match->pendingActionState.update_batter_location_and_orientation == 1 && batterIndex != -1) {
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
            // radius will be given by batter_advance relative to batting radius
            // angle is given by batter_angle and the default ZERO_BATTING_ANGLE
            vec3_set_xz(
                &(match->playerInfo[batterIndex].tPI.location),
                (float)(fieldPositions->pitchPlate.x +
                        cos(ZERO_BATTING_ANGLE + match->pendingActionState.batter_angle) *
                            (BATTING_RADIUS * (1 - match->pendingActionState.batter_advance))),
                (float)(fieldPositions->pitchPlate.z -
                        sin(ZERO_BATTING_ANGLE + match->pendingActionState.batter_angle) *
                            (BATTING_RADIUS * (1 - match->pendingActionState.batter_advance)))
            );
            // and then set the orientation of batter. here we just first select the base direction to be
            // vector from pitchplate to batter and then fix it a bit to make it look more realistic.
            dx = fieldPositions->pitchPlate.x - match->playerInfo[batterIndex].tPI.location.x;
            dz = fieldPositions->pitchPlate.x - match->playerInfo[batterIndex].tPI.location.z;
            dx2 = (float)(cos(BATTER_ANGLE_FIX) * dx - sin(BATTER_ANGLE_FIX) * dz);
            dz2 = (float)(sin(BATTER_ANGLE_FIX) * dx + cos(BATTER_ANGLE_FIX) * dz);
            vec3_set_xz(&(match->playerInfo[batterIndex].tPI.orientation), dx2, dz2);

            match->pendingActionState.update_batter_location_and_orientation = 0;
        }

        // so now can actually start thinking about advancing as the ball is in air.
        if (match->pRAI.pitch_state == PITCH_STAGE_AIRBORNE && match->pendingActionState.run_bat_flag == 0) {
            // start the animation and advancing.
            if (match->pendingActionState.increase_batting_frame_count == 0) {
                match->pendingActionState.increase_batting_frame_count = 1;
            }
            // so at the beginning swing==BAT_ACTION_IDLE. advancing and animation doesnt necessarily start
            // immediately. if pitch is very high the batting animation will take a lot shorter time
            // than what it takes for ball to get down, so the animation and advancing will start a bit
            // later. meter updating on the hand will start immediately and power selection
            // will be available too.
            if (match->aF.bTAF.swing == BAT_ACTION_IDLE) {
                float v = match->ballInfo.velocity.y;
                // note decision s=0 makes the landing point actually to be in air,
                // but thats convenient for our purposes. so here we count
                // how many frames will it take for ball to go up and down again so that we can try
                // to time our batting advancing and animation accordingly. just solve 0 = s + vt + (1/2)at^2
                // and choose the correct branch and then add a little experience-based tweak.
                match->pendingActionState.pitch_frame_time =
                    calculate_pitch_frame_time(v, GRAVITY, 0.0f, PITCH_FRAME_TIME_TWEAK);
                // Here initialize meter_counter and meter_counter max in a way similar to how we initialized those in
                // pitching. relative distance from the end of meter to the indicator is the same. difference is that
                // these values are scaled a bit, to allow as slow movement of the indicator as possible for batting.
                match->pendingActionState.meter_counter = BAT_SWING_MAX - BAT_LOAD_MAX;
                match->pendingActionState.meter_counter_max = BAT_SWING_MAX;
                // so allow user to select power
                match->aF.bTAF.swing = BAT_ACTION_WAIT_FOR_BALL;
                // and batter_ready is zero now. batter isnt ready to action
                // anymore, action is with him already.
                match->pRAI.batter_ready = 0;
            }
            // so if the batter is still not moving, we'll try to figure out if we should be moving.
            if (match->pendingActionState.batter_moving == 0 && batterIndex != -1) {
                // so at the moment there is just enough frames left that if we start animation and advancing now the
                // ball will be at right height for the animation look correct.
                if (match->pendingActionState.batting_frame_count >
                    match->pendingActionState.pitch_frame_time - BAT_ANIMATION_FRAME_HIT_COUNT) {
                    // set batter_moving flag to 1 so that we can better handle starting points of the
                    // animations
                    match->pendingActionState.batter_moving = 1;
                    // so if we are still to swing
                    // select corresponding animation
                    if (match->pendingActionState.batting_mode == BATTING_MODE_SWING) {
                        match->playerInfo[batterIndex].cPI.model = PLAYER_ANIM_BAT_SWING_1;
                    }
                    // to bunt, select bunting animation
                    else if (match->pendingActionState.batting_mode == BATTING_MODE_BUNT) {
                        match->playerInfo[batterIndex].cPI.model = PLAYER_ANIM_BAT_SWING_2;
                    }
                    // to stop the batting select the hands spread -animation
                    else {
                        match->playerInfo[batterIndex].cPI.model = PLAYER_ANIM_BAT_SWING_3;
                    }
                    match->playerInfo[batterIndex].cPI.animationStage = 0;
                    match->playerInfo[batterIndex].cPI.animationStageCount = 34;
                    match->playerInfo[batterIndex].cPI.animationFrequency = 3;
                    // give player some speed to use for advancing.
                    // basically we are just guessing some reasonable speed.
                    // but it depends on frame hit count so its explicitly there
                    // for clarity
                    match->pendingActionState.batter_advance_speed =
                        (float)GENERIC_BATTER_ADVANCE_SPEED_CONSTANT / BAT_ANIMATION_FRAME_HIT_COUNT;
                }
            }
            // if meter indicator reaches the limit, any batting won't happen and
            // we'll just show the animation of player spreading hands.
            if (match->aF.bTAF.swing == BAT_ACTION_WAIT_FOR_BALL) {
                if (match->pendingActionState.meter_counter - (BAT_SWING_MAX - BAT_LOAD_MAX) >= BAT_LOAD_MAX) {
                    // change animation to hands spreading
                    if (match->pendingActionState.batter_moving == 1 && batterIndex != -1) {
                        match->playerInfo[batterIndex].cPI.model = PLAYER_ANIM_BAT_SWING_3;
                    }
                    // set batting_mode to 2 just in case that the indicator went off the meter so early that
                    // the batter didnt even start moving yet, so that we know to choose the right one when moving
                    // starts.
                    match->pendingActionState.batting_mode = BATTING_MODE_STOP;
                    // advance only a small bit
                    match->pendingActionState.batter_advance_limit = SPREAD_ADVANCE;
                    // set flag to indicate that batting has stopped so that we there wont be checking for if the
                    // bat has hit the ball
                    match->pendingActionState.batting_stopped = 1;
                    // set swing to BAT_ACTION_DONE to indicate that theres no
                    // further functionality, we just wait for the animation to end.
                    match->aF.bTAF.swing = BAT_ACTION_DONE;
                }
            }
        }
        // so here we check if the animation has ended and if batting_going_on is still on,
        // so that we dont do this but once. if it is, we set batting_going_on to zero
        // and move the player towards ready position agian.
        if ((match->pendingActionState.batting_frame_count > match->pendingActionState.pitch_frame_time -
                                                                 BAT_ANIMATION_FRAME_HIT_COUNT +
                                                                 BAT_ANIMATION_FRAME_TOTAL_COUNT) &&
            match->pRAI.batting_going_on == 1) {
            Vector3D target;
            match->pRAI.batting_going_on = 0;

            target.x = (float)(fieldPositions->pitchPlate.x + cos(ZERO_BATTING_ANGLE) * BATTING_RADIUS);
            target.z = (float)(fieldPositions->pitchPlate.z - sin(ZERO_BATTING_ANGLE) * BATTING_RADIUS);
            if (batterIndex != -1) {
                move_to_target(match->playerInfo, batterIndex, &target);
            }
        }
        // so here we check if the bat hits. this event happens always the pitch has been in air
        else if (match->pendingActionState.batting_frame_count > match->pendingActionState.pitch_frame_time) {
            // so here we continue only if user hasn't decided to not to bat and if we havent bat already.
            // Guard reads sticky flag: on the hit frame it's still NONE (allows processing),
            // referee promotes it later this frame, and next frame the guard blocks re-entry.
            if (match->pendingActionState.batting_stopped == 0 && betweenPitchState->batOutcome == BAT_OUTCOME_NONE) {
                // if ball doesnt go too far away to left or right
                if (match->ballInfo.location.x < BALL_MAX_OFFSET && match->ballInfo.location.x > -BALL_MAX_OFFSET) {
                    float verticalAngle;
                    float horizontalAngle;
                    float power;

                    verticalAngle = calculate_batting_vertical_angle(
                        match->pendingActionState.selected_batting_power_count,
                        match->pendingActionState.selected_batting_angle_count, match->ballInfo.velocity.y,
                        BAT_SWING_MAX, BAT_LOAD_MAX
                    );

                    // 2 to make it possible to bat to every direction on the field and a bit over.
                    horizontalAngle = -match->pendingActionState.batter_angle * 2;
                    power = (float)match->pendingActionState.selected_batting_power_count;
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
                            match->pRAI.refresh_catch_and_change = 1;
                            match->pRAI.init_player_selection = 1;
                            // and the sound
                            *playSoundEffect = SOUND_SWING;
                            // bat hits — event for referee to promote
                            match->gameEvents.ballHitByBat = 1;
                            // not a pitch anymore
                            match->pRAI.pitch_state = PITCH_STAGE_NONE;
                            // no throw going on now
                            match->pRAI.throw_going_to_base = -1;

                            // move the batter if wanted

                            if (match->pRAI.will_start_running[0] == 1) {
                                int base_index = get_base_controller(match, referee, BASE_HOME);
                                match->pRAI.will_start_running[0] = 0;
                                if (base_index != -1) {
                                    run_to_next_base(match, fieldPositions, base_index, BASE_HOME);
                                    match->pendingActionState.run_bat_flag = 1;
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

void update_batting_meter(MatchSession* match)
{
    // when power has yet to be selected but is to be selected we increase the counter
    // and map the value to proper floating point value to let us show it on the screen.
    if (match->aF.bTAF.swing == BAT_ACTION_WAIT_FOR_BALL) {
        if (match->pendingActionState.meter_counter < match->pendingActionState.meter_counter_max) {
            match->pendingActionState.meter_counter += 1;
        }
        match->pRAI.swing_meter_value = calculate_power_meter_value(
            match->pendingActionState.meter_counter, match->pendingActionState.meter_counter_max
        );
    }
    // when power is selected but angle is to be selected
    else if (match->aF.bTAF.swing == BAT_ACTION_ANGLE_WAIT) {
        // if the value is still valid, increase it
        if (match->pendingActionState.meter_counter < match->pendingActionState.meter_counter_max) {
            match->pendingActionState.meter_counter += 1;
        }
        // otherwise select angle to be the maximum.
        else {
            match->pendingActionState.selected_batting_angle_count = match->pendingActionState.meter_counter_max;
        }

        match->pRAI.swing_meter_value = calculate_angle_meter_value(
            match->pendingActionState.meter_counter, match->pendingActionState.meter_counter_max,
            match->pendingActionState.selected_batting_power_count, BAT_SWING_MAX, BAT_LOAD_MAX
        );
    }
}
