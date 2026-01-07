#include "globals.h"
#include "common_logic.h"
#include "vector_math.h"
#include "geometry.h"
#include "rng.h"
#include "base_logic.h"
#include "game_analysis.h"
#include "game_manipulation.h"
#include "base_control.h"

// Wrapper functions for backward compatibility
// These now call the pure vector_math functions
int isVectorSmallEnoughSphere(Vector3D *vector, float limit)
{
	return vec3_is_small_enough_sphere(vector, limit);
}

int isVectorSmallEnoughCircleXZV(Vector3D *vector, float limit)
{
	return vec3_is_small_enough_circle_xz_v(vector, limit);
}

int isVectorSmallEnoughCircleXZ(float dx, float dz, float limit)
{
	return vec3_is_small_enough_circle_xz(dx, dz, limit);
}

void setVectorXYZ(Vector3D *vector, float x, float y, float z)
{
	vec3_set_xyz(vector, x, y, z);
}

void setVectorV(Vector3D *vector1, Vector3D *vector2)
{
	vec3_set_from_vector(vector1, vector2);
}

void setVectorXZ(Vector3D *vector, float x, float z)
{
	vec3_set_xz(vector, x, z);
}

void addToVectorXZ(Vector3D *vector, float x, float z)
{
	vec3_add_xz(vector, x, z);
}

void addToVectorV(Vector3D *vector1, Vector3D *vector2)
{
	vec3_add_vector(vector1, vector2);
}
/*
	Index is index of the player in the playerInfo-array.
	stopMovement stops arrow key initiated movement. Many situations
	where the change of controlled player will leave the previously controlled
	player moving so this is commonly used to stop these ones.
*/
void stopMovement(PlayerInfo* playerInfo, int index)
{
	int j;
	if(index != -1) {
		for(j = 0; j < DIRECTION_COUNT; j++) {
			playerInfo[index].cTPI.movesToDirection[j] = 0;
		}
		// and after stopping movement, also ensure that no animation stays.
		if(playerInfo[index].cTPI.throwRecoil == 0) {
			playerInfo[index].cPI.model = PLAYER_ANIM_STAND_NO_BALL;
		}
		playerInfo[index].cPI.looksForTarget = 0;
		playerInfo[index].cPI.moving = 0;

		playerInfo[index].cPI.lastLastLocationUpdate = 1;
	}
}
// sometimes for example after a catch, we stop the the player, so that it wouldnt continue
// on its own. but it should still continue, as if player has the key presse down all the time.
// then we call this to start the movement again if there has been no release of the key inbetween
void smoothOutMovement(LocalGameInfo* localGameInfo)
{
	int j;
	for(j = 0; j < DIRECTION_COUNT; j++) {
		if(localGameInfo->aF.cTAF.move[j] == 2) {
			localGameInfo->aF.cTAF.move[j] = 1;
		}
	}
}
// this is for batting team players
void stopTargetLookingPlayer(PlayerInfo* playerInfo, PlayerRuntimeState* playerRuntime, int index)
{
	playerInfo[index].cPI.moving = 0;
	playerInfo[index].cPI.running = 0;
	playerInfo[index].cPI.looksForTarget = 0;
	playerInfo[index].cPI.lastLastLocationUpdate = 1;
}

void setOrientation(PlayerInfo* playerInfo, BallInfo* ballInfo, int i)
{
	// simply set player to orient towards the ball
	if(i != -1) {
		float dx = ballInfo->location.x - playerInfo[i].tPI.location.x;
		float dz = ballInfo->location.z - playerInfo[i].tPI.location.z;
		playerInfo[i].tPI.orientation.x = dx;
		playerInfo[i].tPI.orientation.z = dz;
	}
}

void runToTarget(PlayerInfo* playerInfo, int index, Vector3D *target)
{
	if(index != -1) {
		float dx;
		float dz;
		float speed;
		float norm;
		// so set target location
		playerInfo[index].tPI.targetLocation.x =
		    target->x;
		playerInfo[index].tPI.targetLocation.z =
		    target->z;
		// looking for target yeah
		playerInfo[index].cPI.looksForTarget = 1;
		// find the direction
		dx = playerInfo[index].tPI.targetLocation.x -
		     playerInfo[index].tPI.location.x;
		dz = playerInfo[index].tPI.targetLocation.z -
		     playerInfo[index].tPI.location.z;

		norm = geometry_distance_2d_xz(&playerInfo[index].tPI.targetLocation,
		                               &playerInfo[index].tPI.location);

		if(norm < EPSILON) norm = 1.0f;
		// set the velocity

		speed = BATTING_TEAM_RUN_FACTOR * RUN_SPEED + (RUN_SPEED/16)*playerInfo[index].bTPI.speed;
		setVectorXZ(&playerInfo[index].tPI.velocity, dx*speed/norm, dz*speed/norm);
		// we are running now, ( so for example our orientation wont change now unless we stop running)
		playerInfo[index].cPI.running = 1;
		// we are moving too
		playerInfo[index].cPI.moving = 1;
		// orientation to our direction
		playerInfo[index].tPI.orientation.x = dx;
		playerInfo[index].tPI.orientation.z = dz;
		// and set the running animation
		playerInfo[index].cPI.model = PLAYER_ANIM_RUN_BARE;
		playerInfo[index].cPI.animationStage = 0;
		playerInfo[index].cPI.animationStageCount = 20;
		playerInfo[index].cPI.animationFrequency = 3;
	}
}
/*
	this function puts player with index in the argument moving to some specified
	target by walking. is used for both fielders and batting team.
*/
void moveToTarget(PlayerInfo* playerInfo, int index, Vector3D *target)
{
	if(index != -1) {
		// cant start this if throw is going on. when ball is thrown the
		// control will often change automatically and we dont want the player to
		// start moving with walking animation before its throw animation has finished.
		if(playerInfo[index].cTPI.throwRecoil == 0) {
			float dx;
			float dz;
			float norm;
			playerInfo[index].tPI.targetLocation.x =
			    target->x;
			playerInfo[index].tPI.targetLocation.z =
			    target->z;
			// looksForTarget is important flag to avoid unnecessary
			// overhead of checking whether the player has
			// arrived to target location.
			playerInfo[index].cPI.looksForTarget = 1;
			// first find the unit vector for direction and then set player's
			// velocity to be the direction vector times the walk_speed.
			dx = playerInfo[index].tPI.targetLocation.x -
			     playerInfo[index].tPI.location.x;
			dz = playerInfo[index].tPI.targetLocation.z -
			     playerInfo[index].tPI.location.z;

			norm = geometry_distance_2d_xz(&playerInfo[index].tPI.targetLocation,
			                               &playerInfo[index].tPI.location);

			if(norm < EPSILON) norm = 1.0f;
			setVectorXZ(&playerInfo[index].tPI.velocity, dx*WALK_SPEED/norm, dz*WALK_SPEED/norm);
			// if the player for some reason was running before this, set that to 0.
			// could happen for example if baserunner gets out.
			playerInfo[index].cPI.running = 0;
			// and set moving to 1 so that player's location will be updated.
			playerInfo[index].cPI.moving = 1;

			// choose different walking animation for fielders and batting team.
			if(index < PLAYERS_IN_TEAM + JOKER_COUNT) {
				playerInfo[index].cPI.model = PLAYER_ANIM_WALK_BARE;
			} else {
				playerInfo[index].cPI.model = PLAYER_ANIM_WALK_NO_BALL;
			}
			playerInfo[index].cPI.animationStage = 0;
			playerInfo[index].cPI.animationStageCount = 16;
			playerInfo[index].cPI.animationFrequency = 3;

		}
	}
}
// so this function is called when outs happen but also when wounds happen. it just moves
// players out of the field and then to homebase.
void movePlayerOut(PlayerInfo* playerInfo, PlayerRuntimeState* playerRuntime, FieldPositions* fieldPositions, int index)
{
	Vector3D target;
	// we are walking
	playerInfo[index].cPI.running = 0;
	// left or right?
	if(playerInfo[index].tPI.location.x < 0) {
		target.x = fieldPositions->leftPoint.x - 5.0f;
		target.z = fieldPositions->leftPoint.z + 10.0f;
	} else {
		target.x = fieldPositions->rightPoint.x + 5.0f;
		target.z = fieldPositions->rightPoint.z + 10.0f;
	}
	// path point not passed yet.
	playerRuntime[index].passedPathPoint = 0;

	// and move to target takes care of the rest.
	moveToTarget(playerInfo, index, &target);
}
// so we have the ranked fielders-array and those are players who are somewhat important in relation
// to ball's current location and velocity. so its natural that we have those players moving to catch
// the ball.
void moveRankedToCatch(LocalGameInfo* localGameInfo)
{
	int i;

	for(i = 0; i < RANKED_FIELDERS_COUNT; i++) {
		int index = localGameInfo->pII.fielderRankedIndices[i];
		// controlled player wont get the chance.
		if(index != localGameInfo->pII.controlIndex && localGameInfo->playerInfo[index].
		        cTPI.replacingStage == REPLACEMENT_IDLE) {
			// if we are throwing ( towards a base ) we dont want the baseman there to start moving
			// as it would be nice that he is at the base when ball is caught if baserunner is going there.
			if(localGameInfo->pRAI.throwGoingToBase == -1 ||
			        (localGameInfo->pII.catcherOnBaseIndex[localGameInfo->pRAI.throwGoingToBase] != index)) {
				int k;
				int done = 0;
				// and we have special condition not to move any basemen
				// automatically at all.
				for(k = 0; k < BASE_COUNT; k++) {
					if(localGameInfo->pII.catcherOnBaseIndex[k] == index) {
						done = 1;
					}
				}
				if(done == 0) {
					// set busycatching flag, and move player towards the target point
					// that has been specified beforehand.
					localGameInfo->playerInfo[localGameInfo->pII.fielderRankedIndices[i]].cTPI.busyCatching = 1;
					moveToTarget(localGameInfo->playerInfo, index, &localGameInfo->cameraState.targetPoint);
				}
			}
		}
	}
}

void runToNextBase(LocalGameInfo* localGameInfo, FieldPositions* fieldPositions, int index, BaseID base)
{
	if(index != -1) {
		Vector3D target;
		// first we select the target corresponding to base argument
		if(base == BASE_HOME) {
			if(localGameInfo->pRAI.batterCanAdvance == 0) return;
			target.x = fieldPositions->firstBaseRun.x;
			target.z = fieldPositions->firstBaseRun.z;
			// here as it is the batter, we'll also stop any batting to be able to run freely.
			localGameInfo->pRAI.batterReady = 0;
			localGameInfo->pRAI.battingGoingOn = 0;
			localGameInfo->gameControl.batterStartedRunning = 1;
		} else if(base == BASE_FIRST) {
			target.x = fieldPositions->secondBaseRun.x;
			target.z = fieldPositions->secondBaseRun.z;
		} else if(base == BASE_SECOND) {
			target.x = fieldPositions->thirdBaseRun.x;
			target.z = fieldPositions->thirdBaseRun.z;
		} else if(base == BASE_THIRD) {
			// if we are running home, there is the "flag" point, and we must change the direction there.
			// how it matters here is that if we have already passed the flag, we must run towards homebase,
			// if not, we must run towards flag.
			if(localGameInfo->playerRuntime[index].passedPathPoint == 0) {
				target.x = fieldPositions->runLeftPoint.x;
				target.z = fieldPositions->runLeftPoint.z;

				localGameInfo->cameraState.homeRunCameraFlag = 1;
			} else if(localGameInfo->playerRuntime[index].passedPathPoint == 1) {
				target.x = fieldPositions->homeRunPoint.x;
				target.z = fieldPositions->homeRunPoint.z;
			} else {
				return;
			}


		} else {
			return;
		}
		// and set it so that next player has to have a will of his own to run
		localGameInfo->pRAI.willStartRunning[base] = 0;
		// set state to running, BUT only if we aren't already WOUNDED or OUT (which are terminal/override states)
		if (localGameInfo->playerInfo[index].bTPI.state != PLAYER_STATE_WOUNDED &&
		        localGameInfo->playerInfo[index].bTPI.state != PLAYER_STATE_OUT) {
			localGameInfo->playerInfo[index].bTPI.state = PLAYER_STATE_RUNNING;
		}
		// and we are moving forward
		localGameInfo->playerRuntime[index].goingForward = 1;
		// and runToTarget can continue the job with index and the already set target.
		runToTarget(localGameInfo->playerInfo, index, &target);
	}
}

void runToPreviousBase(LocalGameInfo* localGameInfo, FieldPositions* fieldPositions, int index, BaseID base)
{
	if(index != -1) {
		Vector3D target;
		// run to previous base works similarly to run to next base.
		// starting point here is that we arent on any base, and the base variable is telling us
		// the previous base
		if(base == BASE_HOME) {
			// so when batter returns, he will go to his ready position again.
			target.x = (float)(fieldPositions->pitchPlate.x + cos(ZERO_BATTING_ANGLE)*BATTING_RADIUS);
			target.z = (float)(fieldPositions->pitchPlate.z - sin(ZERO_BATTING_ANGLE)*BATTING_RADIUS);
		} else if(base == BASE_FIRST) {
			target.x = fieldPositions->firstBaseRun.x;
			target.z = fieldPositions->firstBaseRun.z;
		} else if(base == BASE_SECOND) {
			target.x = fieldPositions->secondBaseRun.x;
			target.z = fieldPositions->secondBaseRun.z;
		} else if(base == BASE_THIRD) {
			// here we again select the target by our current location relative to flag
			if(localGameInfo->playerRuntime[index].passedPathPoint == 0) {
				target.x = fieldPositions->thirdBaseRun.x;
				target.z = fieldPositions->thirdBaseRun.z;
			} else if(localGameInfo->playerRuntime[index].passedPathPoint == 1) {
				target.x = fieldPositions->runLeftPoint.x;
				target.z = fieldPositions->runLeftPoint.z;
			} else {
				return;
			}

		} else {
			return;
		}

		// and set it so that next player has to have a will of his own to run
		localGameInfo->pRAI.willStartRunning[base] = 0;
		// we arent going forward
		localGameInfo->playerRuntime[index].goingForward = 0;
		// set state to running, BUT only if we aren't already WOUNDED or OUT
		if (localGameInfo->playerInfo[index].bTPI.state != PLAYER_STATE_WOUNDED &&
		        localGameInfo->playerInfo[index].bTPI.state != PLAYER_STATE_OUT) {
			localGameInfo->playerInfo[index].bTPI.state = PLAYER_STATE_RUNNING;
		}
		// and runToTarget can handle the rest

		runToTarget(localGameInfo->playerInfo, index, &target);
	}
}

void lead(PlayerInfo* playerInfo, PlayerRuntimeState* playerRuntime, FieldPositions* fieldPositions, int index)
{
	if(index != -1) {
		int done = 0;
		Vector3D target;
		// now to lead we must be either on first base or second base, as it doesnt make much sense in
		// third base nor in homebase.
		if(playerInfo[index].bTPI.baseId == BASE_FIRST) {
			// lead target is selected by adding a small step to current location to next bases' direction
			// using firstBase  instead of location in the difference is because we want the step size to stay
			// same
			target.x = playerInfo[index].tPI.location.x + LEAD_STEP*(fieldPositions->secondBaseRun.x -
			           fieldPositions->firstBaseRun.x);
			target.z = playerInfo[index].tPI.location.z + LEAD_STEP*(fieldPositions->secondBaseRun.z -
			           fieldPositions->firstBaseRun.z);
			// if we go over half way, we disallow any leading, you should just run from there.
			if(playerInfo[index].tPI.location.x > fieldPositions->firstBaseRun.x +
			        0.5f*(fieldPositions->secondBaseRun.x - fieldPositions->firstBaseRun.x))
				done = 1;
		} else if(playerInfo[index].bTPI.baseId == BASE_SECOND) {
			// same as in previous but from second to third base
			target.x = playerInfo[index].tPI.location.x + LEAD_STEP*(fieldPositions->thirdBaseRun.x -
			           fieldPositions->secondBaseRun.x);
			target.z = playerInfo[index].tPI.location.z + LEAD_STEP*(fieldPositions->thirdBaseRun.z -
			           fieldPositions->secondBaseRun.z);
			if(playerInfo[index].tPI.location.x < fieldPositions->secondBaseRun.x +
			        0.5f*(fieldPositions->thirdBaseRun.x - fieldPositions->secondBaseRun.x))
				done = 1;
		} else {
			done = 1;
		}
		// if our
		if(done == 0) {
			// walk to our target
			moveToTarget(playerInfo, index, &target);
			// now we in fact are leading
			playerInfo[index].bTPI.state = PLAYER_STATE_LEADING;
			// but we dont set going forward flag.
			playerRuntime[index].goingForward = 0;
		}

	}
}
// so a little function to check if ball's x and z coordinates indicate that ball is out of the
// playing field.
int checkIfBallIsOutOfBounds(BallInfo* ballInfo, FieldPositions* fieldPositions)
{
	int value = 1;
	// first, is ball behind the line at the back, or too much at right or too much at left
	// or in front of the homeline
	// if not, continue
	if(ballInfo->location.z > fieldPositions->backLeftPoint.z &&
	        ballInfo->location.x < fieldPositions->backRightPoint.x &&
	        ballInfo->location.x > fieldPositions->backLeftPoint.x &&
	        ballInfo->location.z < HOME_LINE_Z) {
		float z0 = 1.0f;
		float x = fieldPositions->rightPoint.x;
		float z = fieldPositions->rightPoint.z - z0;
		float slope1 = z/x;
		float slope2;
		x = -fieldPositions->leftPoint.x;
		z = -(fieldPositions->leftPoint.z - z0);
		slope2 = z/x;
		// then here we just have basic line equations to check if the ball is
		// out or inside the lines from pitchPlate to rightPoint and leftPoint.
		if(ballInfo->location.z - slope1*ballInfo->location.x - z0 < 0 &&
		        ballInfo->location.z - slope2*ballInfo->location.x - z0 < 0) {
			value = 0;
		}
	}
	if(value == 1) {
		ballInfo->hasHitGroundOutOfBounds = 1;
	}
	return value;
}

void changePlayer(LocalGameInfo* localGameInfo)
{
	// this is called by user explicitly and sometimes after updating changePlayer lists.
	// so cant change pitch if pitch is going on
	if(localGameInfo->pRAI.pitchState == PITCH_STAGE_NONE) {
		// player will start randomly floating after control changes to next player.
		if(localGameInfo->pII.controlIndex != -1) {
			stopMovement(localGameInfo->playerInfo, localGameInfo->pII.controlIndex);
		}

		if(localGameInfo->pII.fielderRankedIndices[localGameInfo->pII.changePlayerArrayIndex] != -1) {			// set control to new index from the array
			localGameInfo->pII.controlIndex = localGameInfo->pII.fielderRankedIndices[localGameInfo->pII.changePlayerArrayIndex];
			// and set him to run
			localGameInfo->playerInfo[localGameInfo->pII.controlIndex].cPI.running = 1;
			// logic about coming back from replacement if controlled
			if(localGameInfo->playerInfo[localGameInfo->pII.controlIndex].cTPI.replacingStage == REPLACEMENT_ACTIVE) {
				localGameInfo->playerInfo[localGameInfo->pII.controlIndex].cTPI.replacingStage = REPLACEMENT_IDLE;
			}
			// same for busyCatching.
			if(localGameInfo->playerInfo[localGameInfo->pII.controlIndex].cTPI.busyCatching == 1) {
				localGameInfo->playerInfo[localGameInfo->pII.controlIndex].cTPI.busyCatching = 0;
			}
			// move others to catch( so that the previous one for example doesnt just stop if its near the ball )
			moveRankedToCatch(localGameInfo);
			// stop player who has the selection now
			stopMovement(localGameInfo->playerInfo, localGameInfo->pII.controlIndex);
			// but start moving again if movement key being held at the same time. for smooth movement.
			// smoothOutMovement still needs StateInfo due to ActionFlags being in Local but also needing KeyStates?
			// Wait, smoothOutMovement implementation:
			/*
			void smoothOutMovement(StateInfo* stateInfo)
			{
				int j;
				for(j = 0; j < DIRECTION_COUNT; j++) {
					if(stateInfo->localGameInfo->aF.cTAF.move[j] == 2) {
						stateInfo->localGameInfo->aF.cTAF.move[j] = 1;
					}
				}
			}
			*/
			// It only uses LocalGameInfo! I'll narrow it later.
		}
	}

}

void prepareBatter(LocalGameInfo* localGameInfo)
{
	if(localGameInfo->pII.batterIndex != -1) {
		// batter ready model
		localGameInfo->playerInfo[localGameInfo->pII.batterIndex].cPI.model = PLAYER_ANIM_BATTER_READY;
		// can pitch now
		localGameInfo->pRAI.batterReady = 1;
		// waiting for pitch to go in air before starting the batting movement
		localGameInfo->aF.bTAF.swing = 0;
		// batterIndex has been selected before calling this function
		localGameInfo->referee.battingPlayers[localGameInfo->pII.batterIndex].currentSafetyBase = BASE_HOME;
		// and initialize batter so that everything is ready to go.
		localGameInfo->pRAI.initBatter = 1;
	}
}
// so here we calculate index and base of the player who is the leadrunner so that
// we can move him if thats the decision.
void calculateFreeWalk(LocalGameInfo* localGameInfo)
{
	int i;
	BaseID maxBaseAtPitchStart = BASE_NONE;
	BaseID maxBaseId = BASE_NONE;
	int maxIndex = -1;
	// we go throush every (nonwounded) candidate and check who has the biggest base value
	// if there are many of those who have same base value, we will pick the one who has
	// the biggest baseAtPitchStart value. if both are same for some reason
	// then the selection will be quite random but shouldn't happen often and shouldn't be a big deal
	// either.
	for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		int index = i;
		if(localGameInfo->playerInfo[index].bTPI.baseId != BASE_NONE && localGameInfo->playerInfo[index].bTPI.state != PLAYER_STATE_WOUNDED) {
			BaseID currentBaseId = localGameInfo->playerInfo[index].bTPI.baseId;

			// SANITY CHECK: If player is at HOME base, they MUST be the active batter.
			if (currentBaseId == BASE_HOME && index != localGameInfo->pII.batterIndex) {
				printf("[CRITICAL LOGIC ERROR] Ghost Runner detected! Player %d is at BASE_HOME but batterIndex is %d. Ignoring.\n", index, localGameInfo->pII.batterIndex);
				continue;
			}

			// Use base_cmp to compare bases semantically (replaces >=)
			if(base_cmp(currentBaseId, maxBaseId) >= 0) {
				if(currentBaseId == maxBaseId) {
					if(base_cmp(localGameInfo->referee.battingPlayers[index].baseAtPitchStart, maxBaseAtPitchStart) > 0) {
						maxBaseId = currentBaseId;
						maxBaseAtPitchStart = localGameInfo->referee.battingPlayers[index].baseAtPitchStart;
						maxIndex = index;
					}
				} else {
					maxBaseId = currentBaseId;
					maxBaseAtPitchStart = localGameInfo->referee.battingPlayers[index].baseAtPitchStart;
					maxIndex = index;
				}
			}

		}
	}
	localGameInfo->gameControl.freeWalkIndex = maxIndex;
	if(maxIndex != -1) localGameInfo->gameControl.freeWalkBase = localGameInfo->playerInfo[maxIndex].bTPI.baseId;
	else localGameInfo->gameControl.freeWalkBase = BASE_NONE;
}
/*
	Here we initialize all the locations and velocities so that players will be in their correct
	positions and orientations on the field and ball looks like its thrown to pitcher. Models
	are updated also. Before calling this the fielding team must have its position-attributes filled.
*/
void initializeSpatialPlayerInformation(LocalGameInfo* localGameInfo, FieldPositions* fieldPositions, unsigned int* rng_seed)
{
	int i;
	Vector3D* fieldPosition;
	int battingTeamPlacement[PLAYERS_IN_TEAM + JOKER_COUNT];
	// create array that has 0-11 in random order to give batting players in home
	// circle a nicer visual feelin'
	for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		battingTeamPlacement[i] = -1;
	}
	i = 0;
	while(i < PLAYERS_IN_TEAM + JOKER_COUNT) {
		int random = seeded_rand(rng_seed, PLAYERS_IN_TEAM + JOKER_COUNT);
		if(battingTeamPlacement[random] == -1) {
			battingTeamPlacement[random] = i;
			i++;
		}
	}
	// when out of bounds situation or inning ends, we swing the ball in the air and let the pitcher
	// catch it
	localGameInfo->ballInfo.velocity.x = BALL_INIT_SPEED_X;
	localGameInfo->ballInfo.velocity.y = BALL_INIT_SPEED_Y;
	localGameInfo->ballInfo.velocity.z = BALL_INIT_SPEED_Z;
	localGameInfo->ballInfo.location.x = BALL_INIT_LOCATION_X;
	localGameInfo->ballInfo.location.y = BALL_INIT_LOCATION_Y;
	localGameInfo->ballInfo.location.z = BALL_INIT_LOCATION_Z;

	localGameInfo->ballInfo.lastLocation.x = localGameInfo->ballInfo.location.x;
	localGameInfo->ballInfo.lastLocation.y = localGameInfo->ballInfo.location.y;
	localGameInfo->ballInfo.lastLocation.z = localGameInfo->ballInfo.location.z;
	// set locations and models for batting team players
	for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		float radiusFix = (float)(1.0f + fabs(5.5f - battingTeamPlacement[i])/20.0f);
		localGameInfo->playerInfo[i].cPI.model = PLAYER_ANIM_STAND_BARE;

		localGameInfo->playerInfo[i].tPI.homeLocation.x = (float)(fieldPositions->pitchPlate.x +
		    (HOME_RADIUS) * radiusFix * cos(PI - (battingTeamPlacement[i]+1)*PI/(PLAYERS_IN_TEAM + JOKER_COUNT + 1)));
		localGameInfo->playerInfo[i].tPI.homeLocation.y = BALL_HEIGHT_WITH_PLAYER;
		localGameInfo->playerInfo[i].tPI.homeLocation.z = (float)(fieldPositions->pitchPlate.z +
		    (HOME_RADIUS) * radiusFix * sin(PI - (battingTeamPlacement[i]+1)*PI/(PLAYERS_IN_TEAM + JOKER_COUNT + 1)));
		localGameInfo->playerInfo[i].tPI.location.x = localGameInfo->playerInfo[i].tPI.homeLocation.x;
		localGameInfo->playerInfo[i].tPI.location.y = localGameInfo->playerInfo[i].tPI.homeLocation.y;
		localGameInfo->playerInfo[i].tPI.location.z = localGameInfo->playerInfo[i].tPI.homeLocation.z;
		localGameInfo->playerInfo[i].tPI.orientation.x = localGameInfo->ballInfo.location.x - localGameInfo->playerInfo[i].tPI.location.x;
		localGameInfo->playerInfo[i].tPI.orientation.y = localGameInfo->ballInfo.location.y - localGameInfo->playerInfo[i].tPI.location.y;
		localGameInfo->playerInfo[i].tPI.orientation.z = localGameInfo->ballInfo.location.z - localGameInfo->playerInfo[i].tPI.location.z;
		localGameInfo->playerInfo[i].tPI.lastLocation.x = localGameInfo->playerInfo[i].tPI.location.x;
		localGameInfo->playerInfo[i].tPI.lastLocation.y = localGameInfo->playerInfo[i].tPI.location.y;
		localGameInfo->playerInfo[i].tPI.lastLocation.z = localGameInfo->playerInfo[i].tPI.location.z;
	}
	// set locations and models for fielders. here we need positions set.
	for(i = 12; i < 2*PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		localGameInfo->playerInfo[i].cPI.model = PLAYER_ANIM_STAND_NO_BALL;

		switch(i-12) {
		case 0:
			fieldPosition = &(fieldPositions->pitcher);
			break;
		case 1:
			fieldPosition = &(fieldPositions->firstBase);
			break;
		case 2:
			fieldPosition = &(fieldPositions->secondBase);
			break;
		case 3:
			fieldPosition = &(fieldPositions->thirdBase);
			break;
		case 4:
			fieldPosition = &(fieldPositions->bottomRightCatcher);
			break;
		case 5:
			fieldPosition = &(fieldPositions->middleLeftCatcher);
			break;
		case 6:
			fieldPosition = &(fieldPositions->middleRightCatcher);
			break;
		case 7:
			fieldPosition = &(fieldPositions->backLeftCatcher);
			break;
		case 8:
			fieldPosition = &(fieldPositions->backRightCatcher);
			break;
		default:
			fieldPosition = &(fieldPositions->pitchPlate);
			break;

		}
		localGameInfo->playerInfo[i].tPI.homeLocation.x = fieldPosition->x;
		localGameInfo->playerInfo[i].tPI.homeLocation.y = fieldPosition->y;
		localGameInfo->playerInfo[i].tPI.homeLocation.z = fieldPosition->z;

		localGameInfo->playerInfo[i].tPI.location.x = localGameInfo->playerInfo[i].tPI.homeLocation.x;
		localGameInfo->playerInfo[i].tPI.location.y = localGameInfo->playerInfo[i].tPI.homeLocation.y;
		localGameInfo->playerInfo[i].tPI.location.z = localGameInfo->playerInfo[i].tPI.homeLocation.z;

		localGameInfo->playerInfo[i].tPI.orientation.x = localGameInfo->ballInfo.location.x - localGameInfo->playerInfo[i].tPI.location.x;
		localGameInfo->playerInfo[i].tPI.orientation.y = localGameInfo->ballInfo.location.y - localGameInfo->playerInfo[i].tPI.location.y;
		localGameInfo->playerInfo[i].tPI.orientation.z = localGameInfo->ballInfo.location.z - localGameInfo->playerInfo[i].tPI.location.z;

		localGameInfo->playerInfo[i].tPI.lastLocation.x = localGameInfo->playerInfo[i].tPI.location.x;
		localGameInfo->playerInfo[i].tPI.lastLocation.y = localGameInfo->playerInfo[i].tPI.location.y;
		localGameInfo->playerInfo[i].tPI.lastLocation.z = localGameInfo->playerInfo[i].tPI.location.z;
	}
	// these are set for every player.
	for(i = 0; i < 2*PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		localGameInfo->playerInfo[i].tPI.velocity.x = 0.0f;
		localGameInfo->playerInfo[i].tPI.velocity.y = 0.0f;
		localGameInfo->playerInfo[i].tPI.velocity.z = 0.0f;
		localGameInfo->playerInfo[i].tPI.targetLocation.x = 0.0f;
		localGameInfo->playerInfo[i].tPI.targetLocation.y = 0.0f;
		localGameInfo->playerInfo[i].tPI.targetLocation.z = 0.0f;
	}
}
// here we initialize players' stat information, and team information, whether they are joker or not,
// their number etc. all is information that is not going to be reinitialized when out of bounds -
// situation happens
void initializeInningPermanentPlayerInformation(LocalGameInfo* localGameInfo, GlobalGameInfo* globalGameInfo, TeamData* teamData)
{
	int battingTeamIndex = (globalGameInfo->
	                        inning+globalGameInfo->playsFirst+globalGameInfo->period)%2;
	int i;
	int jokerCounter = 0;

	// initialize batting team numbers and jokerness
	for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		if(i < PLAYERS_IN_TEAM) {
			localGameInfo->playerInfo[globalGameInfo->teams[battingTeamIndex].
			                          batterOrder[i]].bTPI.number = i + 1;
			localGameInfo->playerInfo[globalGameInfo->teams[battingTeamIndex].
			                          batterOrder[i]].bTPI.joker = JOKER_REGULAR;
		} else {
			localGameInfo->playerInfo[globalGameInfo->teams[battingTeamIndex].
			                          batterOrder[i]].bTPI.number = 0;
			localGameInfo->playerInfo[globalGameInfo->teams[battingTeamIndex].
			                          batterOrder[i]].bTPI.joker = JOKER_AVAILABLE;
		}
	}
	// initialize batting team
	for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		localGameInfo->playerInfo[i].cPI.team = TEAM_BATTING;

		if(localGameInfo->playerInfo[i].bTPI.joker == JOKER_AVAILABLE) {
			localGameInfo->pII.jokerIndices[jokerCounter] = i;
			jokerCounter++;
		}
		localGameInfo->playerInfo[i].bTPI.name = teamData[(globalGameInfo->teams[battingTeamIndex]
		    .value - 1)].players[i].name;
		localGameInfo->playerInfo[i].bTPI.power = teamData[(globalGameInfo->teams[battingTeamIndex]
		    .value - 1)].players[i].power;
		localGameInfo->playerInfo[i].bTPI.speed = teamData[(globalGameInfo->teams[battingTeamIndex]
		    .value - 1)].players[i].speed;


	}
	// initialize fielders
	for(i = 12; i < 2*PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		localGameInfo->playerInfo[i].cPI.team = TEAM_CATCHING;

		// here we set catcherOnBaseIndices and catcherReplacerOnBaseIndices.
		switch(i-12) {
		case 0:
			localGameInfo->pII.catcherOnBaseIndex[0] = i;
			break;
		case 1:
			localGameInfo->pII.catcherOnBaseIndex[1] = i;
			break;
		case 2:
			localGameInfo->pII.catcherOnBaseIndex[2] = i;
			break;
		case 3:
			localGameInfo->pII.catcherOnBaseIndex[3] = i;
			break;
		case 4:
			localGameInfo->pII.catcherReplacerOnBaseIndex[0] = i;
			localGameInfo->pII.catcherReplacerOnBaseIndex[1] = i;
			break;
		case 5:
			localGameInfo->pII.catcherReplacerOnBaseIndex[3] = i;
			break;
		case 6:
			localGameInfo->pII.catcherReplacerOnBaseIndex[2] = i;
			break;

		}
	}
}
// information that can be flushed
void initializeNonCriticalPlayerInformation(LocalGameInfo* localGameInfo)
{
	int i, j;
	for( i = 0; i < 2*PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		// MILESTONE 7.5: Initialize control state
		localGameInfo->playerRuntime[i].arrivedToBase = 0;
		localGameInfo->playerRuntime[i].woundedApply = 0;
		localGameInfo->playerRuntime[i].passedPathPoint = 0;
		localGameInfo->playerRuntime[i].goingForward = 0;
		localGameInfo->playerRuntime[i].hasMadeRunOnThirdBase = 0;

		localGameInfo->playerInfo[i].cPI.animationFrequency = 1;
		localGameInfo->playerInfo[i].cPI.animationStage = 0;
		localGameInfo->playerInfo[i].cPI.animationStageCount = 0;
		localGameInfo->playerInfo[i].cPI.moving = 0;
		localGameInfo->playerInfo[i].cPI.running = 0;
		localGameInfo->playerInfo[i].cPI.looksForTarget = 0;
		localGameInfo->playerInfo[i].cPI.lastLastLocationUpdate = 1;

		// Critical: throwRecoil must be 0 for all players so moveToTarget doesn't block
		localGameInfo->playerInfo[i].cTPI.throwRecoil = 0;

		if( i >= PLAYERS_IN_TEAM + JOKER_COUNT) {

			localGameInfo->playerInfo[i].cTPI.isNearHomeLocation = 1;
			localGameInfo->playerInfo[i].cTPI.replacingStage = REPLACEMENT_IDLE;
			localGameInfo->playerInfo[i].cTPI.replacingBase = BASE_NONE;
			localGameInfo->playerInfo[i].cTPI.busyCatching = 0;
			// throwRecoil already set above
			// initialize fielderRankedIndices with the indices of five first
			// players in positional order.
			if(i-12 < RANKED_FIELDERS_COUNT) {
				localGameInfo->pII.fielderRankedIndices
				[i-12] = i;
			}

			for(j = 0; j < DIRECTION_COUNT; j++) {
				localGameInfo->playerInfo[i].cTPI.movesToDirection[j] = 0;
			}
		} else {
			localGameInfo->playerInfo[i].bTPI.state = PLAYER_STATE_IDLE;
			localGameInfo->playerInfo[i].bTPI.baseId = BASE_NONE;
		}
	}

}
// this information is important for correct continuity after foul play.
void initializeCriticalBattingTeamInformation(LocalGameInfo* localGameInfo)
{
	int i;
	for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		localGameInfo->referee.battingPlayers[i].baseAtPitchStart = BASE_NONE;
	}
}
// ball flags
void initializeBallInfo(LocalGameInfo* localGameInfo)
{
	localGameInfo->ballInfo.visible = 1;
	localGameInfo->ballInfo.moving = 1;
	localGameInfo->ballInfo.hasHitGround = 0;
	localGameInfo->ballInfo.onGround = 0;
	localGameInfo->ballInfo.hitsGroundToUnWound = 0;
	localGameInfo->ballInfo.hasHitGroundOutOfBounds = 0;
	localGameInfo->ballInfo.needsMoveUpdate = 0;
	localGameInfo->ballInfo.lastLastLocationUpdate = 0;
}
// action flag initialization
void initializeActionInfo(LocalGameInfo* localGameInfo)
{
	int i;

	for(i = 0; i < BASE_COUNT; i++) {
		localGameInfo->aF.bTAF.baseRun[i] = 0;
	}
	localGameInfo->aF.bTAF.chooseBatter = 0;
	localGameInfo->aF.bTAF.takeFreeWalk = 0;
	localGameInfo->aF.bTAF.swing = 0;
	localGameInfo->aF.bTAF.increaseBatterAngle = 0;
	localGameInfo->aF.bTAF.decreaseBatterAngle = 0;

	for(i = 0; i < BASE_COUNT; i++) {
		localGameInfo->aF.cTAF.move[i] = 0;
		localGameInfo->aF.cTAF.throwToBase[i] = 0;
	}
	localGameInfo->aF.cTAF.changePlayer = 0;
	localGameInfo->aF.cTAF.dropBall = 0;
	localGameInfo->aF.cTAF.pitch = 0;
	localGameInfo->aF.cTAF.actionKeyLock = 0;
}
// these can be flushed
void initializeTemporaryGameAnalysisInfo(LocalGameInfo* localGameInfo)
{
	localGameInfo->gameControl.freeWalkCalculationMade = 1;
	localGameInfo->gameControl.waitingForBatterDecision = 0;
	localGameInfo->gameControl.waitingForFreeWalkDecision = 0;
	localGameInfo->gameState.outOfBounds = 0;
	localGameInfo->playerCounters.noMorePlayers = 0;
	localGameInfo->gameState.ballHome = 0;
	localGameInfo->gameState.endPeriod = 0;
	localGameInfo->woundingState.woundingCatch = 0;
	localGameInfo->woundingState.woundingCatchHandled = 0;
	localGameInfo->gameControl.batterStartedRunning = 0;

	localGameInfo->gameState.event = EVENT_NONE;
	localGameInfo->gameControl.checkForRun = 0;
	localGameInfo->gameControl.freeWalkIndex = -1;
	localGameInfo->gameControl.freeWalkBase = -1;
	localGameInfo->gameControl.playerArrivedToBase = 0;
	localGameInfo->gameControl.firstCatchMade = 0;
	localGameInfo->gameControl.pause = 0;

	initGameAnalysis(&(localGameInfo->gameFlowState));
	initGameManipulation(&(localGameInfo->gameFlowState));

	localGameInfo->gameModeState.forceNextPair = 0;
	localGameInfo->cameraState.homeRunCameraFlag = 0;
	localGameInfo->gameModeState.canMakeRunOfHonor = 0;
	localGameInfo->cameraState.targetPoint.x = 0.0f;
	localGameInfo->cameraState.targetPoint.y = 0.0f;
	localGameInfo->cameraState.targetPoint.z = 0.0f;
}
// these should be kept when foul play
void initializeCriticalGameInfo(LocalGameInfo* localGameInfo, GlobalGameInfo* globalGameInfo)
{
	int battingTeamIndex = (globalGameInfo->
	                        inning+globalGameInfo->playsFirst+globalGameInfo->period)%2;
	localGameInfo->gameState.outs = 0;
	localGameInfo->gameState.balls = 0;
	localGameInfo->gameState.strikes = 0;
	localGameInfo->playerCounters.nonJokerPlayersLeft = PLAYERS_IN_TEAM;
	localGameInfo->playerCounters.jokersLeft = 3;
	localGameInfo->playerCounters.battingTeamPlayersOnFieldCount = 0;
	localGameInfo->gameState.runsInTheInning = 0;
	localGameInfo->pII.batterSelectionIndex =
	    globalGameInfo->teams[battingTeamIndex].batterOrder[globalGameInfo->teams[battingTeamIndex].batterOrderIndex];
}
// index information initialization, can be called when out of bounds
void initializeIndexInformation(LocalGameInfo* localGameInfo)
{
	localGameInfo->pII.hasBallIndex = -1;
	localGameInfo->pII.lastHadBallIndex = -1;
	localGameInfo->pII.controlIndex = -1;
	localGameInfo->pII.batterIndex = -1;
	localGameInfo->pII.changePlayerArrayIndex = -1;
}
// player-related action information initialization, can be called when foul play.
void initializePRAIInformation(LocalGameInfo* localGameInfo)
{
	int i;
	localGameInfo->pRAI.pitchState = PITCH_STAGE_NONE;
	localGameInfo->pRAI.meterValue = 0.0f;
	localGameInfo->pRAI.swingMeterValue = 0.0f;
	localGameInfo->pRAI.battingGoingOn = 0;
	localGameInfo->pRAI.batterCanAdvance = 0;
	localGameInfo->pRAI.batHit = 0;
	localGameInfo->pRAI.batMiss = 0;
	localGameInfo->pRAI.throwGoingToBase = -1;
	localGameInfo->pRAI.batterReady = 0;
	localGameInfo->pRAI.refreshCatchAndChange = 0;
	localGameInfo->pRAI.initPlayerSelection = 0;
	localGameInfo->pRAI.initBatter = 0;
	for(i = 0; i < BASE_COUNT; i++) {
		localGameInfo->pRAI.willStartRunning[i] = 0;
	}
}

void setRunnerAndBatter(LocalGameInfo* localGameInfo, GlobalGameInfo* globalGameInfo, FieldPositions* fieldPositions)
{
	int battingTeamIndex = (globalGameInfo->
	                        inning+globalGameInfo->playsFirst+globalGameInfo->period)%2;
	Vector3D target;
	int i;
	if(localGameInfo->gameModeState.runnerBatterPairCounter < globalGameInfo->pairCount) {
		int runnerIndex = globalGameInfo->teams[battingTeamIndex].
		                  batterRunnerIndices[1][localGameInfo->gameModeState.runnerBatterPairCounter];
		int batterIndex = globalGameInfo->teams[battingTeamIndex].
		                  batterRunnerIndices[0][localGameInfo->gameModeState.runnerBatterPairCounter];
		localGameInfo->playerCounters.battingTeamPlayersOnFieldCount = 2;
		// batter
		if(batterIndex != -1) {
			localGameInfo->playerInfo[batterIndex].bTPI.baseId = BASE_HOME;
			localGameInfo->playerInfo[batterIndex].bTPI.state = PLAYER_STATE_AT_BAT;
			localGameInfo->referee.battingPlayers[batterIndex].baseAtPitchStart = BASE_HOME;
			localGameInfo->playerInfo[batterIndex].bTPI.number = localGameInfo->gameModeState.runnerBatterPairCounter + 1;
			// set batterIndex, this will make it so that the player is recognized as a batter when he arrives
			// the batting location
			localGameInfo->pII.batterIndex = batterIndex;
			// move player to default batter ready position
			target.x = (float)(fieldPositions->pitchPlate.x + cos(ZERO_BATTING_ANGLE)*BATTING_RADIUS);
			target.z = (float)(fieldPositions->pitchPlate.z - sin(ZERO_BATTING_ANGLE)*BATTING_RADIUS);
			moveToTarget(localGameInfo->playerInfo, batterIndex, &target);
		}
		// runner
		if(runnerIndex != -1) {
			localGameInfo->playerInfo[runnerIndex].bTPI.baseId = BASE_THIRD;
			localGameInfo->playerInfo[runnerIndex].bTPI.state = PLAYER_STATE_SAFE_ON_BASE;
			localGameInfo->referee.battingPlayers[runnerIndex].baseAtPitchStart = BASE_THIRD;
			localGameInfo->referee.battingPlayers[runnerIndex].hadSafetyAtPitchStart = 1; // Correctness
			localGameInfo->referee.battingPlayers[runnerIndex].currentSafetyBase = BASE_THIRD;

			localGameInfo->playerInfo[runnerIndex].tPI.location.x =
			    fieldPositions->thirdBaseRun.x;
			localGameInfo->playerInfo[runnerIndex].tPI.location.y =
			    fieldPositions->thirdBaseRun.y;
			localGameInfo->playerInfo[runnerIndex].tPI.location.z =
			    fieldPositions->thirdBaseRun.z;
			localGameInfo->playerInfo[runnerIndex].tPI.lastLocation.x =
			    localGameInfo->playerInfo[runnerIndex].tPI.location.x;
			localGameInfo->playerInfo[runnerIndex].tPI.lastLocation.y =
			    localGameInfo->playerInfo[runnerIndex].tPI.location.y;
			localGameInfo->playerInfo[runnerIndex].tPI.lastLocation.z =
			    localGameInfo->playerInfo[runnerIndex].tPI.location.z;
			localGameInfo->playerInfo[runnerIndex].tPI.orientation.x =
			    -localGameInfo->playerInfo[runnerIndex].tPI.location.x;
			localGameInfo->playerInfo[runnerIndex].tPI.orientation.y = 0.0f;
			localGameInfo->playerInfo[runnerIndex].tPI.orientation.z =
			    -localGameInfo->playerInfo[runnerIndex].tPI.location.x;
		}
		// set other runners next to the third base.
		for(i = localGameInfo->gameModeState.runnerBatterPairCounter + 1; i < globalGameInfo->pairCount; i++) {
			int index = globalGameInfo->teams[battingTeamIndex].batterRunnerIndices[1][i];
			if(index != -1) {
				localGameInfo->playerInfo[index].tPI.location.x = fieldPositions->thirdBaseRun.x -
				    2.0f - (i-(localGameInfo->gameModeState.runnerBatterPairCounter + 1))*1.5f;
				localGameInfo->playerInfo[index].tPI.location.y =
				    fieldPositions->thirdBaseRun.y;
				localGameInfo->playerInfo[index].tPI.location.z =
				    fieldPositions->thirdBaseRun.z;
				localGameInfo->playerInfo[index].tPI.lastLocation.x =
				    localGameInfo->playerInfo[index].tPI.location.x;
				localGameInfo->playerInfo[index].tPI.lastLocation.y =
				    localGameInfo->playerInfo[index].tPI.location.y;
				localGameInfo->playerInfo[index].tPI.lastLocation.z =
				    localGameInfo->playerInfo[index].tPI.location.z;
				localGameInfo->playerInfo[index].tPI.orientation.x =
				    -localGameInfo->playerInfo[index].tPI.location.x;
				localGameInfo->playerInfo[index].tPI.orientation.y = 0.0f;
				localGameInfo->playerInfo[index].tPI.orientation.z =
				    -localGameInfo->playerInfo[index].tPI.location.x;
			}
		}
	}
}

void initializeRefereeState(RefereeState* referee)
{
	int i;
	for (i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		referee->battingPlayers[i].baseAtPitchStart = BASE_NONE;
		referee->battingPlayers[i].hadSafetyAtPitchStart = 0;
		referee->battingPlayers[i].currentSafetyBase = BASE_NONE;
		referee->battingPlayers[i].hasPendingWound = 0;
		referee->battingPlayers[i].woundingType = WOUNDING_TYPE_NONE;
		referee->battingPlayers[i].woundingSourceBase = BASE_NONE;
		referee->battingPlayers[i].baseAtLastEvent = BASE_NONE;
		referee->battingPlayers[i].hadSafetyAtLastEvent = 0;
	}
	referee->woundingCatchActive = 0;
	referee->foulPlayActive = 0;
}

void loadMutableWorldSettings(StateInfo* stateInfo, unsigned int* rng_seed)
{
	/*
	* called always when half-inning starts.
	*
	*/
	// initialize ball flags
	initializeBallInfo(stateInfo->localGameInfo);
	// action flags
	initializeActionInfo(stateInfo->localGameInfo);
	// game analysis information that can be flushed when foul play happens
	initializeTemporaryGameAnalysisInfo(stateInfo->localGameInfo);
	// game information that should not be initialized before the inning ends
	initializeCriticalGameInfo(stateInfo->localGameInfo, stateInfo->globalGameInfo);
	// index information that can be flushed
	initializeIndexInformation(stateInfo->localGameInfo);
	// player-related action information that can be flushed
	initializePRAIInformation(stateInfo->localGameInfo);
	// this is information that stays for the whole inning
	initializeInningPermanentPlayerInformation(stateInfo->localGameInfo, stateInfo->globalGameInfo, stateInfo->teamData);
	// information about location and models and orientations. will be flushed when foul play happens
	initializeSpatialPlayerInformation(stateInfo->localGameInfo, stateInfo->fieldPositions, rng_seed);
	// information about players than can be flushed.
	initializeNonCriticalPlayerInformation(stateInfo->localGameInfo);
	// information that cant be flushed when foul play. like baseAtPitchStart.
	initializeCriticalBattingTeamInformation(stateInfo->localGameInfo);
	// initialize referee state
	initializeRefereeState(&stateInfo->localGameInfo->referee);

	if(stateInfo->globalGameInfo->period >= 4) {
		if(!(stateInfo->localGameInfo->gameModeState.runnerBatterPairCounter > 0 &&
		        stateInfo->localGameInfo->gameModeState.runnerBatterPairCounter <
		        stateInfo->globalGameInfo->pairCount)) {
			stateInfo->localGameInfo->gameModeState.runnerBatterPairCounter = 0;
		}
		setRunnerAndBatter(stateInfo->localGameInfo, stateInfo->globalGameInfo, stateInfo->fieldPositions);
	}
}

