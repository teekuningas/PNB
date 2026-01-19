#include "actions_messy/pitching_system.h"
#include "common_logic.h"
#include "action_implementation.h"
#include "pitching_ai_strategy.h"
#include "rng.h"
#include <stdlib.h> // for rand()
#include "base_logic.h"
#include "base_control.h"
#include "state_validator.h"

// Required local constant (was in action_implementation.c)
#define ANIMATION_FREQUENCY 3
#define TIMEOUT_CONSTANT 200

void resetPitchingSystem(StateInfo* stateInfo)
{
	stateInfo->match->pendingActionState.pitchPower = 0;
	stateInfo->match->aiState.pitchStage = 0;
	stateInfo->match->aiState.pitchTime = -1;
	stateInfo->match->aiState.pitchPreviousTime = -1;
	stateInfo->match->aiState.pitchFirstLimit = 0;
	stateInfo->match->aiState.pitchSecondLimit = 0;
	stateInfo->match->aiState.batterReadyTimer = -1;
}

void startPitch(StateInfo* stateInfo)
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
	if(stateInfo->match->pII.hasBallIndex == stateInfo->match->pII.catcherOnBaseIndex[0] &&stateInfo->match->pRAI.pitchState == PITCH_STAGE_NONE &&
	        stateInfo->match->pRAI.batterReady == 1 &&stateInfo->match->pendingActionState.throwGoingOn == 0 &&
	        stateInfo->match->playerInfo[stateInfo->match->pII.catcherOnBaseIndex[0]].cTPI.isNearHomeLocation == 1 &&
	        stateInfo->match->flowControl.waitingForFreeWalkDecision == 0) {
		// we stop the pitcher if we were moving with it when we started
		if(stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].cPI.moving == 1) {
			stopMovement(stateInfo->match->playerInfo, stateInfo->match->pII.hasBallIndex);
		}
		// we choose animation of pitcher crouching.
		stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].cPI.model = PLAYER_ANIM_PITCH_WINDUP;
		stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].cPI.animationStage = 0;
		stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].cPI.animationStageCount = PITCH_DOWN_MAX;
		stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].cPI.animationFrequency = ANIMATION_FREQUENCY;
		// and we force pitcher to this specific pitching location as the pitching can be started even if
		// pitcher is not exactly at this location.
		stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].tPI.location.x =
		    stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].tPI.homeLocation.x;
		stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].tPI.location.z =
		    stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].tPI.homeLocation.z;
		stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].cPI.lastLastLocationUpdate = 1;
		// and set the pitcher to look directly to pitchPlate's direction.
		stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].tPI.orientation.x = -1.0f;
		stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].tPI.orientation.z = 0.0f;
		// ball is moved to center of the pitchPlate so that pitchs will start
		// rising from there.
		setVectorXZ(&(stateInfo->match->ballInfo.location), 0.0f, 0.0f);


		// we enter the next stage where the meter moves and user needs to
		// select the power to continue
		stateInfo->match->aF.cTAF.pitch = PITCH_ACTION_POWER_WAIT;
		// we set pitchState flag to PITCH_STAGE_WINDUP which will hold to the moment
		// of bat hitting ball, meter going all the way down ( no angle selected )
		// or ball hitting ground.
		stateInfo->match->pRAI.pitchState = PITCH_STAGE_WINDUP;
		// so initialize meterCounter and meterCounterMax values. synchronization with the animation here is nice
		// as it will let user press the buttons when its natural in the animation. But basically
		// we start from the point 4/13 and go to 1 on the meter.
		stateInfo->match->pendingActionState.meterCounter = (PITCH_UP_MAX - PITCH_DOWN_MAX)*ANIMATION_FREQUENCY;
		stateInfo->match->pendingActionState.meterCounterMax = PITCH_UP_MAX * ANIMATION_FREQUENCY;
	} else {
		// if conditions dont hold then put pitch=PITCH_ACTION_IDLE so that user can try to
		// initiate new pitch if he wants.
		stateInfo->match->aF.cTAF.pitch = PITCH_ACTION_IDLE;
		stateInfo->match->aF.cTAF.actionKeyLock = 0;
	}
}

void continuePitch(StateInfo* stateInfo)
{
	if(stateInfo->match->pII.hasBallIndex != -1) {
		// as power is selected now, we move to the next phase of meter going down, animation
		// going from crouching to releasing and user to selecting the angle.
		stateInfo->match->aF.cTAF.pitch = PITCH_ACTION_ANGLE_WAIT;
		// here we select pitchpower, and as selected it will be in the interval from
		//	(PITCH_UP_MAX - PITCH_DOWN_MAX)/PITCH_UP_MAX to 1.
		stateInfo->match->pendingActionState.pitchPower = calculate_pitch_power(stateInfo->match->pendingActionState.meterCounter, stateInfo->match->pendingActionState.meterCounterMax);
		// we select the animation
		stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].cPI.model = PLAYER_ANIM_PITCH_THROW;
		stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].cPI.animationFrequency = ANIMATION_FREQUENCY;

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
		stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].cPI.animationStage =
		    (stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].cPI.animationStageCount -
		     stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].cPI.animationStage / ANIMATION_FREQUENCY) *
		    ANIMATION_FREQUENCY;
		stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].cPI.animationStageCount = PITCH_UP_MAX;

		// so now we initialize meterCounter to be what was left to the full amount in previous phase and set counterMax to full maximum.
		// on the screen this meterCounter-value is kind of reversed so that we get a nice indicator going up, indicator going down -effect.
		stateInfo->match->pendingActionState.meterCounter = stateInfo->match->pendingActionState.meterCounterMax - stateInfo->match->pendingActionState.meterCounter;
		stateInfo->match->pendingActionState.meterCounterMax = PITCH_UP_MAX * ANIMATION_FREQUENCY;
	}
}

void releasePitch(StateInfo* stateInfo)
{
	// so here we have now selected the angle also and ball is ready to see the world.
	Vector3D target;
	float dx, dy;
	int i;
	float pitchAngle;
	// as meterCounter goes from 0 to PITCH_UP_MAX and the zero point will be at the 9/13, we minus
	// that to get the selected angle
	pitchAngle = calculate_pitch_angle(stateInfo->match->pendingActionState.meterCounter, stateInfo->match->pendingActionState.meterCounterMax);
	// So here we set the velocity for the ball when it finally leaves the hand of the pitcher.
	// dx is going to be the error term and it doesnt depend on the power so when ball is pitched higher, the error will have more time to
	// increase
	dx = calculate_pitch_dx(pitchAngle);
	// simple formula, just have base_speed so that there wont any very low pitches and then add some power if wanted.
	// it will be made so that its more difficult to hit the ball the higher the pitch is.
	dy = calculate_pitch_dy(stateInfo->match->pendingActionState.pitchPower);
	// we prepare to move the pitcher a bit
	target.x = stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].tPI.location.x + PITCHER_MOVE_AWAY_OFFSET;
	target.z = stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].tPI.location.z;
	// set the ball visible and tell other code that is moving so its location
	// will be updated.
	stateInfo->match->ballInfo.visible = 1;
	stateInfo->match->ballInfo.moving = 1;
	stateInfo->match->ballInfo.currentFlightHasHitGround = 0;
	stateInfo->match->ballInfo.onGround = 0;
	// set the velocity by our dx and dy
	setVectorXYZ(&(stateInfo->match->ballInfo.velocity), dx, dy, 0);
	// .. and move the pitcher
	moveToTarget(stateInfo->match->playerInfo, stateInfo->match->pII.hasBallIndex, &target);
	// set lastHadBallIndex so that pitcher wont catch the ball without it hitting ground first
	stateInfo->match->pII.lastHadBallIndex = stateInfo->match->pII.hasBallIndex; // to allow ball to avoid catching by same player when thrown
	// pitcher doesnt have the ball anymore
	stateInfo->match->pII.hasBallIndex = -1;
	// pitch in air so that for example the batting can be
	// updated.
	stateInfo->match->pRAI.pitchState = PITCH_STAGE_AIRBORNE;
	// this flag's purpose is to take care of batter who starts running towards first base and comes back
	// during the pitch.
	stateInfo->match->pendingActionState.runBatFlag = 0;
	// batter can advance now
	stateInfo->match->pRAI.batterCanAdvance = 1;
	// let ai do the calculation for ball again
	stateInfo->match->aiState.aiWrongPitch = 0;
	// set camera back to normal if there was homerun camera
	stateInfo->match->cameraState.homeRunCameraFlag = 0;

	// Trigger pitch released event
	stateInfo->match->gameEvents.pitchReleased = 1;

	// Note: Referee state snapshotting is now handled by Referee_Update responding to pitchReleased event.

	// run with batting team

	for(i = 1; i < BASE_COUNT; i++) {
		if(stateInfo->match->pRAI.willStartRunning[i] == 1) {
			int index = get_base_controller(stateInfo->match, (BaseID)i);
			stateInfo->match->pRAI.willStartRunning[i] = 0;
			if(index != -1) {
				runToNextBase(stateInfo->match, stateInfo->fieldPositions, index, (BaseID)i);
			}
		}
	}

	// and pitch is PITCH_ACTION_IDLE so we can try to start pitch again when necessary conditions hold
	stateInfo->match->aF.cTAF.pitch = PITCH_ACTION_IDLE;
	stateInfo->match->aF.cTAF.actionKeyLock = 0;

	StateValidator_CaptureSnapshot(stateInfo, "PITCH_START");
}

void updatePitchingMeter(StateInfo* stateInfo)
{
	// when pitch has been started but power not yet selected,
	// we increase meterCounter until its in its maximum
	if(stateInfo->match->aF.cTAF.pitch == PITCH_ACTION_POWER_WAIT) {
		if(stateInfo->match->pendingActionState.meterCounter < stateInfo->match->pendingActionState.meterCounterMax) {
			stateInfo->match->pendingActionState.meterCounter += 1;
		}
		// meterValue is used to render info to screen for user.
		stateInfo->match->pRAI.meterValue = calculate_meter_value(2, stateInfo->match->pendingActionState.meterCounter, stateInfo->match->pendingActionState.meterCounterMax);
	}
	// when power has been selected but the angle is not yet selected,
	// we increase meterCounter until its in its maximum
	else if(stateInfo->match->aF.cTAF.pitch == PITCH_ACTION_ANGLE_WAIT) {
		if(stateInfo->match->pendingActionState.meterCounter < stateInfo->match->pendingActionState.meterCounterMax) {
			stateInfo->match->pendingActionState.meterCounter += 1;
		} else {
			// if counter reaches the maximum, it means animation has
			// reached its end point and indicator on the meter would go off the meter.
			// so when this happnes we terminate the pitch.
			// first we set pitch=PITCH_ACTION_IDLE so that we can start a new pitch
			stateInfo->match->aF.cTAF.pitch = PITCH_ACTION_IDLE;
			stateInfo->match->aF.cTAF.actionKeyLock = 0;
			// and we set pitchState to PITCH_STAGE_NONE to tell other functionality in the code
			// what happened.
			stateInfo->match->pRAI.pitchState = PITCH_STAGE_NONE;
			// ball is returned to its position with player
			stateInfo->match->ballInfo.location.x = stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].tPI.location.x;
			stateInfo->match->ballInfo.location.z = stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].tPI.location.z;
			// and we choose the normal model of fielder having a ball.
			stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].cPI.model = PLAYER_ANIM_STAND_WITH_BALL;
		}
		// update what is seen on the screen.
		stateInfo->match->pRAI.meterValue = calculate_meter_value(4, stateInfo->match->pendingActionState.meterCounter, stateInfo->match->pendingActionState.meterCounterMax);
	}
}

void updateAIPitching(StateInfo* stateInfo, unsigned int* rng_seed)
{
	int pitcherIndex = stateInfo->match->pII.catcherOnBaseIndex[0];
	// here we finish pitching if started.
	// here i use these weird lock timeouts. im not sure if they are necessary
	// but they could be. not gonna try anymore.
	if(stateInfo->match->aiState.pitchStage == 1) {
		if(stateInfo->match->pendingActionState.aiLockTimeoutCounter == -1) {
			stateInfo->match->pendingActionState.aiLockTimeoutCounter = 0;
		}
		if(stateInfo->match->pendingActionState.meterCounter > stateInfo->match->aiState.pitchFirstLimit) {
			stateInfo->match->aiState.pitchStage = 2;
			stateInfo->match->aF.cTAF.pitch = PITCH_ACTION_POWER_SET;
			stateInfo->match->pendingActionState.aiLockTimeoutCounter = -1;
		} else {
			stateInfo->match->pendingActionState.aiLockTimeoutCounter++;
			if(stateInfo->match->pendingActionState.aiLockTimeoutCounter > TIMEOUT_CONSTANT) {
				stateInfo->match->aiState.pitchStage = 0;
				stateInfo->match->pendingActionState.aiActionEventLock = AI_NO_LOCK;
				stateInfo->match->pendingActionState.aiLockUpdate = 1;
				stateInfo->match->pendingActionState.aiLockTimeoutCounter = -1;
			}
		}
	} else if(stateInfo->match->aiState.pitchStage == 2) {
		if(stateInfo->match->pendingActionState.aiLockTimeoutCounter == -1) {
			stateInfo->match->pendingActionState.aiLockTimeoutCounter = 0;
		}
		if(stateInfo->match->pendingActionState.meterCounter > stateInfo->match->aiState.pitchSecondLimit) {
			stateInfo->match->aiState.pitchStage = 3;
			stateInfo->match->aF.cTAF.pitch = PITCH_ACTION_ANGLE_SET;
			stateInfo->match->pendingActionState.aiLockTimeoutCounter = -1;
		} else {
			stateInfo->match->pendingActionState.aiLockTimeoutCounter++;
			if(stateInfo->match->pendingActionState.aiLockTimeoutCounter > TIMEOUT_CONSTANT) {
				stateInfo->match->aiState.pitchStage = 0;
				stateInfo->match->pendingActionState.aiActionEventLock = AI_NO_LOCK;
				stateInfo->match->pendingActionState.aiLockUpdate = 1;
				stateInfo->match->pendingActionState.aiLockTimeoutCounter = -1;
			}
		}
	} else if(stateInfo->match->aiState.pitchStage == 3) {
		stateInfo->match->aiState.pitchStage = 0;
		stateInfo->match->pendingActionState.aiActionEventLock = AI_NO_LOCK;
		stateInfo->match->pendingActionState.aiLockUpdate = 1;
	}

	// if pitcher has the ball and he is in correct position
	if(stateInfo->match->pII.hasBallIndex == pitcherIndex &&
	        stateInfo->match->playerInfo[pitcherIndex].cTPI.isNearHomeLocation == 1) {
		// and lets give player some time to prepare
		if(stateInfo->match->aiState.batterReadyTimer > 70) {
			// try pitching.
			if(stateInfo->match->aiState.pitchStage == 0) {
				int i;
				int homeLocationFlag = 1;
				int pitchFlag = 0;

				stateInfo->match->aiState.pitchTime++;
				if(stateInfo->match->aiState.pitchTime >= 100) {
					pitchFlag = 1;
				}
				for(i = PLAYERS_IN_TEAM + JOKER_COUNT; i < 2*PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
					if(stateInfo->match->playerInfo[i].cTPI.isNearHomeLocation == 0) {
						homeLocationFlag = 0;
					}
				}
				if(stateInfo->match->pendingActionState.aiActionEventLock == AI_NO_LOCK &&stateInfo->match->pendingActionState.aiLockUpdate == 0) {
					if(homeLocationFlag == 1 &&pitchFlag == 1) {
						int rand1 = seeded_rand(rng_seed, 15);
						int rand2 = seeded_rand(rng_seed, 3);
						int rand3 = seeded_rand(rng_seed, 10);

						stateInfo->match->pendingActionState.aiActionEventLock = AI_PITCH_LOCK;
						stateInfo->match->pendingActionState.aiLockUpdate = 1;
						stateInfo->match->aiState.pitchStage = 1;
						stateInfo->match->aF.cTAF.pitch = PITCH_ACTION_START;

						int onFieldCount = count_active_batting_players(stateInfo->match->playerInfo);
						calculate_ai_pitch_targets(
						    rand1, rand2, rand3,
						    onFieldCount,
						    &(stateInfo->match->halfInningState),
						    ANIMATION_FREQUENCY,
						    &(stateInfo->match->aiState.pitchFirstLimit),
						    &(stateInfo->match->aiState.pitchSecondLimit)
						);
					} else {
						// to stop player from unnecessarily moving
					}
				}
			}
		}
	}

	if(stateInfo->match->aiState.pitchPreviousTime == stateInfo->match->aiState.pitchTime) {
		stateInfo->match->aiState.pitchTime = 0;
	}
	stateInfo->match->aiState.pitchPreviousTime = stateInfo->match->aiState.pitchTime;
	// this batterReadyTimer is used to give human player a bit more time before AI pitches.
	if(stateInfo->match->pRAI.batterReady == 1 &&
	        stateInfo->match->pII.catcherOnBaseIndex[0] == stateInfo->match->pII.hasBallIndex &&
	        stateInfo->match->aiState.batterReadyTimer == -1) {
		stateInfo->match->aiState.batterReadyTimer = 0;
	} else if(stateInfo->match->pRAI.batterReady == 0) {
		stateInfo->match->aiState.batterReadyTimer = -1;
	}
	if(stateInfo->match->aiState.batterReadyTimer != -1) {
		stateInfo->match->aiState.batterReadyTimer++;
	}
}