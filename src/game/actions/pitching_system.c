#include "actions/pitching_system.h"
#include "common_logic.h"
#include "execute_actions.h"
#include "pitching_ai_strategy.h"
#include "rng.h"
#include <stdlib.h>
#include "base_logic.h"
#include "base_control.h"
#include "state_validator.h"

// Required local constant (was in execute_actions.c)
#define ANIMATION_FREQUENCY 3
#define TIMEOUT_CONSTANT 200

void reset_pitching_system(MatchSession* match)
{
    match->pendingActionState.pitch_power = 0;
    match->pendingActionState.pitch_phase = PITCH_PHASE_NONE;
    match->aiState.pitchStage = 0;
    match->aiState.pitchTime = -1;
    match->aiState.pitchPreviousTime = -1;
    match->aiState.pitchFirstLimit = 0;
    match->aiState.pitchSecondLimit = 0;
    match->aiState.batterReadyTimer = -1;
}

void start_pitch(MatchSession* match)
{
    /*
        To start a pitch few things must hold:
        i) pitcher must have the ball
        ii) no pitch is already going on.
        iii) batter is ready
        iv) throw is not going on. we can stop pitch and throw but cant stop throw and pitch.
        v) pitcher is close enough to pitching location.
        vi) no free walk decisions pending
    */
    if (match->pII.hasBallIndex == match->pII.catcherOnBaseIndex[0] && match->pRAI.pitch_state == PITCH_STAGE_NONE &&
        match->pRAI.batter_ready == 1 && match->pendingActionState.throw_going_on == 0 &&
        match->playerInfo[match->pII.catcherOnBaseIndex[0]].cTPI.isNearHomeLocation == 1 &&
        match->flowControl.waitingForFreeWalkDecision == 0) {
        // we stop the pitcher if we were moving with it when we started
        if (match->playerInfo[match->pII.hasBallIndex].cPI.moving == 1) {
            stop_movement(match->playerInfo, match->pII.hasBallIndex);
        }
        // we choose animation of pitcher crouching.
        match->playerInfo[match->pII.hasBallIndex].cPI.model = PLAYER_ANIM_PITCH_WINDUP;
        match->playerInfo[match->pII.hasBallIndex].cPI.animationStage = 0;
        match->playerInfo[match->pII.hasBallIndex].cPI.animationStageCount = PITCH_DOWN_MAX;
        match->playerInfo[match->pII.hasBallIndex].cPI.animationFrequency = ANIMATION_FREQUENCY;
        // and we force pitcher to this specific pitching location as the pitching can be started even if
        // pitcher is not exactly at this location.
        match->playerInfo[match->pII.hasBallIndex].tPI.location.x =
            match->playerInfo[match->pII.hasBallIndex].tPI.homeLocation.x;
        match->playerInfo[match->pII.hasBallIndex].tPI.location.z =
            match->playerInfo[match->pII.hasBallIndex].tPI.homeLocation.z;
        match->playerInfo[match->pII.hasBallIndex].cPI.lastLastLocationUpdate = 1;
        // and set the pitcher to look directly to pitchPlate's direction.
        match->playerInfo[match->pII.hasBallIndex].tPI.orientation.x = -1.0f;
        match->playerInfo[match->pII.hasBallIndex].tPI.orientation.z = 0.0f;
        // ball is moved to center of the pitchPlate so that pitchs will start
        // rising from there.
        set_vector_xz(&(match->ballInfo.location), 0.0f, 0.0f);

        // Enter power-wait phase: meter moves and user needs to select power
        match->pendingActionState.pitch_phase = PITCH_PHASE_POWER_WAIT;
        match->pendingActionState.current_catching_action = CATCHING_ACTION_PITCHING;
        // Consume the START intent
        match->aF.cTAF.pitch = PITCH_ACTION_IDLE;
        // we set pitch_state flag to PITCH_STAGE_WINDUP which will hold to the moment
        // of bat hitting ball, meter going all the way down ( no angle selected )
        // or ball hitting ground.
        match->pRAI.pitch_state = PITCH_STAGE_WINDUP;
        // so initialize meter_counter and meter_counter_max values. synchronization with the animation here is nice
        // as it will let user press the buttons when its natural in the animation. But basically
        // we start from the point 4/13 and go to 1 on the meter.
        match->pendingActionState.meter_counter = (PITCH_UP_MAX - PITCH_DOWN_MAX) * ANIMATION_FREQUENCY;
        match->pendingActionState.meter_counter_max = PITCH_UP_MAX * ANIMATION_FREQUENCY;
    } else {
        // if conditions dont hold then put pitch=PITCH_ACTION_IDLE so that user can try to
        // initiate new pitch if he wants.
        match->aF.cTAF.pitch = PITCH_ACTION_IDLE;
    }
}

void continue_pitch(MatchSession* match)
{
    if (match->pII.hasBallIndex != -1) {
        // as power is selected now, we move to the next phase of meter going down, animation
        // going from crouching to releasing and user to selecting the angle.
        match->pendingActionState.pitch_phase = PITCH_PHASE_ANGLE_WAIT;
        // Consume the POWER_SET intent
        match->aF.cTAF.pitch = PITCH_ACTION_IDLE;
        // here we select pitchpower, and as selected it will be in the interval from
        //  (PITCH_UP_MAX - PITCH_DOWN_MAX)/PITCH_UP_MAX to 1.
        match->pendingActionState.pitch_power =
            calculate_pitch_power(match->pendingActionState.meter_counter, match->pendingActionState.meter_counter_max);
        // we select the animation
        match->playerInfo[match->pII.hasBallIndex].cPI.model = PLAYER_ANIM_PITCH_THROW;
        match->playerInfo[match->pII.hasBallIndex].cPI.animationFrequency = ANIMATION_FREQUENCY;

        // current stage depends on the stage of the last animation. if user quickly selects
        // the power, the previous animation might have no time to finish, so if that happens
        // we dont want to start the releasing animation from the beginning.
        // this works quite fluently as the crouching animation's end consists of same frames
        // as releasing animation's beginning.
        //
        // so here animationStageCount is PITCH_DOWN_MAX and we minus from that
        // the number of stages that we already did. as the animation implementation in the code works so that
        // animationStage goes from 0 to stage count times frequency, to get our stage relative to
        // animationStageCount, we divide by the frequency. then we multiply by new frequency
        // to get new stage to range of 0 to PITCH_DOWN_MAX(and we extend to PITCH_UP_MAX after that)
        // times the frequency.
        match->playerInfo[match->pII.hasBallIndex].cPI.animationStage =
            (match->playerInfo[match->pII.hasBallIndex].cPI.animationStageCount -
             match->playerInfo[match->pII.hasBallIndex].cPI.animationStage / ANIMATION_FREQUENCY) *
            ANIMATION_FREQUENCY;
        match->playerInfo[match->pII.hasBallIndex].cPI.animationStageCount = PITCH_UP_MAX;

        // so now we initialize meter_counter to be what was left to the full amount in previous phase and set
        // counterMax to full maximum. on the screen this meter_counter-value is kind of reversed so that we get a nice
        // indicator going up, indicator going down -effect.
        match->pendingActionState.meter_counter =
            match->pendingActionState.meter_counter_max - match->pendingActionState.meter_counter;
        match->pendingActionState.meter_counter_max = PITCH_UP_MAX * ANIMATION_FREQUENCY;
    }
}

void release_pitch(MatchSession* match, const RefereeState* referee, const FieldPositions* fieldPositions)
{
    // so here we have now selected the angle also and ball is ready to see the world.
    Vector3D target;
    float dx, dy;
    int i;
    float pitchAngle;
    // as meter_counter goes from 0 to PITCH_UP_MAX and the zero point will be at the 9/13, we minus
    // that to get the selected angle
    pitchAngle =
        calculate_pitch_angle(match->pendingActionState.meter_counter, match->pendingActionState.meter_counter_max);
    // So here we set the velocity for the ball when it finally leaves the hand of the pitcher.
    // dx is going to be the error term and it doesnt depend on the power so when ball is pitched higher, the error will
    // have more time to increase
    dx = calculate_pitch_dx(pitchAngle);
    // simple formula, just have base_speed so that there wont any very low pitches and then add some power if wanted.
    // it will be made so that its more difficult to hit the ball the higher the pitch is.
    dy = calculate_pitch_dy(match->pendingActionState.pitch_power);
    // we prepare to move the pitcher a bit
    target.x = match->playerInfo[match->pII.hasBallIndex].tPI.location.x + PITCHER_MOVE_AWAY_OFFSET;
    target.z = match->playerInfo[match->pII.hasBallIndex].tPI.location.z;
    // set the ball visible and tell other code that is moving so its location
    // will be updated.
    match->ballInfo.visible = 1;
    match->ballInfo.moving = 1;
    match->ballInfo.currentFlightHasHitGround = 0;
    match->ballInfo.onGround = 0;
    // BUG FIX: Clear hitsGroundToUnWound at pitch release
    // This flag should only be set during the current pitch's flight, not carried over from previous pitches
    match->ballInfo.hitsGroundToUnWound = 0;
    // set the velocity by our dx and dy
    set_vector_xyz(&(match->ballInfo.velocity), dx, dy, 0);
    // .. and move the pitcher
    move_to_target(match->playerInfo, match->pII.hasBallIndex, &target);
    // set lastHadBallIndex so that pitcher wont catch the ball without it hitting ground first
    match->pII.lastHadBallIndex = match->pII.hasBallIndex; // to allow ball to avoid catching by same player when thrown
    // pitcher doesnt have the ball anymore
    match->pII.hasBallIndex = -1;
    // pitch in air so that for example the batting can be
    // updated.
    match->pRAI.pitch_state = PITCH_STAGE_AIRBORNE;
    // this flag's purpose is to take care of batter who starts running towards first base and comes back
    // during the pitch.
    match->pendingActionState.run_bat_flag = 0;
    // batter can advance now
    match->pRAI.batter_can_advance = 1;
    // let ai do the calculation for ball again
    match->aiState.aiWrongPitch = 0;
    // set camera back to normal if there was homerun camera
    match->cameraState.homeRunCameraFlag = 0;

    // Trigger pitch released event
    match->gameEvents.pitchReleased = 1;

    // Note: Referee state snapshotting is now handled at end of frame in game_frame.c

    // run with batting team

    for (i = 1; i < BASE_COUNT; i++) {
        if (match->pRAI.will_start_running[i] == 1) {
            int index = get_base_controller(match, referee, (BaseID)i);
            match->pRAI.will_start_running[i] = 0;
            if (index != -1) {
                run_to_next_base(match, fieldPositions, index, (BaseID)i);
            }
        }
    }

    // Clear pitch phase and action lock — pitch complete
    match->pendingActionState.pitch_phase = PITCH_PHASE_NONE;
    match->pendingActionState.current_catching_action = CATCHING_ACTION_NONE;
    // Consume the ANGLE_SET intent
    match->aF.cTAF.pitch = PITCH_ACTION_IDLE;
}

void update_pitching_meter(MatchSession* match)
{
    // when pitch has been started but power not yet selected,
    // we increase meter_counter until its in its maximum
    if (match->pendingActionState.pitch_phase == PITCH_PHASE_POWER_WAIT) {
        if (match->pendingActionState.meter_counter < match->pendingActionState.meter_counter_max) {
            match->pendingActionState.meter_counter += 1;
        }
        // meter_value is used to render info to screen for user.
        match->pRAI.meter_value = calculate_meter_value(
            2, match->pendingActionState.meter_counter, match->pendingActionState.meter_counter_max
        );
    }
    // when power has been selected but the angle is not yet selected,
    // we increase meter_counter until its in its maximum
    else if (match->pendingActionState.pitch_phase == PITCH_PHASE_ANGLE_WAIT) {
        if (match->pendingActionState.meter_counter < match->pendingActionState.meter_counter_max) {
            match->pendingActionState.meter_counter += 1;
        } else {
            // if counter reaches the maximum, it means animation has
            // reached its end point and indicator on the meter would go off the meter.
            // so when this happnes we terminate the pitch.
            match->pendingActionState.pitch_phase = PITCH_PHASE_NONE;
            match->pendingActionState.current_catching_action = CATCHING_ACTION_NONE;
            // and we set pitch_state to PITCH_STAGE_NONE to tell other functionality in the code
            // what happened.
            match->pRAI.pitch_state = PITCH_STAGE_NONE;
            // ball is returned to its position with player
            match->ballInfo.location.x = match->playerInfo[match->pII.hasBallIndex].tPI.location.x;
            match->ballInfo.location.z = match->playerInfo[match->pII.hasBallIndex].tPI.location.z;
            // and we choose the normal model of fielder having a ball.
            match->playerInfo[match->pII.hasBallIndex].cPI.model = PLAYER_ANIM_STAND_WITH_BALL;
        }
        // update what is seen on the screen.
        match->pRAI.meter_value = calculate_meter_value(
            4, match->pendingActionState.meter_counter, match->pendingActionState.meter_counter_max
        );
    }
}
