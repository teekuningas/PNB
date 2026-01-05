#include "batting_system.h"
#include "common_logic.h"
#include "action_implementation.h"
#include "actions_messy/pitching_system.h"
#include "actions_pure/batting_physics.h"
#include <math.h>
#include "base_logic.h"

// Macros moved from action_implementation.c
#define BATTER_ANGLE_SPEED_CONSTANT 0.02f
#define BATTER_ANGLE_LIMIT PI/7
#define GENERIC_BATTER_ADVANCE_SPEED_CONSTANT 0.7f

#define PITCH_FRAME_TIME_TWEAK 3
#define ANIMATION_FREQUENCY 3
#define BAT_ANIMATION_FRAME_HIT_COUNT (20*ANIMATION_FREQUENCY)
#define BAT_ANIMATION_FRAME_TOTAL_COUNT (34*ANIMATION_FREQUENCY)
#define BALL_MAX_OFFSET 1.0f
#define BUNT_THRESHOLD 20

#define BUNT_ADVANCE 1.0f
#define SWING_ADVANCE 0.5f
#define SPREAD_ADVANCE 0.3f

#define BATTER_ANGLE_FIX (2*PI / 4)

void initBattingSystem(StateInfo* stateInfo)
{
	stateInfo->localGameInfo->pendingActionState.batterSelect = 0;
	stateInfo->localGameInfo->pendingActionState.battingFrameCount = 0;
	stateInfo->localGameInfo->pendingActionState.increaseBattingFrameCount = 0;
	stateInfo->localGameInfo->pendingActionState.selectedBattingPowerCount = 0;
	stateInfo->localGameInfo->pendingActionState.selectedBattingAngleCount = 0;
	stateInfo->localGameInfo->pendingActionState.batterAngle = 0;
	stateInfo->localGameInfo->pendingActionState.batterAngleSpeed = 0;
	stateInfo->localGameInfo->pendingActionState.batterAdvanceSpeed = 0;
	stateInfo->localGameInfo->pendingActionState.batterAdvance = 0;
	stateInfo->localGameInfo->pendingActionState.battingMode = BATTING_MODE_SWING;
	stateInfo->localGameInfo->pendingActionState.batterAdvanceLimit = 0;
	stateInfo->localGameInfo->pendingActionState.battingStopped = 0;
	stateInfo->localGameInfo->pendingActionState.batterMoving = 0;
	stateInfo->localGameInfo->pendingActionState.updateBatterLocationAndOrientation = 0;
	stateInfo->localGameInfo->pendingActionState.pitchFrameTime = 0;
}

void startIncreaseBatterAngle(StateInfo* stateInfo)
{
	stateInfo->localGameInfo->aF.bTAF.increaseBatterAngle = ACTION_ACTIVE;
	// set batterAngleSpeed to 1 to indicate that the direction of the movement is cw
	stateInfo->localGameInfo->pendingActionState.batterAngleSpeed = 1;

}
void stopIncreaseBatterAngle(StateInfo* stateInfo)
{
	stateInfo->localGameInfo->aF.bTAF.increaseBatterAngle = ACTION_IDLE;
	// when stopping the increasing of the angle, we want not to interrupt an ongoing decreasing of the angle
	if(stateInfo->localGameInfo->pendingActionState.batterAngleSpeed != -1) {
		stateInfo->localGameInfo->pendingActionState.batterAngleSpeed = 0;
	}
	stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].cPI.lastLastLocationUpdate = 1;
}

void startDecreaseBatterAngle(StateInfo* stateInfo)
{
	stateInfo->localGameInfo->aF.bTAF.decreaseBatterAngle = ACTION_ACTIVE;
	// set batterAngleSpeed to 1 to indicate that the direction of the movement is ccw
	stateInfo->localGameInfo->pendingActionState.batterAngleSpeed = -1;

}
void stopDecreaseBatterAngle(StateInfo* stateInfo)
{
	stateInfo->localGameInfo->aF.bTAF.decreaseBatterAngle = ACTION_IDLE;
	// when stopping the decreasing of the angle, we want not to interrupt an ongoing increasing of the angle
	if(stateInfo->localGameInfo->pendingActionState.batterAngleSpeed != 1) {
		stateInfo->localGameInfo->pendingActionState.batterAngleSpeed = 0;
	}
	stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].cPI.lastLastLocationUpdate = 1;
}

// here is where the accepting selected player happens.
void selectBatter(StateInfo* stateInfo)
{
	int i = 0;
	int battingTeamIndex = (stateInfo->globalGameInfo->
	                        inning+stateInfo->globalGameInfo->playsFirst+stateInfo->globalGameInfo->period)%2;
	// index cannot be -1 as we couldn't have got this far if it was
	int index = stateInfo->localGameInfo->pII.batterSelectionIndex;
	if(index != -1) {
		Vector3D target;
		// we set the batterSelect to be 0 so that it will be correct one next time we have the decision
		stateInfo->localGameInfo->pendingActionState.batterSelect = 0;
		// and set these to 0 as decision made.
		stateInfo->localGameInfo->gameControl.waitingForBatterDecision = 0;
		stateInfo->localGameInfo->aF.bTAF.chooseBatter = CHOOSE_BATTER_IDLE;

		// here we look for a free spot in battingTeamOnFieldIndices[]
		// and put our new guy there. there will be a spot as we cannot get here
		// if there is more than 3 baserunners already.
		while(1) {
			if(stateInfo->localGameInfo->pII.battingTeamOnFieldIndices[i] == -1) {
				stateInfo->localGameInfo->pII.battingTeamOnFieldIndices[i] = index;
				break;
			} else {
				if(i < 3)
					i++;
				else
					break;
			}

		}
		// new batting team player on the field.
		stateInfo->localGameInfo->playerCounters.battingTeamPlayersOnFieldCount++;
		// has base of zero, is on base and original base is zero too.
		stateInfo->localGameInfo->playerInfo[index].bTPI.baseId = BASE_HOME;
		stateInfo->localGameInfo->playerInfo[index].bTPI.state = PLAYER_STATE_AT_BAT;
		stateInfo->localGameInfo->playerInfo[index].bTPI.originalBase = BASE_HOME;
		// this guy will begin with 0 strikes and 0 balls.
		stateInfo->localGameInfo->gameState.strikes = 0;
		stateInfo->localGameInfo->gameState.balls = 0;
		// set batterIndex
		stateInfo->localGameInfo->pII.batterIndex = index;
		// and they are safe on home base
		stateInfo->localGameInfo->pII.safeOnBaseIndex[0] = index;
		// cant advance yet
		stateInfo->localGameInfo->pRAI.batterCanAdvance = 0;
		// just set default values so that the player can have a fresh start at
		// the field.
		stateInfo->localGameInfo->playerRuntime[index].goingForward = 0;
		stateInfo->localGameInfo->playerRuntime[index].woundedApply = 0;
		stateInfo->localGameInfo->playerRuntime[index].passedPathPoint = 0;
		stateInfo->localGameInfo->playerRuntime[index].hasMadeRunOnThirdBase = 0;
		// if he is a (unused) joker player, mark him as used, and decrease the amount of jokers left.
		if(stateInfo->localGameInfo->playerInfo[index].bTPI.joker == JOKER_AVAILABLE) {
			stateInfo->localGameInfo->playerCounters.jokersLeft--;
			stateInfo->localGameInfo->playerInfo[index].bTPI.joker = JOKER_USED;
		} else {
			// otherwise he is not a joker player and we must decrease the amount of those.
			stateInfo->localGameInfo->playerCounters.nonJokerPlayersLeft--;
			// also the batterIndex will increase(mod 9)
			stateInfo->globalGameInfo->teams[battingTeamIndex].batterOrderIndex = (stateInfo->globalGameInfo->teams[battingTeamIndex].batterOrderIndex + 1)%PLAYERS_IN_TEAM;
		}
		// move player to default batter ready position
		target.x = (float)(stateInfo->fieldPositions->pitchPlate.x + cos(ZERO_BATTING_ANGLE)*BATTING_RADIUS);
		target.z = (float)(stateInfo->fieldPositions->pitchPlate.z - sin(ZERO_BATTING_ANGLE)*BATTING_RADIUS);
		// move to target can take care of the rest.
		moveToTarget(stateInfo->localGameInfo->playerInfo, index, &target);
	}
}

// so this function is called when we decide the power
void selectPower(StateInfo* stateInfo)
{
	// swing is set to BAT_ACTION_ANGLE_WAIT so that the meter indicator on the screen can start decreasing etc.
	stateInfo->localGameInfo->aF.bTAF.swing = BAT_ACTION_ANGLE_WAIT;
	// we select meterCounter value as the power but we move it a bit so that the smallest value isnt 4/13 but instead 0.
	stateInfo->localGameInfo->pendingActionState.selectedBattingPowerCount = stateInfo->localGameInfo->pendingActionState.meterCounter - (BAT_SWING_MAX - BAT_LOAD_MAX);
	// and then for angle we start again from zero and go to the max and later on we'll scale it a bit to look nice on the screen.
	// this is used so that more power we use, faster will the indicator come down when trying too select the angle.
	stateInfo->localGameInfo->pendingActionState.meterCounter = 0;
	stateInfo->localGameInfo->pendingActionState.meterCounterMax = BAT_SWING_MAX;

	// if power is small, we use bunting animation.
	if(stateInfo->localGameInfo->pendingActionState.selectedBattingPowerCount < BUNT_THRESHOLD) {
		// and its updated here only if the player is moving already so that the animation wont start too early.
		if(stateInfo->localGameInfo->pendingActionState.batterMoving == 1) {
			stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].cPI.model = PLAYER_ANIM_BAT_SWING_2;
		}
		// will allow us to move bit further when bunting as the model needs to be closer to ball to look nice.
		stateInfo->localGameInfo->pendingActionState.batterAdvanceLimit = BUNT_ADVANCE;
		// this will take care that if we indeed were too early for the animation to start yet, the correct animation
		// will start when the batter starts to move.
		stateInfo->localGameInfo->pendingActionState.battingMode = BATTING_MODE_BUNT;

	}
	// if power is great enough to give us good swing we'll go with it but thats the default so no need to do anything here.

}
void selectAngle(StateInfo* stateInfo)
{
	// simple enough, enter the state of waiting for animation to end
	stateInfo->localGameInfo->aF.bTAF.swing = BAT_ACTION_DONE;
	// and set angle to be the meterCounter value, its processed further afterwards.
	stateInfo->localGameInfo->pendingActionState.selectedBattingAngleCount = stateInfo->localGameInfo->pendingActionState.meterCounter;
}

void updateBatting(StateInfo* stateInfo)
{
	if(stateInfo->localGameInfo->pRAI.batterReady == 1 && stateInfo->localGameInfo->pRAI.pitchState != PITCH_STAGE_AIRBORNE && stateInfo->localGameInfo->pRAI.battingGoingOn == 0) {
		stateInfo->localGameInfo->pRAI.battingGoingOn = 1;
	}
	// so battingGoingOn goes 1 when batter arrives to its ready position and pitch is not in air.
	if(stateInfo->localGameInfo->pRAI.battingGoingOn == 1) {
		// we increase battingFrameCount from the moment the batter movement started.
		// used for animation and advancing.
		if(stateInfo->localGameInfo->pendingActionState.increaseBattingFrameCount == 1) {
			stateInfo->localGameInfo->pendingActionState.battingFrameCount += 1;
		}

		if(stateInfo->localGameInfo->pRAI.initBatter == 1) {
			// so the default is to swing, we change this to BUNT_ADVANCE or SPREAD_ADVANCE
			// if the circumstances need so
			stateInfo->localGameInfo->pendingActionState.batterAdvanceLimit = SWING_ADVANCE;
			// we start with natural speeds and positions
			stateInfo->localGameInfo->pendingActionState.batterAngle = 0.0f;
			stateInfo->localGameInfo->pendingActionState.batterAngleSpeed = 0;
			stateInfo->localGameInfo->pendingActionState.batterAdvance = 0.0f;
			stateInfo->localGameInfo->pendingActionState.batterAdvanceSpeed = 0.0f;
			// aren't moving yet
			stateInfo->localGameInfo->pendingActionState.batterMoving = 0;
			// swinging mode
			stateInfo->localGameInfo->pendingActionState.battingMode = BATTING_MODE_SWING;
			// animation is yet to start
			stateInfo->localGameInfo->pendingActionState.battingFrameCount = 0;
			// starting of the animation is yet to start
			stateInfo->localGameInfo->pendingActionState.increaseBattingFrameCount = 0;
			// not stopped yet
			stateInfo->localGameInfo->pendingActionState.battingStopped = 0;
			// update location and orientation once here
			stateInfo->localGameInfo->pendingActionState.updateBatterLocationAndOrientation = 1;
			stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->
			                                     pII.batterIndex].cPI.lastLastLocationUpdate = 1;
			// init done, so no need to do that anymore.
			stateInfo->localGameInfo->pRAI.initBatter = 0;
		}
		// update batters location and orientation when either advancing speed or angular speed
		// is nonzero. moving is 0 so location wont be updated with other players in different parts of code
		// and orientation is not updated elsewhere either as in the other part of code it is checked
		// if player's index is batterIndex before updating orientation. so its done here exclusively.
		if(stateInfo->localGameInfo->pendingActionState.batterAngleSpeed != 0 || stateInfo->localGameInfo->pendingActionState.batterAdvanceSpeed != 0) {
			// if the updated angle would be within limits, we can proceed updating the speed.
			// batterAngleSpeed is just 1, 0 or -1 and speed is really given by BATTER_ANGLE_SPEED_CONSTANT.
			if(stateInfo->localGameInfo->pendingActionState.batterAngle + stateInfo->localGameInfo->pendingActionState.batterAngleSpeed*BATTER_ANGLE_SPEED_CONSTANT <
			        BATTER_ANGLE_LIMIT && stateInfo->localGameInfo->pendingActionState.batterAngle + stateInfo->localGameInfo->pendingActionState.batterAngleSpeed*BATTER_ANGLE_SPEED_CONSTANT >
			        -BATTER_ANGLE_LIMIT ) {
				stateInfo->localGameInfo->pendingActionState.batterAngle += stateInfo->localGameInfo->pendingActionState.batterAngleSpeed*BATTER_ANGLE_SPEED_CONSTANT;
				stateInfo->localGameInfo->pendingActionState.updateBatterLocationAndOrientation = 1;
			}
			// if updated advanced location would be within limit that is originally SWING_ADVANCE but could
			// be changed to BUNT_ADVANCE given small power, then can proceed updating.
			if(stateInfo->localGameInfo->pendingActionState.batterAdvance + stateInfo->localGameInfo->pendingActionState.batterAdvanceSpeed < stateInfo->localGameInfo->pendingActionState.batterAdvanceLimit) {
				stateInfo->localGameInfo->pendingActionState.batterAdvance += stateInfo->localGameInfo->pendingActionState.batterAdvanceSpeed;
				stateInfo->localGameInfo->pendingActionState.updateBatterLocationAndOrientation = 1;
			}
		}
		// if need for update of location and orientation
		if(stateInfo->localGameInfo->pendingActionState.updateBatterLocationAndOrientation == 1) {
			float dx;
			float dz;
			float dx2;
			float dz2;
			// update lastLocation for smooth movement
			setVectorXZ(&(stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].tPI.lastLocation),
			            stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].tPI.location.x,
			            stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].tPI.location.z);
			// update location with sine and cosine to new location on the circle centered at pitch plate.
			// radius will be given by batterAdvance relative to batting radius
			// angle is given by batterAngle and the default ZERO_BATTING_ANGLE
			setVectorXZ(&(stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].tPI.location),
			            (float)(stateInfo->fieldPositions->pitchPlate.x + cos(ZERO_BATTING_ANGLE + stateInfo->localGameInfo->pendingActionState.batterAngle)*(BATTING_RADIUS *
			                    (1 - stateInfo->localGameInfo->pendingActionState.batterAdvance))),
			            (float)(stateInfo->fieldPositions->pitchPlate.z - sin(ZERO_BATTING_ANGLE + stateInfo->localGameInfo->pendingActionState.batterAngle)*(BATTING_RADIUS *
			                    (1 - stateInfo->localGameInfo->pendingActionState.batterAdvance))));
			// and then set the orientation of batter. here we just first select the base direction to be
			// vector from pitchplate to batter and then fix it a bit to make it look more realistic.
			dx = stateInfo->fieldPositions->pitchPlate.x - stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].tPI.location.x;
			dz = stateInfo->fieldPositions->pitchPlate.x - stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].tPI.location.z;
			dx2 = (float)(cos(BATTER_ANGLE_FIX)*dx - sin(BATTER_ANGLE_FIX)*dz);
			dz2 = (float)(sin(BATTER_ANGLE_FIX)*dx + cos(BATTER_ANGLE_FIX)*dz);
			setVectorXZ(&(stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].tPI.orientation), dx2, dz2);

			stateInfo->localGameInfo->pendingActionState.updateBatterLocationAndOrientation = 0;
		}


		// so now can actually start thinking about advancing as the ball is in air.
		if(stateInfo->localGameInfo->pRAI.pitchState == PITCH_STAGE_AIRBORNE && stateInfo->localGameInfo->pendingActionState.runBatFlag == 0) {
			// start the animation and advancing.
			if(stateInfo->localGameInfo->pendingActionState.increaseBattingFrameCount == 0) {
				stateInfo->localGameInfo->pendingActionState.increaseBattingFrameCount = 1;
			}
			// so at the beginning swing==BAT_ACTION_IDLE. advancing and animation doesnt necessarily start
			// immediately. if pitch is very high the batting animation will take a lot shorter time
			// than what it takes for ball to get down, so the animation and advancing will start a bit
			// later. meter updating on the hand will start immediately and power selection
			// will be available too.
			if(stateInfo->localGameInfo->aF.bTAF.swing == BAT_ACTION_IDLE) {
				float v = stateInfo->localGameInfo->ballInfo.velocity.y;
				// note decision s=0 makes the landing point actually to be in air,
				// but thats convenient for our purposes. so here we count
				// how many frames will it take for ball to go up and down again so that we can try
				// to time our batting advancing and animation accordingly. just solve 0 = s + vt + (1/2)at^2
				// and choose the correct branch and then add a little experience-based tweak.
				stateInfo->localGameInfo->pendingActionState.pitchFrameTime = calculate_pitch_frame_time(v, GRAVITY, 0.0f, PITCH_FRAME_TIME_TWEAK);
				// Here initialize meterCounter and meterCounter max in a way similar to how we initialized those in pitching.
				// relative distance from the end of meter to the indicator is the same.
				// difference is that these values are scaled a bit, to allow as slow movement of the indicator as possible
				// for batting.
				stateInfo->localGameInfo->pendingActionState.meterCounter = BAT_SWING_MAX - BAT_LOAD_MAX;
				stateInfo->localGameInfo->pendingActionState.meterCounterMax = BAT_SWING_MAX;
				// so allow user to select power
				stateInfo->localGameInfo->aF.bTAF.swing = BAT_ACTION_WAIT_FOR_BALL;
				// and set batHit and batMiss flags to zero. these are needed in other parts of
				// code.
				stateInfo->localGameInfo->pRAI.batHit = 0;
				stateInfo->localGameInfo->pRAI.batMiss = 0;
				// and batterReady is zero now. batter isnt ready to action
				// anymore, action is with him already.
				stateInfo->localGameInfo->pRAI.batterReady = 0;



			}
			// so if the batter is still not moving, we'll try to figure out if we should be moving.
			if(stateInfo->localGameInfo->pendingActionState.batterMoving == 0) {
				// so at the moment there is just enough frames left that if we start animation and advancing now the
				// ball will be at right height for the animation look correct.
				if(stateInfo->localGameInfo->pendingActionState.battingFrameCount > stateInfo->localGameInfo->pendingActionState.pitchFrameTime - BAT_ANIMATION_FRAME_HIT_COUNT) {
					// set batterMoving flag to 1 so that we can better handle starting points of the
					// animations
					stateInfo->localGameInfo->pendingActionState.batterMoving = 1;
					// so if we are still to swing
					// select corresponding animation
					if(stateInfo->localGameInfo->pendingActionState.battingMode == BATTING_MODE_SWING) {
						stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].cPI.model = PLAYER_ANIM_BAT_SWING_1;
					}
					// to bunt, select bunting animation
					else if(stateInfo->localGameInfo->pendingActionState.battingMode == BATTING_MODE_BUNT) {
						stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].cPI.model = PLAYER_ANIM_BAT_SWING_2;
					}
					// to stop the batting select the hands spread -animation
					else {
						stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].cPI.model = PLAYER_ANIM_BAT_SWING_3;
					}
					stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].cPI.animationStage = 0;
					stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].cPI.animationStageCount = 34;
					stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].cPI.animationFrequency = 3;
					// give player some speed to use for advancing.
					// basically we are just guessing some reasonable speed.
					// but it depends on frame hit count so its explicitly there
					// for clarity
					stateInfo->localGameInfo->pendingActionState.batterAdvanceSpeed = (float)GENERIC_BATTER_ADVANCE_SPEED_CONSTANT/BAT_ANIMATION_FRAME_HIT_COUNT;
				}
			}
			// if meter indicator reaches the limit, any batting won't happen and
			// we'll just show the animation of player spreading hands.
			if(stateInfo->localGameInfo->aF.bTAF.swing == BAT_ACTION_WAIT_FOR_BALL) {
				if(stateInfo->localGameInfo->pendingActionState.meterCounter - (BAT_SWING_MAX - BAT_LOAD_MAX) >= BAT_LOAD_MAX) {
					// change animation to hands spreading
					if(stateInfo->localGameInfo->pendingActionState.batterMoving == 1) {
						stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].cPI.model = PLAYER_ANIM_BAT_SWING_3;
					}
					// set battingMode to 2 just in case that the indicator went off the meter so early that
					// the batter didnt even start moving yet, so that we know to choose the right one when moving starts.
					stateInfo->localGameInfo->pendingActionState.battingMode = BATTING_MODE_STOP;
					// advance only a small bit
					stateInfo->localGameInfo->pendingActionState.batterAdvanceLimit = SPREAD_ADVANCE;
					// set flag to indicate that batting has stopped so that we there wont be checking for if the
					// bat has hit the ball
					stateInfo->localGameInfo->pendingActionState.battingStopped = 1;
					// set swing to BAT_ACTION_DONE to indicate that theres no
					// further functionality, we just wait for the animation to end.
					stateInfo->localGameInfo->aF.bTAF.swing = BAT_ACTION_DONE;
				}
			}

		}
		// so here we check if the animation has ended and if battingGoingOn is still on,
		// so that we dont do this but once. if it is, we set battingGoingOn to zero
		// and move the player towards ready position agian.
		if((stateInfo->localGameInfo->pendingActionState.battingFrameCount > stateInfo->localGameInfo->pendingActionState.pitchFrameTime -
		        BAT_ANIMATION_FRAME_HIT_COUNT + BAT_ANIMATION_FRAME_TOTAL_COUNT) &&
		        stateInfo->localGameInfo->pRAI.battingGoingOn == 1) {
			Vector3D target;
			stateInfo->localGameInfo->pRAI.battingGoingOn = 0;

			target.x = (float)(stateInfo->fieldPositions->pitchPlate.x + cos(ZERO_BATTING_ANGLE)*BATTING_RADIUS);
			target.z = (float)(stateInfo->fieldPositions->pitchPlate.z - sin(ZERO_BATTING_ANGLE)*BATTING_RADIUS);
			moveToTarget(stateInfo->localGameInfo->playerInfo, stateInfo->localGameInfo->pII.batterIndex, &target);
		}
		// so here we check if the bat hits. this event happens always the pitch has been in air
		else if(stateInfo->localGameInfo->pendingActionState.battingFrameCount > stateInfo->localGameInfo->pendingActionState.pitchFrameTime) {
			// so here we continue only if user hasn't decided to not to bat and if we havent bat already.
			if(stateInfo->localGameInfo->pendingActionState.battingStopped == 0 && stateInfo->localGameInfo->pRAI.batHit == 0 && stateInfo->localGameInfo->pRAI.batMiss == 0) {
				// if ball doesnt go too far away to left or right
				if(stateInfo->localGameInfo->ballInfo.location.x < BALL_MAX_OFFSET && stateInfo->localGameInfo->ballInfo.location.x > -BALL_MAX_OFFSET) {
					float verticalAngle;
					float horizontalAngle;
					float power;

					verticalAngle = calculate_batting_vertical_angle(stateInfo->localGameInfo->pendingActionState.selectedBattingPowerCount, stateInfo->localGameInfo->pendingActionState.selectedBattingAngleCount, stateInfo->localGameInfo->ballInfo.velocity.y, BAT_SWING_MAX, BAT_LOAD_MAX);

					// 2 to make it possible to bat to every direction on the field and a bit over.
					horizontalAngle = - stateInfo->localGameInfo->pendingActionState.batterAngle * 2;
					power = (float)stateInfo->localGameInfo->pendingActionState.selectedBattingPowerCount;
					// so now we have the two angles and the power. now we just need to find a way to
					// convert them nicely to velocities for the ball. there should be other factors
					// to influence velocity too.
					if(verticalAngle > VERTICAL_ANGLE_LIMIT || verticalAngle < - VERTICAL_ANGLE_LIMIT) {
						// we can also just miss, its not so uncommon!
						stateInfo->localGameInfo->pRAI.batMiss = 1;
					} else {
						int powerFactor;
						Vector3D velocity;

						// verticalAngle in interval -5..5
						// power in interval 0..36
						// horizontalAngle -0.38..0.38
						// ball location x  -1.0 ... 1.0
						// power depends on player's power attribute.
						powerFactor = stateInfo->localGameInfo->
						              playerInfo[stateInfo->localGameInfo->
						                         pII.batterIndex].bTPI.power;

						velocity = calculate_batted_ball_velocity(verticalAngle, horizontalAngle, power, powerFactor, stateInfo->localGameInfo->ballInfo.location.x);


						// make the ball fly in the air with new velocity
						genericSlingBall(&(stateInfo->localGameInfo->ballInfo), &(stateInfo->localGameInfo->pRAI), velocity.x, velocity.y, velocity.z);
						// and the sound
						stateInfo->playSoundEffect = SOUND_SWING;
						// bat hits
						stateInfo->localGameInfo->pRAI.batHit = 1;
						// firstCatchMade set to zero. its used for example to condition checking for runs
						// or out of bounds events.
						stateInfo->localGameInfo->gameControl.firstCatchMade = 0;
						// not a pitch anymore
						stateInfo->localGameInfo->pRAI.pitchState = PITCH_STAGE_NONE;
						// this batter has chance to make run now by running to third base.
						stateInfo->localGameInfo->gameModeState.canMakeRunOfHonor = 1;
						// no throw going on now
						stateInfo->localGameInfo->pRAI.throwGoingToBase = -1;
						// prepare for wounds
						stateInfo->localGameInfo->woundingState.woundingCatch = 0;
						stateInfo->localGameInfo->woundingState.woundingCatchHandled = 0;
						stateInfo->localGameInfo->gameControl.batterStartedRunning = 0;

						// move the batter if wanted

						if(stateInfo->localGameInfo->pRAI.willStartRunning[0] == 1) {
							int index = stateInfo->localGameInfo->pII.safeOnBaseIndex[0];
							stateInfo->localGameInfo->pRAI.willStartRunning[0] = 0;
							if(index != -1) {
								runToNextBase(stateInfo->localGameInfo, stateInfo->fieldPositions, index, 0);
								stateInfo->localGameInfo->pendingActionState.runBatFlag = 1;
							}
						}
					}
					// always when batting,
					// we get a strike
					stateInfo->localGameInfo->gameState.strikes += 1;
				}
				// if the ball went to far away and we still continued our batting
				// we just miss. set the flags, trigger the event and
				// add a strike.
				else {
					stateInfo->localGameInfo->pRAI.batMiss = 1;
					stateInfo->localGameInfo->gameState.strikes += 1;
				}
			}
		}
	}
}

void updateBattingMeter(StateInfo* stateInfo)
{
	// when power has yet to be selected but is to be selected we increase the counter
	// and map the value to proper floating point value to let us show it on the screen.
	if(stateInfo->localGameInfo->aF.bTAF.swing == BAT_ACTION_WAIT_FOR_BALL) {
		if(stateInfo->localGameInfo->pendingActionState.meterCounter < stateInfo->localGameInfo->pendingActionState.meterCounterMax) {
			stateInfo->localGameInfo->pendingActionState.meterCounter += 1;
		}
		stateInfo->localGameInfo->pRAI.swingMeterValue = calculate_power_meter_value(stateInfo->localGameInfo->pendingActionState.meterCounter, stateInfo->localGameInfo->pendingActionState.meterCounterMax);
	}
	// when power is selected but angle is to be selected
	else if(stateInfo->localGameInfo->aF.bTAF.swing == BAT_ACTION_ANGLE_WAIT) {
		// if the value is still valid, increase it
		if(stateInfo->localGameInfo->pendingActionState.meterCounter < stateInfo->localGameInfo->pendingActionState.meterCounterMax) {
			stateInfo->localGameInfo->pendingActionState.meterCounter += 1;
		}
		// otherwise select angle to be the maximum.
		else {
			stateInfo->localGameInfo->pendingActionState.selectedBattingAngleCount = stateInfo->localGameInfo->pendingActionState.meterCounterMax;
		}

		stateInfo->localGameInfo->pRAI.swingMeterValue = calculate_angle_meter_value(stateInfo->localGameInfo->pendingActionState.meterCounter, stateInfo->localGameInfo->pendingActionState.meterCounterMax, stateInfo->localGameInfo->pendingActionState.selectedBattingPowerCount, BAT_SWING_MAX, BAT_LOAD_MAX);
	}
}
