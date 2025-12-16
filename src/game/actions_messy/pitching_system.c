#include "actions_messy/pitching_system.h"
#include "actions_messy/action_state.h"
#include "common_logic.h"
#include "action_implementation.h"
#include "pitching_ai_strategy.h"
#include "rng.h"
#include <stdlib.h> // for rand()

// Required local constant (was in action_implementation.c)
#define ANIMATION_FREQUENCY 3
#define TIMEOUT_CONSTANT 200

// Static variables moved from action_implementation.c
static float pitchPower;
static int aiPitchStage;
static unsigned int aiPitchFirstLimit;
static unsigned int aiPitchSecondLimit;
static int aiPitchTime;
static int aiPitchPreviousTime;
static int aiBatterReadyTimer;

void resetPitchingSystem(void)
{
	pitchPower = 0;
	aiPitchStage = 0;
	aiPitchTime = -1;
	aiPitchPreviousTime = -1;
	aiPitchFirstLimit = 0;
	aiPitchSecondLimit = 0;
	aiBatterReadyTimer = -1;
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
	if(stateInfo->localGameInfo->pII.hasBallIndex == stateInfo->localGameInfo->pII.catcherOnBaseIndex[0] && stateInfo->localGameInfo->pRAI.pitchGoingOn == 0 &&
	        stateInfo->localGameInfo->pRAI.batterReady == 1 && throwGoingOn == 0 &&
	        stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.catcherOnBaseIndex[0]].cTPI.isNearHomeLocation == 1 &&
	        stateInfo->localGameInfo->gAI.waitingForFreeWalkDecision == 0) {
		// we stop the pitcher if we were moving with it when we started
		if(stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.moving == 1) {
			stopMovement(stateInfo, stateInfo->localGameInfo->pII.hasBallIndex);
		}
		// we choose animation of pitcher crouching.
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.model = 6;
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationStage = 0;
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationStageCount = PITCH_DOWN_MAX;
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationFrequency = ANIMATION_FREQUENCY;
		// and we force pitcher to this specific pitching location as the pitching can be started even if
		// pitcher is not exactly at this location.
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.x =
		    stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.homeLocation.x;
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.z =
		    stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.homeLocation.z;
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.lastLastLocationUpdate = 1;
		// and set the pitcher to look directly to pitchPlate's direction.
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.orientation.x = -1.0f;
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.orientation.z = 0.0f;
		// ball is moved to center of the pitchPlate so that pitchs will start
		// rising from there.
		setVectorXZ(&(stateInfo->localGameInfo->ballInfo.location), 0.0f, 0.0f);


		// we enter the next stage where the meter moves and user needs to
		// select the power to continue
		stateInfo->localGameInfo->aF.cTAF.pitch = 2;
		// we set pitchGoingOn flag to 1 which will hold to the moment
		// of bat hitting ball, meter going all the way down ( no angle selected )
		// or ball hitting ground.
		stateInfo->localGameInfo->pRAI.pitchGoingOn = 1;
		// so initialize meterCounter and meterCounterMax values. synchronization with the animation here is nice
		// as it will let user press the buttons when its natural in the animation. But basically
		// we start from the point 4/13 and go to 1 on the meter.
		meterCounter = (PITCH_UP_MAX - PITCH_DOWN_MAX)*ANIMATION_FREQUENCY;
		meterCounterMax = PITCH_UP_MAX * ANIMATION_FREQUENCY;
	} else {
		// if conditions dont hold then put pitch=0 so that user can try to
		// initiate new pitch if he wants.
		stateInfo->localGameInfo->aF.cTAF.pitch = 0;
		stateInfo->localGameInfo->aF.cTAF.actionKeyLock = 0;
	}
}

void continuePitch(StateInfo* stateInfo)
{
	if(stateInfo->localGameInfo->pII.hasBallIndex != -1) {
		// as power is selected now, we move to the next phase of meter going down, animation
		// going from crouching to releasing and user to selecting the angle.
		stateInfo->localGameInfo->aF.cTAF.pitch = 4;
		// here we select pitchpower, and as selected it will be in the interval from
		//	(PITCH_UP_MAX - PITCH_DOWN_MAX)/PITCH_UP_MAX to 1.
		pitchPower = calculate_pitch_power(meterCounter, meterCounterMax);
		// we select the animation
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.model = 7;
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationFrequency = ANIMATION_FREQUENCY;

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
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationStage =
		    (stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationStageCount -
		     stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationStage / ANIMATION_FREQUENCY) *
		    ANIMATION_FREQUENCY;
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationStageCount = PITCH_UP_MAX;

		// so now we initialize meterCounter to be what was left to the full amount in previous phase and set counterMax to full maximum.
		// on the screen this meterCounter-value is kind of reversed so that we get a nice indicator going up, indicator going down -effect.
		meterCounter = meterCounterMax - meterCounter;
		meterCounterMax = PITCH_UP_MAX * ANIMATION_FREQUENCY;
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
	pitchAngle = calculate_pitch_angle(meterCounter, meterCounterMax);
	// So here we set the velocity for the ball when it finally leaves the hand of the pitcher.
	// dx is going to be the error term and it doesnt depend on the power so when ball is pitched higher, the error will have more time to
	// increase
	dx = calculate_pitch_dx(pitchAngle);
	// simple formula, just have base_speed so that there wont any very low pitches and then add some power if wanted.
	// it will be made so that its more difficult to hit the ball the higher the pitch is.
	dy = calculate_pitch_dy(pitchPower);
	// we prepare to move the pitcher a bit
	target.x = stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.x + PITCHER_MOVE_AWAY_OFFSET;
	target.z = stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.z;
	// set the ball visible and tell other code that is moving so its location
	// will be updated.
	stateInfo->localGameInfo->ballInfo.visible = 1;
	stateInfo->localGameInfo->ballInfo.moving = 1;
	// set the velocity by our dx and dy
	setVectorXYZ(&(stateInfo->localGameInfo->ballInfo.velocity), dx, dy, 0);
	// .. and move the pitcher
	moveToTarget(stateInfo, stateInfo->localGameInfo->pII.hasBallIndex, &target);
	// set lastHadBallIndex so that pitcher wont catch the ball without it hitting ground first
	stateInfo->localGameInfo->pII.lastHadBallIndex = stateInfo->localGameInfo->pII.hasBallIndex; // to allow ball to avoid catching by same player when thrown
	// pitcher doesnt have the ball anymore
	stateInfo->localGameInfo->pII.hasBallIndex = -1;
	// pitch in air so that for example the batting can be
	// updated.
	stateInfo->localGameInfo->pRAI.pitchInAir = 1;
	// this flag's purpose is to take care of batter who starts running towards first base and comes back
	// during the pitch.
	runBatFlag = 0;
	// batter can advance now
	stateInfo->localGameInfo->pRAI.batterCanAdvance = 1;
	// let ai do the calculation for ball again
	aiWrongPitch = 0;
	// set camera back to normal if there was homerun camera
	stateInfo->localGameInfo->gAI.homeRunCameraFlag = 0;
	// always when pitch reaches the stage of ball going to air, we update baserunners'
	// original bases to their current bases, so that we can make decisions about
	// foul plays and wounds etc.
	for(i = 0; i < BASE_COUNT; i++) {
		int index = stateInfo->localGameInfo->pII.battingTeamOnFieldIndices[i];
		if(index != -1) {
			int base = stateInfo->localGameInfo->playerInfo[index].bTPI.base;
			// we dont do it though in the case of free walks, as we dont want players to return previous bases
			// after taking a free walk, even if there is foul play. so thats the reason for conditions.
			// free walks set original base to base that follows the base where player was when the
			// free walk decision came available.
			if(stateInfo->localGameInfo->playerInfo[index].bTPI.originalBase < base &&
			        !(stateInfo->localGameInfo->playerInfo[index].bTPI.originalBase == 4 && base == 3)) {
				int done = 0;
				// no such thing as safeOnBaseIndex[4] so have to be base < 4
				// here we just make sure that player is safe on the base that is declared
				// as his originalBase.
				// if he's not, he can try gaining now originalBase by running to next one
				// or he will just get tagged if its foul play.
				if(base >= 0 && base < 4) {
					if(stateInfo->localGameInfo->pII.safeOnBaseIndex[base] != index) {
						done = 1;
						stateInfo->localGameInfo->playerInfo[index].bTPI.originalBase = -1;
					}
				}
				if(done == 0) {
					stateInfo->localGameInfo->playerInfo[index].bTPI.originalBase = stateInfo->localGameInfo->playerInfo[index].bTPI.base;
				}
			}
		}
	}

	// run with batting team

	for(i = 1; i < BASE_COUNT; i++) {
		if(stateInfo->localGameInfo->pRAI.willStartRunning[i] == 1) {
			int index = stateInfo->localGameInfo->pII.safeOnBaseIndex[i];
			stateInfo->localGameInfo->pRAI.willStartRunning[i] = 0;
			if(index != -1) {
				runToNextBase(stateInfo, index, i);
			}
		}
	}

	// and pitch is zero so we can try to start pitch again when necessary conditions hold
	stateInfo->localGameInfo->aF.cTAF.pitch = 0;
	stateInfo->localGameInfo->aF.cTAF.actionKeyLock = 0;

}

void updatePitchingMeter(StateInfo* stateInfo)
{
	// when pitch has been started but power not yet selected,
	// we increase meterCounter until its in its maximum
	if(stateInfo->localGameInfo->aF.cTAF.pitch == 2) {
		if(meterCounter < meterCounterMax) {
			meterCounter += 1;
		}
		// meterValue is used to render info to screen for user.
		stateInfo->localGameInfo->pRAI.meterValue = calculate_meter_value(2, meterCounter, meterCounterMax);
	}
	// when power has been selected but the angle is not yet selected,
	// we increase meterCounter until its in its maximum
	else if(stateInfo->localGameInfo->aF.cTAF.pitch == 4) {
		if(meterCounter < meterCounterMax) {
			meterCounter += 1;
		} else {
			// if counter reaches the maximum, it means animation has
			// reached its end point and indicator on the meter would go off the meter.
			// so when this happnes we terminate the pitch.
			// first we set pitch=0 so that we can start a new pitch
			stateInfo->localGameInfo->aF.cTAF.pitch = 0;
			stateInfo->localGameInfo->aF.cTAF.actionKeyLock = 0;
			// and we set pitchGoingOn to 0 to tell other functionality in the code
			// what happened.
			stateInfo->localGameInfo->pRAI.pitchGoingOn = 0;
			// ball is returned to its position with player
			stateInfo->localGameInfo->ballInfo.location.x = stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.x;
			stateInfo->localGameInfo->ballInfo.location.z = stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.z;
			// and we choose the normal model of fielder having a ball.
			stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.model = 1;
		}
		// update what is seen on the screen.
		stateInfo->localGameInfo->pRAI.meterValue = calculate_meter_value(4, meterCounter, meterCounterMax);
	}
}

void updateAIPitching(StateInfo* stateInfo, unsigned int* rng_seed)
{
	int pitcherIndex = stateInfo->localGameInfo->pII.catcherOnBaseIndex[0];
	// here we finish pitching if started.
	// here i use these weird lock timeouts. im not sure if they are necessary
	// but they could be. not gonna try anymore.
	if(aiPitchStage == 1) {
		if(aiLockTimeoutCounter == -1) {
			aiLockTimeoutCounter = 0;
		}
		if(meterCounter > aiPitchFirstLimit) {
			aiPitchStage = 2;
			stateInfo->keyStates->imitateKeyPress[KEY_2] = 0;
			aiLockTimeoutCounter = -1;
		} else {
			aiLockTimeoutCounter++;
			if(aiLockTimeoutCounter > TIMEOUT_CONSTANT) {
				aiPitchStage = 0;
				flushKeys(stateInfo);
				aiActionEventLock = AI_NO_LOCK;
				aiLockUpdate = 1;
				aiLockTimeoutCounter = -1;
			}
		}
	} else if(aiPitchStage == 2) {
		if(aiLockTimeoutCounter == -1) {
			aiLockTimeoutCounter = 0;
		}
		if(meterCounter > aiPitchSecondLimit) {
			aiPitchStage = 3;
			stateInfo->keyStates->imitateKeyPress[KEY_2] = 1;
			aiLockTimeoutCounter = -1;
		} else {
			aiLockTimeoutCounter++;
			if(aiLockTimeoutCounter > TIMEOUT_CONSTANT) {
				aiPitchStage = 0;
				flushKeys(stateInfo);
				aiActionEventLock = AI_NO_LOCK;
				aiLockUpdate = 1;
				aiLockTimeoutCounter = -1;
			}
		}
	} else if(aiPitchStage == 3) {
		aiPitchStage = 0;
		flushKeys(stateInfo);
		aiActionEventLock = AI_NO_LOCK;
		aiLockUpdate = 1;
	}

	// if pitcher has the ball and he is in correct position
	if(stateInfo->localGameInfo->pII.hasBallIndex == pitcherIndex &&
	        stateInfo->localGameInfo->playerInfo[pitcherIndex].cTPI.isNearHomeLocation == 1) {
		// and lets give player some time to prepare
		if(aiBatterReadyTimer > 70) {
			// try pitching.
			if(aiPitchStage == 0) {
				int i;
				int homeLocationFlag = 1;
				int pitchFlag = 0;

				aiPitchTime++;
				if(aiPitchTime >= 100) {
					pitchFlag = 1;
				}
				for(i = PLAYERS_IN_TEAM + JOKER_COUNT; i < 2*PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
					if(stateInfo->localGameInfo->playerInfo[i].cTPI.isNearHomeLocation == 0) {
						homeLocationFlag = 0;
					}
				}
				if(aiActionEventLock == AI_NO_LOCK && aiLockUpdate == 0) {
					if(homeLocationFlag == 1 && pitchFlag == 1) {
						int rand1 = seeded_rand(rng_seed, 15);
						int rand2 = seeded_rand(rng_seed, 3);
						int rand3 = seeded_rand(rng_seed, 10);

						aiActionEventLock = AI_PITCH_LOCK;
						aiLockUpdate = 1;
						aiPitchStage = 1;
						flushKeys(stateInfo);
						stateInfo->keyStates->imitateKeyPress[KEY_2] = 1;

						calculate_ai_pitch_targets(
						    rand1, rand2, rand3,
						    stateInfo->localGameInfo->gAI.battingTeamPlayersOnFieldCount,
						    stateInfo->localGameInfo->gAI.strikes,
						    stateInfo->localGameInfo->gAI.balls,
						    ANIMATION_FREQUENCY,
						    &aiPitchFirstLimit,
						    &aiPitchSecondLimit
						);
					} else {
						// to stop player from unnecessarily moving
						flushKeys(stateInfo);
					}
				}
			}
		}
	}

	if(aiPitchPreviousTime == aiPitchTime) {
		aiPitchTime = 0;
	}
	aiPitchPreviousTime = aiPitchTime;
	// this batterReadyTimer is used to give human player a bit more time before AI pitches.
	if(stateInfo->localGameInfo->pRAI.batterReady == 1 &&
	        stateInfo->localGameInfo->pII.catcherOnBaseIndex[0] == stateInfo->localGameInfo->pII.hasBallIndex &&
	        aiBatterReadyTimer == -1) {
		aiBatterReadyTimer = 0;
	} else if(stateInfo->localGameInfo->pRAI.batterReady == 0) {
		aiBatterReadyTimer = -1;
	}
	if(aiBatterReadyTimer != -1) {
		aiBatterReadyTimer++;
	}
}