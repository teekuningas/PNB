#ifndef COMMON_LOGIC_H
#define COMMON_LOGIC_H

// just common functions used in multiple .c

// here some vector math helper functions used everywhere

static __inline int isVectorSmallEnoughSphere(Vector3D *vector, float limit)
{
	if(sqrt((vector->x)*(vector->x) + (vector->y)*(vector->y) + (vector->z)*(vector->z)) < limit) return 1;
	else return 0;
}

static __inline int isVectorSmallEnoughCircleXZV(Vector3D *vector, float limit)
{
	if(sqrt((vector->x)*(vector->x) + (vector->z)*(vector->z)) < limit) return 1;
	else return 0;
}

static __inline int isVectorSmallEnoughCircleXZ(float dx, float dz, float limit)
{
	if(sqrt(dx*dx + dz*dz) < limit) return 1;
	else return 0;
}

static __inline void setVectorXYZ(Vector3D *vector, float x, float y, float z)
{
	vector->x = x;
	vector->y = y;
	vector->z = z;
}
static __inline void setVectorV(Vector3D *vector1, Vector3D *vector2)
{
	vector1->x = vector2->x;
	vector1->y = vector2->y;
	vector1->z = vector2->z;
}
static __inline void setVectorXZ(Vector3D *vector, float x, float z)
{
	vector->x = x;
	vector->z = z;
}

static __inline void addToVectorXZ(Vector3D *vector, float x, float z)
{
	vector->x += x;
	vector->z += z;
}
static __inline void addToVectorV(Vector3D *vector1, Vector3D *vector2)
{
	vector1->x += vector2->x;
	vector1->y += vector2->y;
	vector1->z += vector2->z;
}
/*
	Index is index of the player in the playerInfo-array.
	stopMovement stops arrow key initiated movement. Many situations
	where the change of controlled player will leave the previously controlled
	player moving so this is commonly used to stop these ones.
*/
static __inline void stopMovement(int index)
{
	int j;
	if(index != -1) {
		for(j = 0; j < DIRECTION_COUNT; j++) {
			stateInfo.localGameInfo->playerInfo[index].cTPI.movesToDirection[j] = 0;
		}
		// and after stopping movement, also ensure that no animation stays.
		if(stateInfo.localGameInfo->playerInfo[index].cTPI.throwRecoil == 0) {
			stateInfo.localGameInfo->playerInfo[index].cPI.model = 0;
		}
		stateInfo.localGameInfo->playerInfo[index].cPI.looksForTarget = 0;
		stateInfo.localGameInfo->playerInfo[index].cPI.moving = 0;

		stateInfo.localGameInfo->playerInfo[index].cPI.lastLastLocationUpdate = 1;
	}
}
// sometimes for example after a catch, we stop the the player, so that it wouldnt continue
// on its own. but it should still continue, as if player has the key presse down all the time.
// then we call this to start the movement again if there has been no release of the key inbetween
static __inline void smoothOutMovement()
{
	int j;
	for(j = 0; j < DIRECTION_COUNT; j++) {
		if(stateInfo.localGameInfo->aF.cTAF.move[j] == 2) {
			stateInfo.localGameInfo->aF.cTAF.move[j] = 1;
		}
	}
}
// this is for batting team players
static __inline void stopTargetLookingPlayer(int index)
{
	stateInfo.localGameInfo->playerInfo[index].cPI.moving = 0;
	stateInfo.localGameInfo->playerInfo[index].cPI.running = 0;
	stateInfo.localGameInfo->playerInfo[index].cPI.looksForTarget = 0;
	stateInfo.localGameInfo->playerInfo[index].cPI.lastLastLocationUpdate = 1;
}

static __inline void setOrientation(int i)
{
	// simply set player to orient towards the ball
	if(i != -1) {
		float dx = stateInfo.localGameInfo->ballInfo.location.x - stateInfo.localGameInfo->playerInfo[i].tPI.location.x;
		float dz = stateInfo.localGameInfo->ballInfo.location.z - stateInfo.localGameInfo->playerInfo[i].tPI.location.z;
		stateInfo.localGameInfo->playerInfo[i].tPI.orientation.x = dx;
		stateInfo.localGameInfo->playerInfo[i].tPI.orientation.z = dz;
	}
}

static __inline void runToTarget(int index, Vector3D *target)
{
	if(index != -1) {
		float dx;
		float dz;
		float speed;
		float norm;
		// so set target location
		stateInfo.localGameInfo->playerInfo[index].tPI.targetLocation.x =
		    target->x;
		stateInfo.localGameInfo->playerInfo[index].tPI.targetLocation.z =
		    target->z;
		// looking for target yeah
		stateInfo.localGameInfo->playerInfo[index].cPI.looksForTarget = 1;
		// find the direction
		dx = stateInfo.localGameInfo->playerInfo[index].tPI.targetLocation.x -
		     stateInfo.localGameInfo->playerInfo[index].tPI.location.x;
		dz = stateInfo.localGameInfo->playerInfo[index].tPI.targetLocation.z -
		     stateInfo.localGameInfo->playerInfo[index].tPI.location.z;
		norm = (float)sqrt(dx*dx + dz*dz);
		if(norm < EPSILON) norm = 1.0f;
		// set the velocity

		speed = BATTING_TEAM_RUN_FACTOR * RUN_SPEED + (RUN_SPEED/16)*stateInfo.localGameInfo->playerInfo[index].bTPI.speed;
		setVectorXZ(&stateInfo.localGameInfo->playerInfo[index].tPI.velocity, dx*speed/norm, dz*speed/norm);
		// we are running now, ( so for example our orientation wont change now unless we stop running)
		stateInfo.localGameInfo->playerInfo[index].cPI.running = 1;
		// we are moving too
		stateInfo.localGameInfo->playerInfo[index].cPI.moving = 1;
		// orientation to our direction
		stateInfo.localGameInfo->playerInfo[index].tPI.orientation.x = dx;
		stateInfo.localGameInfo->playerInfo[index].tPI.orientation.z = dz;
		// and set the running animation
		stateInfo.localGameInfo->playerInfo[index].cPI.model = 12;
		stateInfo.localGameInfo->playerInfo[index].cPI.animationStage = 0;
		stateInfo.localGameInfo->playerInfo[index].cPI.animationStageCount = 20;
		stateInfo.localGameInfo->playerInfo[index].cPI.animationFrequency = 3;
	}
}
/*
	this function puts player with index in the argument moving to some specified
	target by walking. is used for both fielders and batting team.
*/
static __inline void moveToTarget(int index, Vector3D *target)
{
	if(index != -1) {
		// cant start this if throw is going on. when ball is thrown the
		// control will often change automatically and we dont want the player to
		// start moving with walking animation before its throw animation has finished.
		if(stateInfo.localGameInfo->playerInfo[index].cTPI.throwRecoil == 0) {
			float dx;
			float dz;
			float norm;
			stateInfo.localGameInfo->playerInfo[index].tPI.targetLocation.x =
			    target->x;
			stateInfo.localGameInfo->playerInfo[index].tPI.targetLocation.z =
			    target->z;
			// looksForTarget is important flag to avoid unnecessary
			// overhead of checking whether the player has
			// arrived to target location.
			stateInfo.localGameInfo->playerInfo[index].cPI.looksForTarget = 1;
			// first find the unit vector for direction and then set player's
			// velocity to be the direction vector times the walk_speed.
			dx = stateInfo.localGameInfo->playerInfo[index].tPI.targetLocation.x -
			     stateInfo.localGameInfo->playerInfo[index].tPI.location.x;
			dz = stateInfo.localGameInfo->playerInfo[index].tPI.targetLocation.z -
			     stateInfo.localGameInfo->playerInfo[index].tPI.location.z;
			norm = (float)sqrt(dx*dx + dz*dz);
			if(norm < EPSILON) norm = 1.0f;
			setVectorXZ(&stateInfo.localGameInfo->playerInfo[index].tPI.velocity, dx*WALK_SPEED/norm, dz*WALK_SPEED/norm);
			// if the player for some reason was running before this, set that to 0.
			// could happen for example if baserunner gets out.
			stateInfo.localGameInfo->playerInfo[index].cPI.running = 0;
			// and set moving to 1 so that player's location will be updated.
			stateInfo.localGameInfo->playerInfo[index].cPI.moving = 1;
			// choose different walking animation for fielders and batting team.
			if(index < PLAYERS_IN_TEAM + JOKER_COUNT) {
				stateInfo.localGameInfo->playerInfo[index].cPI.model = 11;
			} else {
				stateInfo.localGameInfo->playerInfo[index].cPI.model = 2;
			}
			stateInfo.localGameInfo->playerInfo[index].cPI.animationStage = 0;
			stateInfo.localGameInfo->playerInfo[index].cPI.animationStageCount = 16;
			stateInfo.localGameInfo->playerInfo[index].cPI.animationFrequency = 3;

		}
	}
}
// so this function is called when outs happen but also when wounds happen. it just moves
// players out of the field and then to homebase.
static __inline void movePlayerOut(int index)
{
	Vector3D target;
	// we are walking
	stateInfo.localGameInfo->playerInfo[index].cPI.running = 0;
	// left or right?
	if(stateInfo.localGameInfo->playerInfo[index].tPI.location.x < 0) {
		target.x = stateInfo.fieldPositions->leftPoint.x - 5.0f;
		target.z = stateInfo.fieldPositions->leftPoint.z + 10.0f;
	} else {
		target.x = stateInfo.fieldPositions->rightPoint.x + 5.0f;
		target.z = stateInfo.fieldPositions->rightPoint.z + 10.0f;
	}
	// path point not passed yet.
	stateInfo.localGameInfo->playerInfo[index].bTPI.passedPathPoint = 0;
	// and move to target takes care of the rest.
	moveToTarget(index, &target);
}
// so we have the ranked fielders-array and those are players who are somewhat important in relation
// to ball's current location and velocity. so its natural that we have those players moving to catch
// the ball.
static __inline void moveRankedToCatch()
{
	int i;

	for(i = 0; i < RANKED_FIELDERS_COUNT; i++) {
		int index = stateInfo.localGameInfo->pII.fielderRankedIndices[i];
		// controlled player wont get the chance.
		if(index != stateInfo.localGameInfo->pII.controlIndex && stateInfo.localGameInfo->playerInfo[index].
		        cTPI.replacingStage == 0) {
			// if we are throwing ( towards a base ) we dont want the baseman there to start moving
			// as it would be nice that he is at the base when ball is caught if baserunner is going there.
			if(stateInfo.localGameInfo->pRAI.throwGoingToBase == -1 ||
			        (stateInfo.localGameInfo->pII.catcherOnBaseIndex[stateInfo.localGameInfo->pRAI.throwGoingToBase] != index)) {
				int k;
				int done = 0;
				// and we have special condition not to move any basemen
				// automatically at all.
				for(k = 0; k < BASE_COUNT; k++) {
					if(stateInfo.localGameInfo->pII.catcherOnBaseIndex[k] == index) {
						done = 1;
					}
				}
				if(done == 0) {
					// set busycatching flag, and move player towards the target point
					// that has been specified beforehand.
					stateInfo.localGameInfo->playerInfo[stateInfo.localGameInfo->pII.fielderRankedIndices[i]].cTPI.busyCatching = 1;
					moveToTarget(index, &stateInfo.localGameInfo->gAI.targetPoint);
				}
			}
		}
	}
}

static __inline void runToNextBase(int index, int base)
{
	if(index != -1) {
		Vector3D target;
		// first we select the target corresponding to base argument
		if(base == 0) {
			if(stateInfo.localGameInfo->pRAI.batterCanAdvance == 0) return;
			target.x = stateInfo.fieldPositions->firstBaseRun.x;
			target.z = stateInfo.fieldPositions->firstBaseRun.z;
			// here as it is the batter, we'll also stop any batting to be able to run freely.
			stateInfo.localGameInfo->pRAI.batterReady = 0;
			stateInfo.localGameInfo->pRAI.battingGoingOn = 0;
			stateInfo.localGameInfo->gAI.batterStartedRunning = 1;
		} else if(base == 1) {
			target.x = stateInfo.fieldPositions->secondBaseRun.x;
			target.z = stateInfo.fieldPositions->secondBaseRun.z;
		} else if(base == 2) {
			target.x = stateInfo.fieldPositions->thirdBaseRun.x;
			target.z = stateInfo.fieldPositions->thirdBaseRun.z;
		} else if(base == 3) {
			// if we are running home, there is the "flag" point, and we must change the direction there.
			// how it matters here is that if we have already passed the flag, we must run towards homebase,
			// if not, we must run towards flag.
			if(stateInfo.localGameInfo->playerInfo[index].bTPI.passedPathPoint == 0) {
				target.x = stateInfo.fieldPositions->runLeftPoint.x;
				target.z = stateInfo.fieldPositions->runLeftPoint.z;

				stateInfo.localGameInfo->gAI.homeRunCameraFlag = 1;
			} else if(stateInfo.localGameInfo->playerInfo[index].bTPI.passedPathPoint == 1) {
				target.x = stateInfo.fieldPositions->homeRunPoint.x;
				target.z = stateInfo.fieldPositions->homeRunPoint.z;
			} else {
				return;
			}


		} else {
			return;
		}
		// and set it so that next player has to have a will of his own to run
		stateInfo.localGameInfo->pRAI.willStartRunning[base] = 0;
		// if we are running we arent leading
		stateInfo.localGameInfo->playerInfo[index].bTPI.leading = 0;
		// and we are leaving the base, or at least we arent there yet.
		stateInfo.localGameInfo->playerInfo[index].bTPI.isOnBase = 0;
		// and we are moving forward
		stateInfo.localGameInfo->playerInfo[index].bTPI.goingForward = 1;
		// and runToTarget can continue the job with index and the already set target.
		runToTarget(index, &target);
	}
}

static __inline void runToPreviousBase(int index, int base)
{
	if(index != -1) {
		Vector3D target;
		// run to previous base works similarly to run to next base.
		// starting point here is that we arent on any base, and the base variable is telling us
		// the previous base
		if(base == 0) {
			// so when batter returns, he will go to his ready position again.
			target.x = (float)(stateInfo.fieldPositions->pitchPlate.x + cos(ZERO_BATTING_ANGLE)*BATTING_RADIUS);
			target.z = (float)(stateInfo.fieldPositions->pitchPlate.z - sin(ZERO_BATTING_ANGLE)*BATTING_RADIUS);
		} else if(base == 1) {
			target.x = stateInfo.fieldPositions->firstBaseRun.x;
			target.z = stateInfo.fieldPositions->firstBaseRun.z;
		} else if(base == 2) {
			target.x = stateInfo.fieldPositions->secondBaseRun.x;
			target.z = stateInfo.fieldPositions->secondBaseRun.z;
		} else if(base == 3) {
			// here we again select the target by our current location relative to flag
			if(stateInfo.localGameInfo->playerInfo[index].bTPI.passedPathPoint == 0) {
				target.x = stateInfo.fieldPositions->thirdBaseRun.x;
				target.z = stateInfo.fieldPositions->thirdBaseRun.z;
			} else if(stateInfo.localGameInfo->playerInfo[index].bTPI.passedPathPoint == 1) {
				target.x = stateInfo.fieldPositions->runLeftPoint.x;
				target.z = stateInfo.fieldPositions->runLeftPoint.z;
			} else {
				return;
			}

		} else {
			return;
		}

		// and set it so that next player has to have a will of his own to run
		stateInfo.localGameInfo->pRAI.willStartRunning[base] = 0;
		// we arent going forward
		stateInfo.localGameInfo->playerInfo[index].bTPI.goingForward = 0;
		// nor are we leading
		stateInfo.localGameInfo->playerInfo[index].bTPI.leading = 0;
		// and runToTarget can handle the rest

		runToTarget(index, &target);
	}
}

static __inline void lead(int index)
{
	if(index != -1) {
		int done = 0;
		Vector3D target;
		// now to lead we must be either on first base or second base, as it doesnt make much sense in
		// third base nor in homebase.
		if(stateInfo.localGameInfo->playerInfo[index].bTPI.base == 1) {
			// lead target is selected by adding a small step to current location to next bases' direction
			// using firstBase  instead of location in the difference is because we want the step size to stay
			// same
			target.x = stateInfo.localGameInfo->playerInfo[index].tPI.location.x + LEAD_STEP*(stateInfo.fieldPositions->secondBaseRun.x -
			           stateInfo.fieldPositions->firstBaseRun.x);
			target.z = stateInfo.localGameInfo->playerInfo[index].tPI.location.z + LEAD_STEP*(stateInfo.fieldPositions->secondBaseRun.z -
			           stateInfo.fieldPositions->firstBaseRun.z);
			// if we go over half way, we disallow any leading, you should just run from there.
			if(stateInfo.localGameInfo->playerInfo[index].tPI.location.x > stateInfo.fieldPositions->firstBaseRun.x +
			        0.5f*(stateInfo.fieldPositions->secondBaseRun.x - stateInfo.fieldPositions->firstBaseRun.x))
				done = 1;
		} else if(stateInfo.localGameInfo->playerInfo[index].bTPI.base == 2) {
			// same as in previous but from second to third base
			target.x = stateInfo.localGameInfo->playerInfo[index].tPI.location.x + LEAD_STEP*(stateInfo.fieldPositions->thirdBaseRun.x -
			           stateInfo.fieldPositions->secondBaseRun.x);
			target.z = stateInfo.localGameInfo->playerInfo[index].tPI.location.z + LEAD_STEP*(stateInfo.fieldPositions->thirdBaseRun.z -
			           stateInfo.fieldPositions->secondBaseRun.z);
			if(stateInfo.localGameInfo->playerInfo[index].tPI.location.x < stateInfo.fieldPositions->secondBaseRun.x +
			        0.5f*(stateInfo.fieldPositions->thirdBaseRun.x - stateInfo.fieldPositions->secondBaseRun.x))
				done = 1;
		} else {
			done = 1;
		}
		// if our
		if(done == 0) {
			// walk to our target
			moveToTarget(index, &target);
			// now we in fact are leading
			stateInfo.localGameInfo->playerInfo[index].bTPI.leading = 1;
			// but we dont set going forward flag.
			stateInfo.localGameInfo->playerInfo[index].bTPI.goingForward = 0;
			// we arent in any base
			stateInfo.localGameInfo->playerInfo[index].bTPI.isOnBase = 0;
		}

	}
}
// so a little function to check if ball's x and z coordinates indicate that ball is out of the
// playing field.
static __inline int checkIfBallIsOutOfBounds()
{
	int value = 1;
	// first, is ball behind the line at the back, or too much at right or too much at left
	// or in front of the homeline
	// if not, continue
	if(stateInfo.localGameInfo->ballInfo.location.z > stateInfo.fieldPositions->backLeftPoint.z &&
	        stateInfo.localGameInfo->ballInfo.location.x < stateInfo.fieldPositions->backRightPoint.x &&
	        stateInfo.localGameInfo->ballInfo.location.x > stateInfo.fieldPositions->backLeftPoint.x &&
	        stateInfo.localGameInfo->ballInfo.location.z < HOME_LINE_Z) {
		float z0 = 1.0f;
		float x = stateInfo.fieldPositions->rightPoint.x;
		float z = stateInfo.fieldPositions->rightPoint.z - z0;
		float slope1 = z/x;
		float slope2;
		x = -stateInfo.fieldPositions->leftPoint.x;
		z = -(stateInfo.fieldPositions->leftPoint.z - z0);
		slope2 = z/x;
		// then here we just have basic line equations to check if the ball is
		// out or inside the lines from pitchPlate to rightPoint and leftPoint.
		if(stateInfo.localGameInfo->ballInfo.location.z - slope1*stateInfo.localGameInfo->ballInfo.location.x - z0 < 0 &&
		        stateInfo.localGameInfo->ballInfo.location.z - slope2*stateInfo.localGameInfo->ballInfo.location.x - z0 < 0) {
			value = 0;
		}
	}
	if(value == 1) {
		stateInfo.localGameInfo->ballInfo.hasHitGroundOutOfBounds = 1;
	}
	return value;
}

static __inline void changePlayer()
{
	// this is called by user explicitly and sometimes after updating changePlayer lists.
	// so cant change pitch if pitch is going on
	if(stateInfo.localGameInfo->pRAI.pitchGoingOn == 0) {
		// again we must ensure that the movement of previous player stops because user
		// initiated movement cant stop without user explicitly stopping it and we dont want
		// that the player will start randomly floating after control changes to next player.
		if(stateInfo.localGameInfo->pII.controlIndex != -1) {
			stopMovement(stateInfo.localGameInfo->pII.controlIndex);
		}

		if(stateInfo.localGameInfo->pII.fielderRankedIndices[stateInfo.localGameInfo->pII.changePlayerArrayIndex] != -1) {
			// set control to new index from the array
			stateInfo.localGameInfo->pII.controlIndex = stateInfo.localGameInfo->pII.fielderRankedIndices[stateInfo.localGameInfo->pII.changePlayerArrayIndex];
			// and set him to run
			stateInfo.localGameInfo->playerInfo[stateInfo.localGameInfo->pII.controlIndex].cPI.running = 1;
			// set replacing stage to 0, as after player is changed again, we dont want these things to leave
			// hanging.
			if(stateInfo.localGameInfo->playerInfo[stateInfo.localGameInfo->pII.controlIndex].cTPI.replacingStage == 1) {
				stateInfo.localGameInfo->playerInfo[stateInfo.localGameInfo->pII.controlIndex].cTPI.replacingStage = 0;
			}
			// same for busyCatching.
			if(stateInfo.localGameInfo->playerInfo[stateInfo.localGameInfo->pII.controlIndex].cTPI.busyCatching == 1) {
				stateInfo.localGameInfo->playerInfo[stateInfo.localGameInfo->pII.controlIndex].cTPI.busyCatching = 0;
			}
			// move others to catch( so that the previous one for example doesnt just stop if its near the ball )
			moveRankedToCatch();
			// stop player who has the selection now
			stopMovement(stateInfo.localGameInfo->pII.controlIndex);
			// but start moving again if movement key being held at the same time. for smooth movement.
			smoothOutMovement();
		}
	}

}

static __inline void prepareBatter()
{
	if(stateInfo.localGameInfo->pII.batterIndex != -1) {
		// batter ready model
		stateInfo.localGameInfo->playerInfo[stateInfo.localGameInfo->pII.batterIndex].cPI.model = 13;
		// can pitch now
		stateInfo.localGameInfo->pRAI.batterReady = 1;
		// waiting for pitch to go in air before starting the batting movement
		stateInfo.localGameInfo->aF.bTAF.swing = 0;
		// batterIndex has been selected before calling this function
		stateInfo.localGameInfo->pII.safeOnBaseIndex[0] = stateInfo.localGameInfo->pII.batterIndex;
		// and initialize batter so that everything is ready to go.
		stateInfo.localGameInfo->pRAI.initBatter = 1;
	}
}
// so here we calculate index and base of the player who is the leadrunner so that
// we can move him if thats the decision.
static __inline void calculateFreeWalk()
{
	int i;
	int maxOriginalBase = -1;
	int maxBase = -1;
	int maxIndex = -1;
	// we go throush every (nonwounded) candidate and check who has the biggest base value
	// if there are many of those who have same base value, we will pick the one who has
	// the biggest original base value. if both are same for some reason
	// then the selection will be quite random but shouldn't happen often and shouldn't be a big deal
	// either.
	for(i = 0; i < BASE_COUNT; i++) {
		int index = stateInfo.localGameInfo->pII.battingTeamOnFieldIndices[i];
		if(index != -1 && stateInfo.localGameInfo->playerInfo[index].bTPI.wounded == 0) {
			if(stateInfo.localGameInfo->playerInfo[index].bTPI.base >= maxBase) {
				if(stateInfo.localGameInfo->playerInfo[index].bTPI.base == maxBase) {
					if(stateInfo.localGameInfo->playerInfo[index].bTPI.originalBase > maxOriginalBase) {
						maxBase = stateInfo.localGameInfo->playerInfo[index].bTPI.base;
						maxOriginalBase = stateInfo.localGameInfo->playerInfo[index].bTPI.originalBase;
						maxIndex = index;
					}
				} else {
					maxBase = stateInfo.localGameInfo->playerInfo[index].bTPI.base;
					maxOriginalBase = stateInfo.localGameInfo->playerInfo[index].bTPI.originalBase;
					maxIndex = index;
				}
			}

		}
	}
	stateInfo.localGameInfo->gAI.freeWalkIndex = maxIndex;
	if(maxIndex != -1) stateInfo.localGameInfo->gAI.freeWalkBase = stateInfo.localGameInfo->playerInfo[maxIndex].bTPI.base;
	else stateInfo.localGameInfo->gAI.freeWalkBase = -1;
}
/*
	Here we initialize all the locations and velocities so that players will be in their correct
	positions and orientations on the field and ball looks like its thrown to pitcher. Models
	are updated also. Before calling this the fielding team must have its position-attributes filled.
*/
static __inline void initializeSpatialPlayerInformation()
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
		int random = rand()%(PLAYERS_IN_TEAM + JOKER_COUNT);
		if(battingTeamPlacement[random] == -1) {
			battingTeamPlacement[random] = i;
			i++;
		}
	}
	// when out of bounds situation or inning ends, we swing the ball in the air and let the pitcher
	// catch it
	stateInfo.localGameInfo->ballInfo.velocity.x = BALL_INIT_SPEED_X;
	stateInfo.localGameInfo->ballInfo.velocity.y = BALL_INIT_SPEED_Y;
	stateInfo.localGameInfo->ballInfo.velocity.z = BALL_INIT_SPEED_Z;
	stateInfo.localGameInfo->ballInfo.location.x = BALL_INIT_LOCATION_X;
	stateInfo.localGameInfo->ballInfo.location.y = BALL_INIT_LOCATION_Y;
	stateInfo.localGameInfo->ballInfo.location.z = BALL_INIT_LOCATION_Z;

	stateInfo.localGameInfo->ballInfo.lastLocation.x = stateInfo.localGameInfo->ballInfo.location.x;
	stateInfo.localGameInfo->ballInfo.lastLocation.y = stateInfo.localGameInfo->ballInfo.location.y;
	stateInfo.localGameInfo->ballInfo.lastLocation.z = stateInfo.localGameInfo->ballInfo.location.z;
	// set locations and models for batting team players
	for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		float radiusFix = (float)(1.0f + fabs(5.5f - battingTeamPlacement[i])/20.0f);
		stateInfo.localGameInfo->playerInfo[i].cPI.model = 10;

		stateInfo.localGameInfo->playerInfo[i].tPI.homeLocation.x = (float)(stateInfo.fieldPositions->pitchPlate.x +
		        (HOME_RADIUS) * radiusFix * cos(PI - (battingTeamPlacement[i]+1)*PI/(PLAYERS_IN_TEAM + JOKER_COUNT + 1)));
		stateInfo.localGameInfo->playerInfo[i].tPI.homeLocation.y = BALL_HEIGHT_WITH_PLAYER;
		stateInfo.localGameInfo->playerInfo[i].tPI.homeLocation.z = (float)(stateInfo.fieldPositions->pitchPlate.z +
		        (HOME_RADIUS) * radiusFix * sin(PI - (battingTeamPlacement[i]+1)*PI/(PLAYERS_IN_TEAM + JOKER_COUNT + 1)));
		stateInfo.localGameInfo->playerInfo[i].tPI.location.x = stateInfo.localGameInfo->playerInfo[i].tPI.homeLocation.x;
		stateInfo.localGameInfo->playerInfo[i].tPI.location.y = stateInfo.localGameInfo->playerInfo[i].tPI.homeLocation.y;
		stateInfo.localGameInfo->playerInfo[i].tPI.location.z = stateInfo.localGameInfo->playerInfo[i].tPI.homeLocation.z;
		stateInfo.localGameInfo->playerInfo[i].tPI.orientation.x = stateInfo.localGameInfo->ballInfo.location.x - stateInfo.localGameInfo->playerInfo[i].tPI.location.x;
		stateInfo.localGameInfo->playerInfo[i].tPI.orientation.y = stateInfo.localGameInfo->ballInfo.location.y - stateInfo.localGameInfo->playerInfo[i].tPI.location.y;
		stateInfo.localGameInfo->playerInfo[i].tPI.orientation.z = stateInfo.localGameInfo->ballInfo.location.z - stateInfo.localGameInfo->playerInfo[i].tPI.location.z;
		stateInfo.localGameInfo->playerInfo[i].tPI.lastLocation.x = stateInfo.localGameInfo->playerInfo[i].tPI.location.x;
		stateInfo.localGameInfo->playerInfo[i].tPI.lastLocation.y = stateInfo.localGameInfo->playerInfo[i].tPI.location.y;
		stateInfo.localGameInfo->playerInfo[i].tPI.lastLocation.z = stateInfo.localGameInfo->playerInfo[i].tPI.location.z;
	}
	// set locations and models for fielders. here we need positions set.
	for(i = 12; i < 2*PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		stateInfo.localGameInfo->playerInfo[i].cPI.model = 0;

		switch(i-12) {
		case 0:
			fieldPosition = &(stateInfo.fieldPositions->pitcher);
			break;
		case 1:
			fieldPosition = &(stateInfo.fieldPositions->firstBase);
			break;
		case 2:
			fieldPosition = &(stateInfo.fieldPositions->secondBase);
			break;
		case 3:
			fieldPosition = &(stateInfo.fieldPositions->thirdBase);
			break;
		case 4:
			fieldPosition = &(stateInfo.fieldPositions->bottomRightCatcher);
			break;
		case 5:
			fieldPosition = &(stateInfo.fieldPositions->middleLeftCatcher);
			break;
		case 6:
			fieldPosition = &(stateInfo.fieldPositions->middleRightCatcher);
			break;
		case 7:
			fieldPosition = &(stateInfo.fieldPositions->backLeftCatcher);
			break;
		case 8:
			fieldPosition = &(stateInfo.fieldPositions->backRightCatcher);
			break;
		default:
			fieldPosition = &(stateInfo.fieldPositions->pitchPlate);
			break;

		}
		stateInfo.localGameInfo->playerInfo[i].tPI.homeLocation.x = fieldPosition->x;
		stateInfo.localGameInfo->playerInfo[i].tPI.homeLocation.y = fieldPosition->y;
		stateInfo.localGameInfo->playerInfo[i].tPI.homeLocation.z = fieldPosition->z;

		stateInfo.localGameInfo->playerInfo[i].tPI.location.x = stateInfo.localGameInfo->playerInfo[i].tPI.homeLocation.x;
		stateInfo.localGameInfo->playerInfo[i].tPI.location.y = stateInfo.localGameInfo->playerInfo[i].tPI.homeLocation.y;
		stateInfo.localGameInfo->playerInfo[i].tPI.location.z = stateInfo.localGameInfo->playerInfo[i].tPI.homeLocation.z;

		stateInfo.localGameInfo->playerInfo[i].tPI.orientation.x = stateInfo.localGameInfo->ballInfo.location.x - stateInfo.localGameInfo->playerInfo[i].tPI.location.x;
		stateInfo.localGameInfo->playerInfo[i].tPI.orientation.y = stateInfo.localGameInfo->ballInfo.location.y - stateInfo.localGameInfo->playerInfo[i].tPI.location.y;
		stateInfo.localGameInfo->playerInfo[i].tPI.orientation.z = stateInfo.localGameInfo->ballInfo.location.z - stateInfo.localGameInfo->playerInfo[i].tPI.location.z;

		stateInfo.localGameInfo->playerInfo[i].tPI.lastLocation.x = stateInfo.localGameInfo->playerInfo[i].tPI.location.x;
		stateInfo.localGameInfo->playerInfo[i].tPI.lastLocation.y = stateInfo.localGameInfo->playerInfo[i].tPI.location.y;
		stateInfo.localGameInfo->playerInfo[i].tPI.lastLocation.z = stateInfo.localGameInfo->playerInfo[i].tPI.location.z;
	}
	// these are set for every player.
	for(i = 0; i < 2*PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		stateInfo.localGameInfo->playerInfo[i].tPI.velocity.x = 0.0f;
		stateInfo.localGameInfo->playerInfo[i].tPI.velocity.y = 0.0f;
		stateInfo.localGameInfo->playerInfo[i].tPI.velocity.z = 0.0f;
		stateInfo.localGameInfo->playerInfo[i].tPI.targetLocation.x = 0.0f;
		stateInfo.localGameInfo->playerInfo[i].tPI.targetLocation.y = 0.0f;
		stateInfo.localGameInfo->playerInfo[i].tPI.targetLocation.z = 0.0f;
	}
}
// here we initialize players' stat information, and team information, whether they are joker or not,
// their number etc. all is information that is not going to be reinitialized when out of bounds -
// situation happens
static __inline void initializeInningPermanentPlayerInformation()
{
	int battingTeamIndex = (stateInfo.globalGameInfo->
	                        inning+stateInfo.globalGameInfo->playsFirst+stateInfo.globalGameInfo->period)%2;
	int catchingTeamIndex = (battingTeamIndex+1)%2;
	int i;
	int jokerCounter = 0;

	// initialize batting team numbers and jokerness
	for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		if(i < PLAYERS_IN_TEAM) {
			stateInfo.localGameInfo->playerInfo[stateInfo.globalGameInfo->teams[battingTeamIndex].
			                                    batterOrder[i]].bTPI.number = i + 1;
			stateInfo.localGameInfo->playerInfo[stateInfo.globalGameInfo->teams[battingTeamIndex].
			                                    batterOrder[i]].bTPI.joker = 0;
		} else {
			stateInfo.localGameInfo->playerInfo[stateInfo.globalGameInfo->teams[battingTeamIndex].
			                                    batterOrder[i]].bTPI.number = 0;
			stateInfo.localGameInfo->playerInfo[stateInfo.globalGameInfo->teams[battingTeamIndex].
			                                    batterOrder[i]].bTPI.joker = 1;
		}
	}
	// initialize batting team
	for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		stateInfo.localGameInfo->playerInfo[i].cPI.team = battingTeamIndex;

		if(stateInfo.localGameInfo->playerInfo[i].bTPI.joker == 1) {
			stateInfo.localGameInfo->pII.jokerIndices[jokerCounter] = i;
			jokerCounter++;
		}
		stateInfo.localGameInfo->playerInfo[i].bTPI.name = stateInfo.teamData[(stateInfo.globalGameInfo->teams[battingTeamIndex]
		        .value - 1)].players[i].name;
		stateInfo.localGameInfo->playerInfo[i].bTPI.power = stateInfo.teamData[(stateInfo.globalGameInfo->teams[battingTeamIndex]
		        .value - 1)].players[i].power;
		stateInfo.localGameInfo->playerInfo[i].bTPI.speed = stateInfo.teamData[(stateInfo.globalGameInfo->teams[battingTeamIndex]
		        .value - 1)].players[i].speed;


	}
	// initialize fielders
	for(i = 12; i < 2*PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		stateInfo.localGameInfo->playerInfo[i].cPI.team = catchingTeamIndex;

		// here we set catcherOnBaseIndices and catcherReplacerOnBaseIndices.
		switch(i-12) {
		case 0:
			stateInfo.localGameInfo->pII.catcherOnBaseIndex[0] = i;
			break;
		case 1:
			stateInfo.localGameInfo->pII.catcherOnBaseIndex[1] = i;
			break;
		case 2:
			stateInfo.localGameInfo->pII.catcherOnBaseIndex[2] = i;
			break;
		case 3:
			stateInfo.localGameInfo->pII.catcherOnBaseIndex[3] = i;
			break;
		case 4:
			stateInfo.localGameInfo->pII.catcherReplacerOnBaseIndex[0] = i;
			stateInfo.localGameInfo->pII.catcherReplacerOnBaseIndex[1] = i;
			break;
		case 5:
			stateInfo.localGameInfo->pII.catcherReplacerOnBaseIndex[3] = i;
			break;
		case 6:
			stateInfo.localGameInfo->pII.catcherReplacerOnBaseIndex[2] = i;
			break;

		}
	}
}
// information that can be flushed
static __inline void initializeNonCriticalPlayerInformation()
{
	int i, j;
	for( i = 0; i < 2*PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		stateInfo.localGameInfo->playerInfo[i].cPI.animationFrequency = 1;
		stateInfo.localGameInfo->playerInfo[i].cPI.animationStage = 0;
		stateInfo.localGameInfo->playerInfo[i].cPI.animationStageCount = 0;
		stateInfo.localGameInfo->playerInfo[i].cPI.moving = 0;
		stateInfo.localGameInfo->playerInfo[i].cPI.running = 0;
		stateInfo.localGameInfo->playerInfo[i].cPI.looksForTarget = 0;
		stateInfo.localGameInfo->playerInfo[i].cPI.lastLastLocationUpdate = 1;
		if( i >= PLAYERS_IN_TEAM + JOKER_COUNT) {

			stateInfo.localGameInfo->playerInfo[i].cTPI.isNearHomeLocation = 1;
			stateInfo.localGameInfo->playerInfo[i].cTPI.replacingStage = 0;
			stateInfo.localGameInfo->playerInfo[i].cTPI.replacingBase = -1;
			stateInfo.localGameInfo->playerInfo[i].cTPI.busyCatching = 0;
			stateInfo.localGameInfo->playerInfo[i].cTPI.throwRecoil = 0;
			// initialize fielderRankedIndices with the indices of five first
			// players in positional order.
			if(i-12 < RANKED_FIELDERS_COUNT) {
				stateInfo.localGameInfo->pII.fielderRankedIndices
				[i-12] = i;
			}

			for(j = 0; j < DIRECTION_COUNT; j++) {
				stateInfo.localGameInfo->playerInfo[i].cTPI.movesToDirection[j] = 0;
			}
		} else {
			stateInfo.localGameInfo->playerInfo[i].bTPI.arrivedToBase = 0;
			stateInfo.localGameInfo->playerInfo[i].bTPI.isOnBase = 0;
			stateInfo.localGameInfo->playerInfo[i].bTPI.leading = 0;
			stateInfo.localGameInfo->playerInfo[i].bTPI.passedPathPoint = 0;
			stateInfo.localGameInfo->playerInfo[i].bTPI.goingForward = 0;
			stateInfo.localGameInfo->playerInfo[i].bTPI.wounded = 0;
			stateInfo.localGameInfo->playerInfo[i].bTPI.woundedApply = 0;
			stateInfo.localGameInfo->playerInfo[i].bTPI.takingFreeWalk = 0;
			stateInfo.localGameInfo->playerInfo[i].bTPI.out = 0;
			stateInfo.localGameInfo->playerInfo[i].bTPI.hasMadeRunOnThirdBase = 0;
			stateInfo.localGameInfo->playerInfo[i].bTPI.base = -1;
		}
	}

}
// this information is important for correct continuity after foul play.
static __inline void initializeCriticalBattingTeamInformation()
{
	int i;
	for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		stateInfo.localGameInfo->playerInfo[i].bTPI.originalBase = -1;
	}
}
// ball flags
static __inline void initializeBallInfo()
{
	stateInfo.localGameInfo->ballInfo.visible = 1;
	stateInfo.localGameInfo->ballInfo.moving = 1;
	stateInfo.localGameInfo->ballInfo.hasHitGround = 0;
	stateInfo.localGameInfo->ballInfo.onGround = 0;
	stateInfo.localGameInfo->ballInfo.hitsGroundToUnWound = 0;
	stateInfo.localGameInfo->ballInfo.hasHitGroundOutOfBounds = 0;
	stateInfo.localGameInfo->ballInfo.needsMoveUpdate = 0;
	stateInfo.localGameInfo->ballInfo.lastLastLocationUpdate = 0;
}
// action flag initialization
static __inline void initializeActionInfo()
{
	int i;

	for(i = 0; i < BASE_COUNT; i++) {
		stateInfo.localGameInfo->aF.bTAF.baseRun[i] = 0;
	}
	stateInfo.localGameInfo->aF.bTAF.chooseBatter = 0;
	stateInfo.localGameInfo->aF.bTAF.takeFreeWalk = 0;
	stateInfo.localGameInfo->aF.bTAF.swing = 0;
	stateInfo.localGameInfo->aF.bTAF.increaseBatterAngle = 0;
	stateInfo.localGameInfo->aF.bTAF.decreaseBatterAngle = 0;

	for(i = 0; i < BASE_COUNT; i++) {
		stateInfo.localGameInfo->aF.cTAF.move[i] = 0;
		stateInfo.localGameInfo->aF.cTAF.throwToBase[i] = 0;
	}
	stateInfo.localGameInfo->aF.cTAF.changePlayer = 0;
	stateInfo.localGameInfo->aF.cTAF.dropBall = 0;
	stateInfo.localGameInfo->aF.cTAF.pitch = 0;
	stateInfo.localGameInfo->aF.cTAF.actionKeyLock = 0;
}
// these can be flushed
static __inline void initializeTemporaryGameAnalysisInfo()
{
	stateInfo.localGameInfo->gAI.freeWalkCalculationMade = 1;
	stateInfo.localGameInfo->gAI.waitingForBatterDecision = 0;
	stateInfo.localGameInfo->gAI.waitingForFreeWalkDecision = 0;
	stateInfo.localGameInfo->gAI.outOfBounds = 0;
	stateInfo.localGameInfo->gAI.noMorePlayers = 0;
	stateInfo.localGameInfo->gAI.ballHome = 0;
	stateInfo.localGameInfo->gAI.endPeriod = 0;
	stateInfo.localGameInfo->gAI.woundingCatch = 0;
	stateInfo.localGameInfo->gAI.woundingCatchHandled = 0;
	stateInfo.localGameInfo->gAI.batterStartedRunning = 0;

	stateInfo.localGameInfo->gAI.gameInfoEvent = 0;
	stateInfo.localGameInfo->gAI.checkForRun = 0;
	stateInfo.localGameInfo->gAI.freeWalkIndex = -1;
	stateInfo.localGameInfo->gAI.freeWalkBase = -1;
	stateInfo.localGameInfo->gAI.playerArrivedToBase = 0;
	stateInfo.localGameInfo->gAI.firstCatchMade = 0;
	stateInfo.localGameInfo->gAI.pause = 0;
	stateInfo.localGameInfo->gAI.initLocals = 1;
	stateInfo.localGameInfo->gAI.forceNextPair = 0;
	stateInfo.localGameInfo->gAI.homeRunCameraFlag = 0;
	stateInfo.localGameInfo->gAI.canMakeRunOfHonor = 0;
	stateInfo.localGameInfo->gAI.targetPoint.x = 0.0f;
	stateInfo.localGameInfo->gAI.targetPoint.y = 0.0f;
	stateInfo.localGameInfo->gAI.targetPoint.z = 0.0f;
}
// these should be kept when foul play
static __inline void initializeCriticalGameInfo()
{
	int i;
	int battingTeamIndex = (stateInfo.globalGameInfo->
	                        inning+stateInfo.globalGameInfo->playsFirst+stateInfo.globalGameInfo->period)%2;
	stateInfo.localGameInfo->gAI.outs = 0;
	stateInfo.localGameInfo->gAI.balls = 0;
	stateInfo.localGameInfo->gAI.strikes = 0;
	stateInfo.localGameInfo->gAI.nonJokerPlayersLeft = PLAYERS_IN_TEAM;
	stateInfo.localGameInfo->gAI.jokersLeft = 3;
	stateInfo.localGameInfo->gAI.battingTeamPlayersOnFieldCount = 0;
	stateInfo.localGameInfo->gAI.runsInTheInning = 0;
	stateInfo.localGameInfo->pII.batterSelectionIndex =
	    stateInfo.globalGameInfo->teams[battingTeamIndex].batterOrder[stateInfo.globalGameInfo->teams[battingTeamIndex].batterOrderIndex];
	for(i = 0; i < BASE_COUNT; i++) {
		stateInfo.localGameInfo->pII.battingTeamOnFieldIndices[i] = -1;
	}

}
// index information initialization, can be called when out of bounds
static __inline void initializeIndexInformation()
{
	int i;
	for(i = 0; i < BASE_COUNT; i++) {
		stateInfo.localGameInfo->pII.safeOnBaseIndex[i] = -1;
	}

	stateInfo.localGameInfo->pII.hasBallIndex = -1;
	stateInfo.localGameInfo->pII.lastHadBallIndex = -1;
	stateInfo.localGameInfo->pII.controlIndex = -1;
	stateInfo.localGameInfo->pII.batterIndex = -1;
	stateInfo.localGameInfo->pII.changePlayerArrayIndex = -1;
}
// player related action information initialization, can be called when foul play.
static __inline void initializePRAIInformation()
{
	int i;
	stateInfo.localGameInfo->pRAI.pitchGoingOn = 0;
	stateInfo.localGameInfo->pRAI.pitchInAir = 0;
	stateInfo.localGameInfo->pRAI.meterValue = 0.0f;
	stateInfo.localGameInfo->pRAI.swingMeterValue = 0.0f;
	stateInfo.localGameInfo->pRAI.battingGoingOn = 0;
	stateInfo.localGameInfo->pRAI.batterCanAdvance = 0;
	stateInfo.localGameInfo->pRAI.batHit = 0;
	stateInfo.localGameInfo->pRAI.batMiss = 0;
	stateInfo.localGameInfo->pRAI.throwGoingToBase = -1;
	stateInfo.localGameInfo->pRAI.batterReady = 0;
	stateInfo.localGameInfo->pRAI.refreshCatchAndChange = 0;
	stateInfo.localGameInfo->pRAI.initPlayerSelection = 0;
	stateInfo.localGameInfo->pRAI.initBatter = 0;
	for(i = 0; i < BASE_COUNT; i++) {
		stateInfo.localGameInfo->pRAI.willStartRunning[i] = 0;
	}
}

static __inline void setRunnerAndBatter()
{
	int battingTeamIndex = (stateInfo.globalGameInfo->
	                        inning+stateInfo.globalGameInfo->playsFirst+stateInfo.globalGameInfo->period)%2;
	Vector3D target;
	int i;
	if(stateInfo.localGameInfo->gAI.runnerBatterPairCounter < stateInfo.globalGameInfo->pairCount) {
		int runnerIndex = stateInfo.globalGameInfo->teams[battingTeamIndex].
		                  batterRunnerIndices[1][stateInfo.localGameInfo->gAI.runnerBatterPairCounter];
		int batterIndex = stateInfo.globalGameInfo->teams[battingTeamIndex].
		                  batterRunnerIndices[0][stateInfo.localGameInfo->gAI.runnerBatterPairCounter];
		stateInfo.localGameInfo->pII.battingTeamOnFieldIndices[0] = runnerIndex;
		stateInfo.localGameInfo->pII.battingTeamOnFieldIndices[1] = batterIndex;
		stateInfo.localGameInfo->gAI.battingTeamPlayersOnFieldCount = 2;
		// batter
		if(batterIndex != -1) {
			stateInfo.localGameInfo->playerInfo[batterIndex].bTPI.base = 0;
			stateInfo.localGameInfo->playerInfo[batterIndex].bTPI.isOnBase = 1;
			stateInfo.localGameInfo->playerInfo[batterIndex].bTPI.originalBase = 0;
			stateInfo.localGameInfo->playerInfo[batterIndex].bTPI.number = stateInfo.localGameInfo->gAI.runnerBatterPairCounter + 1;
			// set batterIndex, this will make it so that the player is recognized as a batter when he arrives
			// the batting location
			stateInfo.localGameInfo->pII.batterIndex = batterIndex;
			// move player to default batter ready position
			target.x = (float)(stateInfo.fieldPositions->pitchPlate.x + cos(ZERO_BATTING_ANGLE)*BATTING_RADIUS);
			target.z = (float)(stateInfo.fieldPositions->pitchPlate.z - sin(ZERO_BATTING_ANGLE)*BATTING_RADIUS);
			moveToTarget(batterIndex, &target);
		}
		// runner
		if(runnerIndex != -1) {
			stateInfo.localGameInfo->playerInfo[runnerIndex].bTPI.base = 3;
			stateInfo.localGameInfo->playerInfo[runnerIndex].bTPI.isOnBase = 1;
			stateInfo.localGameInfo->playerInfo[runnerIndex].bTPI.originalBase = 3;
			stateInfo.localGameInfo->pII.safeOnBaseIndex[3] = runnerIndex;

			stateInfo.localGameInfo->playerInfo[runnerIndex].tPI.location.x =
			    stateInfo.fieldPositions->thirdBaseRun.x;
			stateInfo.localGameInfo->playerInfo[runnerIndex].tPI.location.y =
			    stateInfo.fieldPositions->thirdBaseRun.y;
			stateInfo.localGameInfo->playerInfo[runnerIndex].tPI.location.z =
			    stateInfo.fieldPositions->thirdBaseRun.z;
			stateInfo.localGameInfo->playerInfo[runnerIndex].tPI.lastLocation.x =
			    stateInfo.localGameInfo->playerInfo[runnerIndex].tPI.location.x;
			stateInfo.localGameInfo->playerInfo[runnerIndex].tPI.lastLocation.y =
			    stateInfo.localGameInfo->playerInfo[runnerIndex].tPI.location.y;
			stateInfo.localGameInfo->playerInfo[runnerIndex].tPI.lastLocation.z =
			    stateInfo.localGameInfo->playerInfo[runnerIndex].tPI.location.z;
			stateInfo.localGameInfo->playerInfo[runnerIndex].tPI.orientation.x =
			    -stateInfo.localGameInfo->playerInfo[runnerIndex].tPI.location.x;
			stateInfo.localGameInfo->playerInfo[runnerIndex].tPI.orientation.y = 0.0f;
			stateInfo.localGameInfo->playerInfo[runnerIndex].tPI.orientation.z =
			    -stateInfo.localGameInfo->playerInfo[runnerIndex].tPI.location.x;
		}
		// set other runners next to the third base.
		for(i = stateInfo.localGameInfo->gAI.runnerBatterPairCounter + 1; i < stateInfo.globalGameInfo->pairCount; i++) {
			int index = stateInfo.globalGameInfo->teams[battingTeamIndex].batterRunnerIndices[1][i];
			if(index != -1) {
				stateInfo.localGameInfo->playerInfo[index].tPI.location.x = stateInfo.fieldPositions->thirdBaseRun.x -
				        2.0f - (i-(stateInfo.localGameInfo->gAI.runnerBatterPairCounter + 1))*1.5f;
				stateInfo.localGameInfo->playerInfo[index].tPI.location.y =
				    stateInfo.fieldPositions->thirdBaseRun.y;
				stateInfo.localGameInfo->playerInfo[index].tPI.location.z =
				    stateInfo.fieldPositions->thirdBaseRun.z;
				stateInfo.localGameInfo->playerInfo[index].tPI.lastLocation.x =
				    stateInfo.localGameInfo->playerInfo[index].tPI.location.x;
				stateInfo.localGameInfo->playerInfo[index].tPI.lastLocation.y =
				    stateInfo.localGameInfo->playerInfo[index].tPI.location.y;
				stateInfo.localGameInfo->playerInfo[index].tPI.lastLocation.z =
				    stateInfo.localGameInfo->playerInfo[index].tPI.location.z;
				stateInfo.localGameInfo->playerInfo[index].tPI.orientation.x =
				    -stateInfo.localGameInfo->playerInfo[index].tPI.location.x;
				stateInfo.localGameInfo->playerInfo[index].tPI.orientation.y = 0.0f;
				stateInfo.localGameInfo->playerInfo[index].tPI.orientation.z =
				    -stateInfo.localGameInfo->playerInfo[index].tPI.location.x;
			}
		}
	}
}

static __inline void loadMutableWorldSettings()
{
	/*
	* called always when half-inning starts.
	*
	*/
	// initialize ball flags
	initializeBallInfo();
	// action flags
	initializeActionInfo();
	// game analysis information that can be flushed when foul play happens
	initializeTemporaryGameAnalysisInfo();
	// game information that should not be initialized before the inning ends
	initializeCriticalGameInfo();
	// index information that can be flushed
	initializeIndexInformation();
	// player-related action information that can be flushed
	initializePRAIInformation();
	// this is information that stays for the whole inning
	initializeInningPermanentPlayerInformation();
	// information about location and models and orientations. will be flushed when foul play happens
	initializeSpatialPlayerInformation();
	// information about players than can be flushed.
	initializeNonCriticalPlayerInformation();
	// information that cant be flushed when foul play. like originalBase.
	initializeCriticalBattingTeamInformation();

	if(stateInfo.globalGameInfo->period >= 4) {
		if(!(stateInfo.localGameInfo->gAI.runnerBatterPairCounter > 0 &&
		        stateInfo.localGameInfo->gAI.runnerBatterPairCounter <
		        stateInfo.globalGameInfo->pairCount)) {
			stateInfo.localGameInfo->gAI.runnerBatterPairCounter = 0;
		}
		setRunnerAndBatter();
	}

}


#endif /* COMMON_LOGIC_H */