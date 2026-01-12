#include <string.h>
#include "globals.h"
#include "common_logic.h"
#include "vector_math.h"
#include "geometry.h"
#include "rng.h"
#include "base_logic.h"
#include "game_analysis.h"
#include "game_manipulation.h"
#include "base_control.h"
#include "referee.h"
#include "rules_pure/player_utils.h"

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
void smoothOutMovement(MatchSession* match)
{
	int j;
	for(j = 0; j < DIRECTION_COUNT; j++) {
		if(match->aF.cTAF.move[j] == 2) {
			match->aF.cTAF.move[j] = 1;
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
void moveRankedToCatch(MatchSession* match)
{
	int i;

	for(i = 0; i < RANKED_FIELDERS_COUNT; i++) {
		int index = match->pII.fielderRankedIndices[i];
		// controlled player wont get the chance.
		if(index != match->pII.controlIndex &&match->playerInfo[index].
		        cTPI.replacingStage == REPLACEMENT_IDLE) {
			// if we are throwing ( towards a base ) we dont want the baseman there to start moving
			// as it would be nice that he is at the base when ball is caught if baserunner is going there.
			if(match->pRAI.throwGoingToBase == -1 ||
			        (match->pII.catcherOnBaseIndex[match->pRAI.throwGoingToBase] != index)) {
				int k;
				int done = 0;
				// and we have special condition not to move any basemen
				// automatically at all.
				for(k = 0; k < BASE_COUNT; k++) {
					if(match->pII.catcherOnBaseIndex[k] == index) {
						done = 1;
					}
				}
				if(done == 0) {
					// set busycatching flag, and move player towards the target point
					// that has been specified beforehand.
					match->playerInfo[match->pII.fielderRankedIndices[i]].cTPI.busyCatching = 1;
					moveToTarget(match->playerInfo, index, &match->cameraState.targetPoint);
				}
			}
		}
	}
}

void runToNextBase(MatchSession* match, FieldPositions* fieldPositions, int index, BaseID base)
{
	if(index != -1) {
		Vector3D target;
		// first we select the target corresponding to base argument
		if(base == BASE_HOME) {
			if(match->pRAI.batterCanAdvance == 0) return;
			target.x = fieldPositions->firstBaseRun.x;
			target.z = fieldPositions->firstBaseRun.z;
			// here as it is the batter, we'll also stop any batting to be able to run freely.
			match->pRAI.batterReady = 0;
			match->pRAI.battingGoingOn = 0;
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
			if(match->playerRuntime[index].passedPathPoint == 0) {
				target.x = fieldPositions->runLeftPoint.x;
				target.z = fieldPositions->runLeftPoint.z;

				match->cameraState.homeRunCameraFlag = 1;
			} else if(match->playerRuntime[index].passedPathPoint == 1) {
				target.x = fieldPositions->homeRunPoint.x;
				target.z = fieldPositions->homeRunPoint.z;
			} else {
				return;
			}


		} else {
			return;
		}
		// and set it so that next player has to have a will of his own to run
		match->pRAI.willStartRunning[base] = 0;
		// set state to running, BUT only if we aren't already WOUNDED or OUT (which are terminal/override states)
		if (match->playerInfo[index].bTPI.state != PLAYER_STATE_WOUNDED &&
		        match->playerInfo[index].bTPI.state != PLAYER_STATE_OUT) {
			match->playerInfo[index].bTPI.state = PLAYER_STATE_RUNNING;
		}
		// and we are moving forward
		match->playerRuntime[index].goingForward = 1;
		// and runToTarget can continue the job with index and the already set target.
		runToTarget(match->playerInfo, index, &target);
	}
}

void runToPreviousBase(MatchSession* match, FieldPositions* fieldPositions, int index, BaseID base)
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
			if(match->playerRuntime[index].passedPathPoint == 0) {
				target.x = fieldPositions->thirdBaseRun.x;
				target.z = fieldPositions->thirdBaseRun.z;
			} else if(match->playerRuntime[index].passedPathPoint == 1) {
				target.x = fieldPositions->runLeftPoint.x;
				target.z = fieldPositions->runLeftPoint.z;
			} else {
				return;
			}

		} else {
			return;
		}

		// and set it so that next player has to have a will of his own to run
		match->pRAI.willStartRunning[base] = 0;
		// we arent going forward
		match->playerRuntime[index].goingForward = 0;
		// set state to running, BUT only if we aren't already WOUNDED or OUT
		if (match->playerInfo[index].bTPI.state != PLAYER_STATE_WOUNDED &&
		        match->playerInfo[index].bTPI.state != PLAYER_STATE_OUT) {
			match->playerInfo[index].bTPI.state = PLAYER_STATE_RUNNING;
		}
		// and runToTarget can handle the rest

		runToTarget(match->playerInfo, index, &target);
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

void changePlayer(MatchSession* match)
{
	// this is called by user explicitly and sometimes after updating changePlayer lists.
	// so cant change pitch if pitch is going on
	if(match->pRAI.pitchState == PITCH_STAGE_NONE) {
		// player will start randomly floating after control changes to next player.
		if(match->pII.controlIndex != -1) {
			stopMovement(match->playerInfo, match->pII.controlIndex);
		}

		if(match->pII.fielderRankedIndices[match->pII.changePlayerArrayIndex] != -1) { // set control to new index from the array
			match->pII.controlIndex = match->pII.fielderRankedIndices[match->pII.changePlayerArrayIndex];
			// and set him to run
			match->playerInfo[match->pII.controlIndex].cPI.running = 1;
			// logic about coming back from replacement if controlled
			if(match->playerInfo[match->pII.controlIndex].cTPI.replacingStage == REPLACEMENT_ACTIVE) {
				match->playerInfo[match->pII.controlIndex].cTPI.replacingStage = REPLACEMENT_IDLE;
			}
			// same for busyCatching.
			if(match->playerInfo[match->pII.controlIndex].cTPI.busyCatching == 1) {
				match->playerInfo[match->pII.controlIndex].cTPI.busyCatching = 0;
			}
			// move others to catch( so that the previous one for example doesnt just stop if its near the ball )
			moveRankedToCatch(match);
			// stop player who has the selection now
			stopMovement(match->playerInfo, match->pII.controlIndex);
			// but start moving again if movement key being held at the same time. for smooth movement.
			// smoothOutMovement still needs StateInfo due to ActionFlags being in Local but also needing KeyStates?
			// Wait, smoothOutMovement implementation:
			/*
			void smoothOutMovement(StateInfo* stateInfo)
			{
				int j;
				for(j = 0; j < DIRECTION_COUNT; j++) {
					if(stateInfo->match->aF.cTAF.move[j] == 2) {
						stateInfo->match->aF.cTAF.move[j] = 1;
					}
				}
			}
			*/
			// It only uses MatchSession! I'll narrow it later.
		}
	}

}

void prepareBatter(MatchSession* match)
{
	int batterIndex = get_active_batter_index(match);
	if(batterIndex != -1) {
		// batter ready model
		match->playerInfo[batterIndex].cPI.model = PLAYER_ANIM_BATTER_READY;
		// can pitch now
		match->pRAI.batterReady = 1;
		// waiting for pitch to go in air before starting the batting movement
		match->aF.bTAF.swing = 0;
		// batterIndex has been selected before calling this function
		match->referee.battingPlayers[batterIndex].currentSafetyBase = BASE_HOME;
		// and initialize batter so that everything is ready to go.
		match->pRAI.initBatter = 1;
	}
}
// so here we calculate index and base of the player who is the leadrunner so that
// we can move him if thats the decision.
void calculateFreeWalk(MatchSession* match)
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
		if(match->playerInfo[index].bTPI.baseId != BASE_NONE &&match->playerInfo[index].bTPI.state != PLAYER_STATE_WOUNDED) {
			BaseID currentBaseId = match->playerInfo[index].bTPI.baseId;

			// SANITY CHECK: If player is at HOME base, they MUST be the active batter.
			int batterIndex = get_active_batter_index(match);
			if (currentBaseId == BASE_HOME &&index != batterIndex) {
				printf("[CRITICAL LOGIC ERROR] Ghost Runner detected! Player %d is at BASE_HOME but batterIndex is %d. Ignoring.\n", index, batterIndex);
				continue;
			}

			// Use base_cmp to compare bases semantically (replaces >=)
			if(base_cmp(currentBaseId, maxBaseId) >= 0) {
				if(currentBaseId == maxBaseId) {
					if(base_cmp(match->referee.battingPlayers[index].baseAtPitchStart, maxBaseAtPitchStart) > 0) {
						maxBaseId = currentBaseId;
						maxBaseAtPitchStart = match->referee.battingPlayers[index].baseAtPitchStart;
						maxIndex = index;
					}
				} else {
					maxBaseId = currentBaseId;
					maxBaseAtPitchStart = match->referee.battingPlayers[index].baseAtPitchStart;
					maxIndex = index;
				}
			}

		}
	}
	match->gameControl.freeWalkIndex = maxIndex;
	if(maxIndex != -1) match->gameControl.freeWalkBase = match->playerInfo[maxIndex].bTPI.baseId;
	else match->gameControl.freeWalkBase = BASE_NONE;
}
/*
	Here we initialize all the locations and velocities so that players will be in their correct
	positions and orientations on the field and ball looks like its thrown to pitcher. Models
	are updated also. Before calling this the fielding team must have its position-attributes filled.
*/
void initializeSpatialPlayerInformation(MatchSession* match, FieldPositions* fieldPositions, unsigned int* rng_seed)
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
	match->ballInfo.velocity.x = BALL_INIT_SPEED_X;
	match->ballInfo.velocity.y = BALL_INIT_SPEED_Y;
	match->ballInfo.velocity.z = BALL_INIT_SPEED_Z;
	match->ballInfo.location.x = BALL_INIT_LOCATION_X;
	match->ballInfo.location.y = BALL_INIT_LOCATION_Y;
	match->ballInfo.location.z = BALL_INIT_LOCATION_Z;

	match->ballInfo.lastLocation.x = match->ballInfo.location.x;
	match->ballInfo.lastLocation.y = match->ballInfo.location.y;
	match->ballInfo.lastLocation.z = match->ballInfo.location.z;
	// set locations and models for batting team players
	for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		float radiusFix = (float)(1.0f + fabs(5.5f - battingTeamPlacement[i])/20.0f);
		match->playerInfo[i].cPI.model = PLAYER_ANIM_STAND_BARE;

		match->playerInfo[i].tPI.homeLocation.x = (float)(fieldPositions->pitchPlate.x +
		    (HOME_RADIUS) * radiusFix * cos(PI - (battingTeamPlacement[i]+1)*PI/(PLAYERS_IN_TEAM + JOKER_COUNT + 1)));
		match->playerInfo[i].tPI.homeLocation.y = BALL_HEIGHT_WITH_PLAYER;
		match->playerInfo[i].tPI.homeLocation.z = (float)(fieldPositions->pitchPlate.z +
		    (HOME_RADIUS) * radiusFix * sin(PI - (battingTeamPlacement[i]+1)*PI/(PLAYERS_IN_TEAM + JOKER_COUNT + 1)));
		match->playerInfo[i].tPI.location.x = match->playerInfo[i].tPI.homeLocation.x;
		match->playerInfo[i].tPI.location.y = match->playerInfo[i].tPI.homeLocation.y;
		match->playerInfo[i].tPI.location.z = match->playerInfo[i].tPI.homeLocation.z;
		match->playerInfo[i].tPI.orientation.x = match->ballInfo.location.x - match->playerInfo[i].tPI.location.x;
		match->playerInfo[i].tPI.orientation.y = match->ballInfo.location.y - match->playerInfo[i].tPI.location.y;
		match->playerInfo[i].tPI.orientation.z = match->ballInfo.location.z - match->playerInfo[i].tPI.location.z;
		match->playerInfo[i].tPI.lastLocation.x = match->playerInfo[i].tPI.location.x;
		match->playerInfo[i].tPI.lastLocation.y = match->playerInfo[i].tPI.location.y;
		match->playerInfo[i].tPI.lastLocation.z = match->playerInfo[i].tPI.location.z;
	}
	// set locations and models for fielders. here we need positions set.
	for(i = 12; i < 2*PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		match->playerInfo[i].cPI.model = PLAYER_ANIM_STAND_NO_BALL;

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
		match->playerInfo[i].tPI.homeLocation.x = fieldPosition->x;
		match->playerInfo[i].tPI.homeLocation.y = fieldPosition->y;
		match->playerInfo[i].tPI.homeLocation.z = fieldPosition->z;

		match->playerInfo[i].tPI.location.x = match->playerInfo[i].tPI.homeLocation.x;
		match->playerInfo[i].tPI.location.y = match->playerInfo[i].tPI.homeLocation.y;
		match->playerInfo[i].tPI.location.z = match->playerInfo[i].tPI.homeLocation.z;

		match->playerInfo[i].tPI.orientation.x = match->ballInfo.location.x - match->playerInfo[i].tPI.location.x;
		match->playerInfo[i].tPI.orientation.y = match->ballInfo.location.y - match->playerInfo[i].tPI.location.y;
		match->playerInfo[i].tPI.orientation.z = match->ballInfo.location.z - match->playerInfo[i].tPI.location.z;

		match->playerInfo[i].tPI.lastLocation.x = match->playerInfo[i].tPI.location.x;
		match->playerInfo[i].tPI.lastLocation.y = match->playerInfo[i].tPI.location.y;
		match->playerInfo[i].tPI.lastLocation.z = match->playerInfo[i].tPI.location.z;
	}
	// these are set for every player.
	for(i = 0; i < 2*PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		match->playerInfo[i].tPI.velocity.x = 0.0f;
		match->playerInfo[i].tPI.velocity.y = 0.0f;
		match->playerInfo[i].tPI.velocity.z = 0.0f;
		match->playerInfo[i].tPI.targetLocation.x = 0.0f;
		match->playerInfo[i].tPI.targetLocation.y = 0.0f;
		match->playerInfo[i].tPI.targetLocation.z = 0.0f;
	}
}
// here we initialize players' stat information, and team information, whether they are joker or not,
// their number etc. all is information that is not going to be reinitialized when out of bounds -
// situation happens
void initializeInningPermanentPlayerInformation(MatchSession* match, Scoreboard* scoreboard, TeamData* teamData)
{
	int battingTeamIndex = (scoreboard->
	                        inning+scoreboard->playsFirst+scoreboard->period)%2;
	int i;
	int jokerCounter = 0;

	// initialize batting team numbers and jokerness
	for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		if(i < PLAYERS_IN_TEAM) {
			match->playerInfo[scoreboard->teams[battingTeamIndex].
			                  batterOrder[i]].bTPI.number = i + 1;
			match->playerInfo[scoreboard->teams[battingTeamIndex].
			                  batterOrder[i]].bTPI.joker = JOKER_REGULAR;
		} else {
			match->playerInfo[scoreboard->teams[battingTeamIndex].
			                  batterOrder[i]].bTPI.number = 0;
			match->playerInfo[scoreboard->teams[battingTeamIndex].
			                  batterOrder[i]].bTPI.joker = JOKER_AVAILABLE;
		}
	}
	// initialize batting team
	for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		match->playerInfo[i].cPI.team = TEAM_BATTING;

		if(match->playerInfo[i].bTPI.joker == JOKER_AVAILABLE) {
			match->pII.jokerIndices[jokerCounter] = i;
			jokerCounter++;
		}
		match->playerInfo[i].bTPI.name = teamData[(scoreboard->teams[battingTeamIndex]
		                                 .value - 1)].players[i].name;
		match->playerInfo[i].bTPI.power = teamData[(scoreboard->teams[battingTeamIndex]
		                                  .value - 1)].players[i].power;
		match->playerInfo[i].bTPI.speed = teamData[(scoreboard->teams[battingTeamIndex]
		                                  .value - 1)].players[i].speed;


	}
	// initialize fielders
	for(i = 12; i < 2*PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		match->playerInfo[i].cPI.team = TEAM_CATCHING;

		// here we set catcherOnBaseIndices and catcherReplacerOnBaseIndices.
		switch(i-12) {
		case 0:
			match->pII.catcherOnBaseIndex[0] = i;
			break;
		case 1:
			match->pII.catcherOnBaseIndex[1] = i;
			break;
		case 2:
			match->pII.catcherOnBaseIndex[2] = i;
			break;
		case 3:
			match->pII.catcherOnBaseIndex[3] = i;
			break;
		case 4:
			match->pII.catcherReplacerOnBaseIndex[0] = i;
			match->pII.catcherReplacerOnBaseIndex[1] = i;
			break;
		case 5:
			match->pII.catcherReplacerOnBaseIndex[3] = i;
			break;
		case 6:
			match->pII.catcherReplacerOnBaseIndex[2] = i;
			break;

		}
	}
}
// information that can be flushed
void initializeNonCriticalPlayerInformation(MatchSession* match)
{
	int i, j;
	for( i = 0; i < 2*PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		// MILESTONE 7.5: Initialize control state
		match->playerRuntime[i].arrivedToBase = 0;
		match->playerRuntime[i].passedPathPoint = 0;
		match->playerRuntime[i].goingForward = 0;
		match->playerRuntime[i].hasMadeRunOnThirdBase = 0;

		match->playerInfo[i].cPI.animationFrequency = 1;
		match->playerInfo[i].cPI.animationStage = 0;
		match->playerInfo[i].cPI.animationStageCount = 0;
		match->playerInfo[i].cPI.moving = 0;
		match->playerInfo[i].cPI.running = 0;
		match->playerInfo[i].cPI.looksForTarget = 0;
		match->playerInfo[i].cPI.lastLastLocationUpdate = 1;

		// Critical: throwRecoil must be 0 for all players so moveToTarget doesn't block
		match->playerInfo[i].cTPI.throwRecoil = 0;

		if( i >= PLAYERS_IN_TEAM + JOKER_COUNT) {

			match->playerInfo[i].cTPI.isNearHomeLocation = 1;
			match->playerInfo[i].cTPI.replacingStage = REPLACEMENT_IDLE;
			match->playerInfo[i].cTPI.replacingBase = BASE_NONE;
			match->playerInfo[i].cTPI.busyCatching = 0;
			// throwRecoil already set above
			// initialize fielderRankedIndices with the indices of five first
			// players in positional order.
			if(i-12 < RANKED_FIELDERS_COUNT) {
				match->pII.fielderRankedIndices
				[i-12] = i;
			}

			for(j = 0; j < DIRECTION_COUNT; j++) {
				match->playerInfo[i].cTPI.movesToDirection[j] = 0;
			}
		} else {
			match->playerInfo[i].bTPI.state = PLAYER_STATE_IDLE;
			match->playerInfo[i].bTPI.baseId = BASE_NONE;
		}
	}

}
// this information is important for correct continuity after foul play.
void initializeCriticalBattingTeamInformation(MatchSession* match)
{
	int i;
	for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		match->referee.battingPlayers[i].baseAtPitchStart = BASE_NONE;
	}
}
// ball flags
void initializeBallInfo(MatchSession* match)
{
	match->ballInfo.visible = 1;
	match->ballInfo.moving = 1;
	match->ballInfo.hasHitGround = 0;
	match->ballInfo.onGround = 0;
	match->ballInfo.hitsGroundToUnWound = 0;
	match->ballInfo.hasHitGroundOutOfBounds = 0;
	match->ballInfo.needsMoveUpdate = 0;
	match->ballInfo.lastLastLocationUpdate = 0;
}
// action flag initialization
void initializeActionInfo(MatchSession* match)
{
	int i;

	for(i = 0; i < BASE_COUNT; i++) {
		match->aF.bTAF.baseRun[i] = 0;
	}
	match->aF.bTAF.chooseBatter = 0;
	match->aF.bTAF.takeFreeWalk = 0;
	match->aF.bTAF.swing = 0;
	match->aF.bTAF.increaseBatterAngle = 0;
	match->aF.bTAF.decreaseBatterAngle = 0;

	for(i = 0; i < BASE_COUNT; i++) {
		match->aF.cTAF.move[i] = 0;
		match->aF.cTAF.throwToBase[i] = 0;
	}
	match->aF.cTAF.changePlayer = 0;
	match->aF.cTAF.dropBall = 0;
	match->aF.cTAF.pitch = 0;
	match->aF.cTAF.actionKeyLock = 0;
}
// these can be flushed
void initializeTemporaryGameAnalysisInfo(MatchSession* match)
{
	match->gameControl.freeWalkCalculationMade = 1;
	match->gameControl.waitingForBatterDecision = 0;
	match->gameControl.waitingForFreeWalkDecision = 0;
	match->halfInningState.outOfBounds = 0;
	match->playerCounters.noMorePlayers = 0;
	match->gameFlowState.ballHome = 0;
	match->halfInningState.endPeriod = 0;
	match->referee.woundingCatchPending = 0;
	match->referee.woundingCatchHandled = 0;

	match->halfInningState.event = EVENT_NONE;
	match->gameControl.freeWalkIndex = -1;
	match->gameControl.freeWalkBase = -1;
	match->gameEvents.playerArrivedAtBase = 0;
	match->gameControl.pause = 0;

	// MILESTONE 16: Initialize new structures (Phase 1)
	// GameEvents (transient, will be cleared each frame)
	clearFrameEvents(&match->gameEvents);

	// GameControl (stateful)
	match->gameControl.pause = 0;
	match->gameControl.waitingForBatterDecision = 0;
	match->gameControl.waitingForFreeWalkDecision = 0;
	match->gameControl.freeWalkCalculationMade = 1;
	match->gameControl.freeWalkIndex = -1;
	match->gameControl.freeWalkBase = BASE_NONE;
	match->gameControl.catchHasBeenMade = 0;

	initGameAnalysis(&(match->gameFlowState));
	initGameManipulation(&(match->gameFlowState));

	match->homeRunContestState.forceNextPair = 0;
	match->cameraState.homeRunCameraFlag = 0;
	match->cameraState.targetPoint.x = 0.0f;
	match->cameraState.targetPoint.y = 0.0f;
	match->cameraState.targetPoint.z = 0.0f;
}

void clearFrameEvents(GameEvents* events)
{
	events->catchMade = 0;
	events->playerArrivedAtBase = 0;
	events->pitchStarted = 0;
	events->pitchReleased = 0;
	events->ballHitByBat = 0;
	events->ballMissedByBat = 0;
	events->ballHitGround = 0;
	events->freeWalkAccepted = 0;
	events->freeWalkRejected = 0;
	events->outOfBoundsOccurred = 0;
}

// these should be kept when foul play
void initializeCriticalGameInfo(MatchSession* match, Scoreboard* scoreboard)
{
	int battingTeamIndex = (scoreboard->
	                        inning+scoreboard->playsFirst+scoreboard->period)%2;
	match->halfInningState.outs = 0;
	match->halfInningState.balls = 0;
	match->halfInningState.strikes = 0;
	match->playerCounters.nonJokerPlayersLeft = PLAYERS_IN_TEAM;
	match->playerCounters.jokersLeft = 3;
	match->halfInningState.runsInTheInning = 0;
	match->pII.batterSelectionIndex =
	    scoreboard->teams[battingTeamIndex].batterOrder[scoreboard->teams[battingTeamIndex].batterOrderIndex];
}
// index information initialization, can be called when out of bounds
void initializeIndexInformation(MatchSession* match)
{
	match->pII.hasBallIndex = -1;
	match->pII.lastHadBallIndex = -1;
	match->pII.controlIndex = -1;
	match->pII.changePlayerArrayIndex = -1;
}
// player-related action information initialization, can be called when foul play.
void initializePRAIInformation(MatchSession* match)
{
	int i;
	match->pRAI.pitchState = PITCH_STAGE_NONE;
	match->pRAI.meterValue = 0.0f;
	match->pRAI.swingMeterValue = 0.0f;
	match->pRAI.battingGoingOn = 0;
	match->pRAI.batterCanAdvance = 0;
	match->pRAI.batHit = 0;
	match->pRAI.batMiss = 0;
	match->pRAI.throwGoingToBase = -1;
	match->pRAI.batterReady = 0;
	match->pRAI.refreshCatchAndChange = 0;
	match->pRAI.initPlayerSelection = 0;
	match->pRAI.initBatter = 0;
	for(i = 0; i < BASE_COUNT; i++) {
		match->pRAI.willStartRunning[i] = 0;
	}
}

void setRunnerAndBatter(MatchSession* match, Scoreboard* scoreboard, FieldPositions* fieldPositions)
{
	int battingTeamIndex = (scoreboard->
	                        inning+scoreboard->playsFirst+scoreboard->period)%2;
	Vector3D target;
	int i;
	if(match->homeRunContestState.runnerBatterPairCounter < scoreboard->pairCount) {
		int runnerIndex = scoreboard->teams[battingTeamIndex].
		                  batterRunnerIndices[1][match->homeRunContestState.runnerBatterPairCounter];
		int batterIndex = scoreboard->teams[battingTeamIndex].
		                  batterRunnerIndices[0][match->homeRunContestState.runnerBatterPairCounter];
		// batter
		if(batterIndex != -1) {
			match->playerInfo[batterIndex].bTPI.baseId = BASE_HOME;
			match->playerInfo[batterIndex].bTPI.state = PLAYER_STATE_AT_BAT;
			match->referee.battingPlayers[batterIndex].baseAtPitchStart = BASE_HOME;
			match->playerInfo[batterIndex].bTPI.number = match->homeRunContestState.runnerBatterPairCounter + 1;
			// move player to default batter ready position
			target.x = (float)(fieldPositions->pitchPlate.x + cos(ZERO_BATTING_ANGLE)*BATTING_RADIUS);
			target.z = (float)(fieldPositions->pitchPlate.z - sin(ZERO_BATTING_ANGLE)*BATTING_RADIUS);
			moveToTarget(match->playerInfo, batterIndex, &target);
		}
		// runner
		if(runnerIndex != -1) {
			match->playerInfo[runnerIndex].bTPI.baseId = BASE_THIRD;
			match->playerInfo[runnerIndex].bTPI.state = PLAYER_STATE_ON_BASE;
			match->referee.battingPlayers[runnerIndex].baseAtPitchStart = BASE_THIRD;
			match->referee.battingPlayers[runnerIndex].hadSafetyAtPitchStart = 1; // Correctness
			match->referee.battingPlayers[runnerIndex].currentSafetyBase = BASE_THIRD;

			match->playerInfo[runnerIndex].tPI.location.x =
			    fieldPositions->thirdBaseRun.x;
			match->playerInfo[runnerIndex].tPI.location.y =
			    fieldPositions->thirdBaseRun.y;
			match->playerInfo[runnerIndex].tPI.location.z =
			    fieldPositions->thirdBaseRun.z;
			match->playerInfo[runnerIndex].tPI.lastLocation.x =
			    match->playerInfo[runnerIndex].tPI.location.x;
			match->playerInfo[runnerIndex].tPI.lastLocation.y =
			    match->playerInfo[runnerIndex].tPI.location.y;
			match->playerInfo[runnerIndex].tPI.lastLocation.z =
			    match->playerInfo[runnerIndex].tPI.location.z;
			match->playerInfo[runnerIndex].tPI.orientation.x =
			    -match->playerInfo[runnerIndex].tPI.location.x;
			match->playerInfo[runnerIndex].tPI.orientation.y = 0.0f;
			match->playerInfo[runnerIndex].tPI.orientation.z =
			    -match->playerInfo[runnerIndex].tPI.location.x;
		}
		// set other runners next to the third base.
		for(i = match->homeRunContestState.runnerBatterPairCounter + 1; i < scoreboard->pairCount; i++) {
			int index = scoreboard->teams[battingTeamIndex].batterRunnerIndices[1][i];
			if(index != -1) {
				match->playerInfo[index].tPI.location.x = fieldPositions->thirdBaseRun.x -
				    2.0f - (i-(match->homeRunContestState.runnerBatterPairCounter + 1))*1.5f;
				match->playerInfo[index].tPI.location.y =
				    fieldPositions->thirdBaseRun.y;
				match->playerInfo[index].tPI.location.z =
				    fieldPositions->thirdBaseRun.z;
				match->playerInfo[index].tPI.lastLocation.x =
				    match->playerInfo[index].tPI.location.x;
				match->playerInfo[index].tPI.lastLocation.y =
				    match->playerInfo[index].tPI.location.y;
				match->playerInfo[index].tPI.lastLocation.z =
				    match->playerInfo[index].tPI.location.z;
				match->playerInfo[index].tPI.orientation.x =
				    -match->playerInfo[index].tPI.location.x;
				match->playerInfo[index].tPI.orientation.y = 0.0f;
				match->playerInfo[index].tPI.orientation.z =
				    -match->playerInfo[index].tPI.location.x;
			}
		}
	}
}

void loadMutableWorldSettings(StateInfo* stateInfo, unsigned int* rng_seed)
{
	/*
	* called always when half-inning starts.
	*
	*/
	// initialize ball flags
	initializeBallInfo(stateInfo->match);
	// action flags
	initializeActionInfo(stateInfo->match);
	// game analysis information that can be flushed when foul play happens
	initializeTemporaryGameAnalysisInfo(stateInfo->match);
	// game information that should not be initialized before the inning ends
	initializeCriticalGameInfo(stateInfo->match, &stateInfo->match->scoreboard);
	// index information that can be flushed
	initializeIndexInformation(stateInfo->match);
	// player-related action information that can be flushed
	initializePRAIInformation(stateInfo->match);
	// this is information that stays for the whole inning
	initializeInningPermanentPlayerInformation(stateInfo->match, &stateInfo->match->scoreboard, stateInfo->teamData);
	// information about location and models and orientations. will be flushed when foul play happens
	initializeSpatialPlayerInformation(stateInfo->match, stateInfo->fieldPositions, rng_seed);
	// information about players than can be flushed.
	initializeNonCriticalPlayerInformation(stateInfo->match);
	// information that cant be flushed when foul play. like baseAtPitchStart.
	initializeCriticalBattingTeamInformation(stateInfo->match);
	// initialize referee state
	initializeRefereeState(&stateInfo->match->referee);

	if(stateInfo->match->scoreboard.period >= 4) {
		if(!(stateInfo->match->homeRunContestState.runnerBatterPairCounter > 0 &&
		        stateInfo->match->homeRunContestState.runnerBatterPairCounter <
		        stateInfo->match->scoreboard.pairCount)) {
			stateInfo->match->homeRunContestState.runnerBatterPairCounter = 0;
		}
		setRunnerAndBatter(stateInfo->match, &stateInfo->match->scoreboard, stateInfo->fieldPositions);
	}
}