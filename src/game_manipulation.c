#include "globals.h"

#include "game_manipulation.h"
#include "game_manipulation_internal.h"
#include "common_logic.h"

// this module tries to do a lot of dirty work of analyzing spatial data and moving things and then
// also setting flags for more sophisticated modules.

void gameManipulation()
{
	// init?
	if(stateInfo.localGameInfo->gAI.initLocals > 0)
	{
		initGameManipulation();
		stateInfo.localGameInfo->gAI.initLocals++;
		if(stateInfo.localGameInfo->gAI.initLocals == INIT_LOCALS_COUNT)
		{
			stateInfo.localGameInfo->gAI.initLocals = 0;
		}
	}

	updateBallStatus(); // update ball's location due to velocity, also update ball's velocity due to gravity, and set some flags related to ball
	checkIfBallCanBeCatched(); //  if no one has the ball, it could be catched, couldn't it?
	checkIfNearHomeLocation(); //
	baseRunnerMovementsOnBaseArrivals();
	basemenReplacements();
	moveIdlingPlayersToHomeLocation();
	rankPlayersAndMoveThem(); // ranks players and moves them when needed. its ranked lists are also used for changing players.
	playerLocationOrientationAndTargets(); // update every players' location and orientation according to player's velocity and ball's location.
	updateBallToPlayer(); // if some player has the ball, update ball velocity so that the ball will follow the player
	updateModels();
}

void initGameManipulation()
{
	closeToGround = 0;
}


static __inline void updateBallStatus()
{
	if(stateInfo.localGameInfo->ballInfo.lastLastLocationUpdate == 1)
	{
			setVectorV(&(stateInfo.localGameInfo->ballInfo.lastLocation), &(stateInfo.localGameInfo->ballInfo.location));
			stateInfo.localGameInfo->ballInfo.lastLastLocationUpdate = 0;
	}
	// check first if ball's moving flag is 1, so that no overhead of doing updates on nothing.
	if(stateInfo.localGameInfo->ballInfo.moving == 1)
	{
		// update lastLocation and location.
		setVectorV(&(stateInfo.localGameInfo->ballInfo.lastLocation), &(stateInfo.localGameInfo->ballInfo.location));
		addToVectorV(&(stateInfo.localGameInfo->ballInfo.location), &(stateInfo.localGameInfo->ballInfo.velocity));
		if(stateInfo.localGameInfo->pII.hasBallIndex == -1)
		{
			// if ball is free then we make sure that it stays within the play area.
			if(stateInfo.localGameInfo->ballInfo.location.z > FIELD_FRONT && stateInfo.localGameInfo->ballInfo.velocity.z > 0)
			{
				stateInfo.localGameInfo->ballInfo.velocity.z = -stateInfo.localGameInfo->ballInfo.velocity.z*BALL_SLOW_FACTOR_Y;
			}
			else if(stateInfo.localGameInfo->ballInfo.location.z < FIELD_BACK && stateInfo.localGameInfo->ballInfo.velocity.z < 0)
			{
				stateInfo.localGameInfo->ballInfo.velocity.z = -stateInfo.localGameInfo->ballInfo.velocity.z*BALL_SLOW_FACTOR_Y;
			}
			if(stateInfo.localGameInfo->ballInfo.location.x > FIELD_RIGHT && stateInfo.localGameInfo->ballInfo.velocity.x > 0)
			{
				stateInfo.localGameInfo->ballInfo.velocity.x = -stateInfo.localGameInfo->ballInfo.velocity.x*BALL_SLOW_FACTOR_Y;
			}
			else if(stateInfo.localGameInfo->ballInfo.location.x < FIELD_LEFT && stateInfo.localGameInfo->ballInfo.velocity.x < 0)
			{
				stateInfo.localGameInfo->ballInfo.velocity.x = -stateInfo.localGameInfo->ballInfo.velocity.x*BALL_SLOW_FACTOR_Y;
			}
			if(stateInfo.localGameInfo->ballInfo.onGround == 0)
			{
				// if ball is not on the ground yet, let it be affected by gravity.
				stateInfo.localGameInfo->ballInfo.velocity.y -= GRAVITY;
				// if ball now comes close to the ground
				if(stateInfo.localGameInfo->ballInfo.location.y < BALL_SIZE / 2)
				{
					int outOfBounds = 0;
					// we check out of bounds situation only if we are close to ground to avoid
					// unnecesaary overhead
					if(closeToGround == 0)
					{
						closeToGround = 1;
						outOfBounds = checkIfBallIsOutOfBounds();
					}
					// it could be its first time
					if(stateInfo.localGameInfo->ballInfo.hasHitGround == 0) // to avoid situation where it feels like player changing key doesnt work
					{															// because if this wasnt here, rankedindicesarrayindex would be always initialized to 0 with every change.
						stateInfo.localGameInfo->ballInfo.hasHitGround = 1;
						stateInfo.localGameInfo->gAI.woundingCatch = 0;
						// this is used to track if ball has been dropped after a catch to avoid wounding
						stateInfo.localGameInfo->ballInfo.hitsGroundToUnWound = 1;
						// if no catch made and bat hit and we hit the ground and ball is out of bounds,
						// we are going to have a foul play.
						if(stateInfo.localGameInfo->pRAI.batHit == 1 &&
							stateInfo.localGameInfo->gAI.firstCatchMade == 0)
						{
							if(outOfBounds == 1)
							{
								stateInfo.localGameInfo->gAI.outOfBounds = 1;
							}
						}
					}
					// if pitch is going on, it means that batter didnt bat or missed and
					// we need to set pitch-flags to 0 and make checks for strikes and balls and trigger corresponding events
					if(stateInfo.localGameInfo->pRAI.pitchGoingOn == 1)
					{
						// in ball hits the plate, it is a strike, if not its a ball
						if(stateInfo.localGameInfo->ballInfo.location.x < PLATE_WIDTH/2 && stateInfo.localGameInfo->ballInfo.location.x > -PLATE_WIDTH/2)
						{
							if(stateInfo.localGameInfo->pRAI.batMiss != 1)
							{
								stateInfo.localGameInfo->gAI.strikes += 1;
								stateInfo.localGameInfo->gAI.gameInfoEvent = 5;
							}
						}
						else
						{
							if(stateInfo.localGameInfo->pRAI.batMiss != 1)
							{
								stateInfo.localGameInfo->gAI.balls += 1;
								stateInfo.localGameInfo->gAI.gameInfoEvent = 6;

								// here we also set freeWalk flags because it could be possible that batting team now
								// has right for free walk.
								stateInfo.localGameInfo->gAI.freeWalkCalculationMade = 0;
								stateInfo.localGameInfo->gAI.freeWalkIndex = -1;
								stateInfo.localGameInfo->gAI.freeWalkBase = -1;
							}
						}
						stateInfo.localGameInfo->pRAI.pitchInAir = 0;
						stateInfo.localGameInfo->pRAI.pitchGoingOn = 0;
					}
					// if ball has enough y-velocity it will bounce
					if(stateInfo.localGameInfo->ballInfo.velocity.y < -BALL_BOUNCE_THRESHOLD)
					{
						// so try to update ranks of fielders
						stateInfo.localGameInfo->pRAI.refreshCatchAndChange = 1;
						// change the direction of y-velocity
						stateInfo.localGameInfo->ballInfo.velocity.y = -stateInfo.localGameInfo->ballInfo.velocity.y;
						setVectorXYZ(&(stateInfo.localGameInfo->ballInfo.velocity), BALL_SLOW_FACTOR_X*stateInfo.localGameInfo->ballInfo.velocity.x,
							BALL_SLOW_FACTOR_Y*stateInfo.localGameInfo->ballInfo.velocity.y, BALL_SLOW_FACTOR_Z*stateInfo.localGameInfo->ballInfo.velocity.z);

					}
					// if instead we dont have enough y-velocity to bounce, we just set velocity to 0, location to ground and
					// change ranked indices for the last time.
					else if(stateInfo.localGameInfo->ballInfo.velocity.y < 0)
					{
						stateInfo.localGameInfo->ballInfo.onGround = 1;
						stateInfo.localGameInfo->pRAI.refreshCatchAndChange = 1;
						stateInfo.localGameInfo->ballInfo.location.y = BALL_SIZE / 2;
						stateInfo.localGameInfo->ballInfo.velocity.y = 0.0f;
					}
				}
				else
				{
					if(closeToGround == 1)
					{
						closeToGround = 0;
					}
				}

			}
			else
			{
				// if we are rolling on the ground already, slow down the velocity a bit and stop ball if x and z -velocities are small enough
				setVectorXZ(&(stateInfo.localGameInfo->ballInfo.velocity), BALL_ON_GROUND_SLOW_FACTOR*stateInfo.localGameInfo->ballInfo.velocity.x,
					BALL_ON_GROUND_SLOW_FACTOR*stateInfo.localGameInfo->ballInfo.velocity.z);
				if(isVectorSmallEnoughCircleXZV(&(stateInfo.localGameInfo->ballInfo.velocity), EPSILON))
				{
					setVectorXZ(&(stateInfo.localGameInfo->ballInfo.velocity), 0.0f, 0.0f);
					stateInfo.localGameInfo->pRAI.refreshCatchAndChange = 1;
					stateInfo.localGameInfo->ballInfo.moving = 0;
					stateInfo.localGameInfo->ballInfo.lastLastLocationUpdate = 1;
				}
			}
		}
	}
}

static __inline void updateBallToPlayer()
{
	if(stateInfo.localGameInfo->pII.hasBallIndex != -1)
	{
		// if someone has ball and that someone is moving and ball movement needs updating, give it same v as the player has
		if(stateInfo.localGameInfo->playerInfo[stateInfo.localGameInfo->pII.hasBallIndex].cPI.moving == 1)
		{
			if(stateInfo.localGameInfo->ballInfo.needsMoveUpdate == 1)
			{
					stateInfo.localGameInfo->ballInfo.moving = 1;
					stateInfo.localGameInfo->ballInfo.needsMoveUpdate = 0;
					setVectorXYZ(&(stateInfo.localGameInfo->ballInfo.velocity), stateInfo.localGameInfo->playerInfo[stateInfo.localGameInfo->pII.hasBallIndex]
						.tPI.velocity.x, 0.0f, stateInfo.localGameInfo->playerInfo[stateInfo.localGameInfo->pII.hasBallIndex].tPI.velocity.z);
			}
		}
		// if someone has ball and that someone doesn't move but ball still moves, stop the ball
		else
		{
			if(stateInfo.localGameInfo->ballInfo.moving == 1)
			{
				stateInfo.localGameInfo->ballInfo.moving = 0;
				stateInfo.localGameInfo->ballInfo.lastLastLocationUpdate = 1;
				setVectorXYZ(&(stateInfo.localGameInfo->ballInfo.velocity), 0.0f, 0.0f, 0.0f);
			}
		}
	}
}


// so the goal is check if ball is close enough to any of the players and if so, do all the work needed
// to move game to post-catch state.
static __inline void checkIfBallCanBeCatched()
{
	int i;
	// so to go to do checking we should first confirm that it even is possible by checking that no one has ball
	// and pitch is not going on. also cant catch if it just happens to be frame of updating ranks
	// as that could make some weird things happen. try on the next frame again.
	if(stateInfo.localGameInfo->pII.hasBallIndex == -1 && stateInfo.localGameInfo->pRAI.pitchGoingOn == 0 &&
		stateInfo.localGameInfo->pRAI.refreshCatchAndChange != 1)
	{
		for(i = PLAYERS_IN_TEAM + JOKER_COUNT; i < PLAYERS_IN_TEAM * 2 + JOKER_COUNT; i++)
		{
			// so we confirm that this particular player is valid to catch, by checking his lastHadBallIndex.
			// its purpose is to deny players catching balls right after throwing them.
			// but if ball has hit ground, we dont care about lastHadBallIndex anymore.
			// allow players to drop balls and catch if thrown close to them by themselves.
			if ((i != stateInfo.localGameInfo->pII.lastHadBallIndex || stateInfo.localGameInfo->ballInfo.hasHitGround == 1))
			{
				Vector3D distance;
				float limit = CATCH_DISTANCE;
				distance.x = stateInfo.localGameInfo->playerInfo[i].tPI.location.x -
					stateInfo.localGameInfo->ballInfo.location.x;
				distance.z = stateInfo.localGameInfo->playerInfo[i].tPI.location.z -
					stateInfo.localGameInfo->ballInfo.location.z;
				distance.y = 2 * stateInfo.localGameInfo->playerInfo[i].tPI.location.y / 5 -
					stateInfo.localGameInfo->ballInfo.location.y;
				// check if ball is close enough to this particular player.
				if(isVectorSmallEnoughSphere(&distance, limit) == 1)
				{													 // weird things would happen
					int j;
					int baseCatcherFlag = 0;
					float p1x, p2x, p1z, p2z;
					if(stateInfo.localGameInfo->pII.lastHadBallIndex != -1)
					{
						p1x = stateInfo.localGameInfo->playerInfo[i].tPI.location.x;
						p1z = stateInfo.localGameInfo->playerInfo[i].tPI.location.z;
						p2x = stateInfo.localGameInfo->
							playerInfo[stateInfo.localGameInfo->pII.lastHadBallIndex].tPI.location.x;
						p2z = stateInfo.localGameInfo->
							playerInfo[stateInfo.localGameInfo->pII.lastHadBallIndex].tPI.location.z;
						if(stateInfo.localGameInfo->pRAI.throwGoingToBase != -1 && (i == stateInfo.localGameInfo->pII.catcherOnBaseIndex[stateInfo.localGameInfo->pRAI.throwGoingToBase] ||
								i == stateInfo.localGameInfo->pII.catcherReplacerOnBaseIndex[stateInfo.localGameInfo->
								pRAI.throwGoingToBase]))
						{
							baseCatcherFlag = 1;
						}

					}
					// to avoid annyoing twitching we need to check that catching player is far enough
					if(stateInfo.localGameInfo->pII.lastHadBallIndex == -1 || stateInfo.localGameInfo->ballInfo.hasHitGround == 1 ||
						baseCatcherFlag == 1 ||
						!isVectorSmallEnoughCircleXZ(p1x-p2x, p1z-p2z, PLAYER_TOO_CLOSE_TO_CATCH_LIMIT))
					{
						// ensure that player that previously was controlled doesnt continue his key-controlled movement when key is still down when control changes.
						// note this could be different player than the one who caught the ball.
						if(stateInfo.localGameInfo->pII.controlIndex != -1 && stateInfo.localGameInfo->playerInfo[stateInfo.localGameInfo->pII.controlIndex].cPI.moving == 1)
						{
							stopMovement(stateInfo.localGameInfo->pII.controlIndex);
						}
						// if throwRecoil is 1, model will be automatically set to 0 when animation ends
						// but if its 0, we must change it from having ball to not having ball.
						if(stateInfo.localGameInfo->pII.controlIndex != -1 && stateInfo.localGameInfo->playerInfo[stateInfo.localGameInfo->pII.controlIndex].cTPI.throwRecoil == 0)
						{
							stateInfo.localGameInfo->playerInfo[stateInfo.localGameInfo->pII.controlIndex].cPI.model = 0;
						}
						// set busyCatching to 0 for every one who was busy if ball is caught so that
						// players are free to be moved to their home locations.
						for(j = PLAYERS_IN_TEAM + JOKER_COUNT; j < PLAYERS_IN_TEAM * 2 + JOKER_COUNT; j++)
						{
							if(stateInfo.localGameInfo->playerInfo[j].cTPI.busyCatching == 1)
							{
								stateInfo.localGameInfo->playerInfo[j].cTPI.busyCatching = 0;
								// also stop them so that decisions about moving to home location can be made.
								stopMovement(j);
							}

						}
						// stop the guy who caught the ball
						stopMovement(i);
						// but start moving again if movement key being held at the same time. for smooth movement.
						smoothOutMovement();
						// set the has ball model.
						stateInfo.localGameInfo->playerInfo[i].cPI.model = 1;
						// wounding catchs are the catchs that come directly from the bat.
						if(stateInfo.localGameInfo->ballInfo.hasHitGround == 0 && stateInfo.localGameInfo->gAI.firstCatchMade == 0 &&
							stateInfo.localGameInfo->pRAI.batHit == 1)
						{
							stateInfo.localGameInfo->gAI.woundingCatch = 1;
						}
						// make sound
						stateInfo.playSoundEffect = SOUND_CATCH;
						// this could be the fifth but the first is still made.
						stateInfo.localGameInfo->gAI.firstCatchMade = 1;
						stateInfo.localGameInfo->pRAI.throwGoingToBase = -1;
						stateInfo.localGameInfo->pII.controlIndex = i;
						stateInfo.localGameInfo->pII.hasBallIndex = i;

						// some information for AI to make decisions
						stateInfo.localGameInfo->ballInfo.hasHitGroundOutOfBounds = 0;

						// and set running flag
						stateInfo.localGameInfo->playerInfo[stateInfo.localGameInfo->pII.controlIndex].cPI.running = 1;
						// stop the ball and hide it. if player starts to move again,
						// then ball's velocity will also be updated agian.
						stateInfo.localGameInfo->ballInfo.visible = 0;
						stateInfo.localGameInfo->ballInfo.moving = 0;
						stateInfo.localGameInfo->ballInfo.onGround = 0;
						stateInfo.localGameInfo->ballInfo.hasHitGround = 0;
						// and set ball's location to player's location.
						setVectorV(&(stateInfo.localGameInfo->ballInfo.location), &(stateInfo.localGameInfo->playerInfo[i].tPI.location));
					}
				}

			}
		}
	}
}
// when moving fielders automatically, we often use the knowledge of whether we are near home location or not
// so here we check if we thats the case
static __inline void checkIfNearHomeLocation()
{
	int i;
	for(i = PLAYERS_IN_TEAM + JOKER_COUNT; i < PLAYERS_IN_TEAM * 2 + JOKER_COUNT; i++)
	{
		// we check it only for moving players to reduce overhead a bit.
		if(stateInfo.localGameInfo->playerInfo[i].cPI.moving == 1)
		{

			float dx = stateInfo.localGameInfo->playerInfo[i].tPI.location.x -
				stateInfo.localGameInfo->playerInfo[i].tPI.homeLocation.x;
			float dz = stateInfo.localGameInfo->playerInfo[i].tPI.location.z -
				stateInfo.localGameInfo->playerInfo[i].tPI.homeLocation.z;
			// if we are already close, we check if happen to have gone out.
			if(stateInfo.localGameInfo->playerInfo[i].cTPI.isNearHomeLocation == 1)
			{
				if(isVectorSmallEnoughCircleXZ(dx, dz, DISTANCE_FROM_HOME_LOCATION_THRESHOLD) == 0)
				{
					stateInfo.localGameInfo->playerInfo[i].cTPI.isNearHomeLocation = 0;
				}
			}
			// if are out we check if happened to wander in.
			else if(stateInfo.localGameInfo->playerInfo[i].cTPI.isNearHomeLocation == 0)
			{
				if(isVectorSmallEnoughCircleXZ(dx, dz, DISTANCE_FROM_HOME_LOCATION_THRESHOLD) == 1)
				{
					stateInfo.localGameInfo->playerInfo[i].cTPI.isNearHomeLocation = 1;
				}
			}
		}
	}
}
// so here we are going to handle situations when baserunners arrive bases and we must handle those players' flags
// and players who were there already must have their status updated also etc
static __inline void baseRunnerMovementsOnBaseArrivals()
{
	// so everything starts with some player arriving base, this flag is set on target checking function.
	if(stateInfo.localGameInfo->gAI.playerArrivedToBase == 1)
	{
		int i;
		// we check every player who is a baserunner ( or batter )
		for(i = 0; i < BASE_COUNT; i++)
		{
			int index = stateInfo.localGameInfo->pII.battingTeamOnFieldIndices[i];
			// these indices can be -1 so we have to check for that and also we'll check if this particular player
			// was one who arrived. there can be many though so we cant break out after this.
			if(index != -1 && stateInfo.localGameInfo->playerInfo[index].bTPI.arrivedToBase == 1)
			{
				// no need to check on this player anymore when next runner arrives and this one has stayed where he was.
				stateInfo.localGameInfo->playerInfo[index].bTPI.arrivedToBase = 0;
				// if out has been made, player is removed immediately from the game so we wouldnt be here.
				// if wound has been made, player still tries to get to next base, so we have to handle that here.
				if(stateInfo.localGameInfo->playerInfo[index].bTPI.out != 1)
				{
					int j;
					// so after we have a valid player, we are going to check to which base he arrived
					// and after that do the actions corresponding to base and direction.
					// we dont have the code handling batter arriving homebase here as
					// it didn't need much of a work, so its handled in target checking function.
					// homeruns are checked here though, in that case base==4? so its out of this loop.
					for(j = 1; j < BASE_COUNT; j++)
					{
						if(stateInfo.localGameInfo->playerInfo[index].bTPI.base == j)
						{
							// if we were taking a free walk, now its done and we can set the flag off.
							if(stateInfo.localGameInfo->playerInfo[index].bTPI.takingFreeWalk == 1)
							{
								stateInfo.localGameInfo->playerInfo[index].bTPI.takingFreeWalk = 0;
							}
							// if we were wounded we must be removed out of the field and
							// also basemen already on the base must be removed as they get wounded too.
							if(stateInfo.localGameInfo->playerInfo[index].bTPI.wounded == 1)
							{
								stateInfo.localGameInfo->pII.battingTeamOnFieldIndices[i] = -1;
								stateInfo.localGameInfo->gAI.battingTeamPlayersOnFieldCount--;
								movePlayerOut(index);
								// if there was a player on this base before and he isnt safe here
								// it means he had tried to run to next base and is wounded already so need for this.
								// if there is a player safe here:
								if(stateInfo.localGameInfo->pII.safeOnBaseIndex[j] != -1)
								{
									int k;
									int fieldIndex = -1;
									// we need to find his index on battingTeamOnFieldIndices as he is going to be removed.
									for(k = 0; k < BASE_COUNT; k++)
									{
										if(stateInfo.localGameInfo->pII.battingTeamOnFieldIndices[k] ==
											stateInfo.localGameInfo->pII.safeOnBaseIndex[j])
										{
											fieldIndex = k;
											break;
										}
									}
									// we set the flags and off he goes.
									if(fieldIndex != -1)
									{
										stateInfo.localGameInfo->pII.battingTeamOnFieldIndices[fieldIndex] = -1;
										stateInfo.localGameInfo->gAI.battingTeamPlayersOnFieldCount--;
										stateInfo.localGameInfo->playerInfo[stateInfo.localGameInfo->pII.safeOnBaseIndex[j]].bTPI.wounded = 1;
										movePlayerOut(stateInfo.localGameInfo->pII.safeOnBaseIndex[j]);
										stateInfo.localGameInfo->pII.safeOnBaseIndex[j] = -1;
									}
								}
							}
							else
							{
								// if the player wasnt wounded, now he is arriving in a valid way and can be arriving to next base
								// (or the previous one. but if he arrives to previous one, nothing really
								// needs changing so we dont even set those flags in the target looking.)
								if(stateInfo.localGameInfo->pII.safeOnBaseIndex[j] != -1)
								{
									runToNextBase(stateInfo.localGameInfo->pII.safeOnBaseIndex[j], j);
								}
								// we are now safe here.
								stateInfo.localGameInfo->pII.safeOnBaseIndex[j] = index;
								// if we were safe on previous base ( no other player arrived there before we arrived here
								// or our safety was removed from there by a fielder), we set that that -1
								// to indicate we arent there anymore.
								if(stateInfo.localGameInfo->pII.safeOnBaseIndex[j-1] == index)
								{
									stateInfo.localGameInfo->pII.safeOnBaseIndex[j-1] = -1;
								}
								// if we arrived to base 3 and were originally from homebase
								if(stateInfo.localGameInfo->playerInfo[index].bTPI.base == 3 &&
									stateInfo.localGameInfo->playerInfo[index].bTPI.originalBase == 0 &&
									stateInfo.localGameInfo->gAI.outs < 3)
								{
									// set flag to check if our run is valid. difficult to imagine situation where
									// it wasnt right away, ball had to be swinged to like heaven and back again
									// so that it could be caught or be determined to be foul play after
									// player has run all the way from base 0 to 3. but anyway.
									stateInfo.localGameInfo->gAI.checkForRun = 1;
								}
							}
						}
					}
					// whether we got safely to first base or were wounded, we have to set batterIndex to -1
					// so that next player can go batting.
					if(stateInfo.localGameInfo->pII.batterIndex == index) stateInfo.localGameInfo->pII.batterIndex = -1;
					// if the player arrived to a base and had it marked as 4, it means that it possibly a run.
					if(stateInfo.localGameInfo->playerInfo[index].bTPI.base == 4)
					{
						// if we were safe at base 3, we are not anymore.
						if(stateInfo.localGameInfo->pII.safeOnBaseIndex[3] == index)
						{
							stateInfo.localGameInfo->pII.safeOnBaseIndex[3] = -1;
						}
						// if our originalBase was 0, we would have had a run at base 3 already so this wont be run
						// unless a new pitch is pitched and then our originalBase changes.
						if((stateInfo.localGameInfo->playerInfo[index].bTPI.originalBase != 0 ||
							stateInfo.localGameInfo->gAI.canMakeRunOfHonor == 0) &&
							stateInfo.localGameInfo->playerInfo[index].bTPI.takingFreeWalk == 0)
						{
							stateInfo.localGameInfo->gAI.checkForRun = 1;
						}
						else
						{
							// if was 0, we just remove this player from field.
							// if it wasnt 0, player will be removed afterwards.
							stateInfo.localGameInfo->pII.battingTeamOnFieldIndices[i] = -1;
							stateInfo.localGameInfo->gAI.battingTeamPlayersOnFieldCount--;
							stateInfo.localGameInfo->playerInfo[index].bTPI.base = -1;
						}

					}
				}
			}
		}
		// set the flag off as now everything has been handled.
		stateInfo.localGameInfo->gAI.playerArrivedToBase = 0;
	}
}

static __inline void basemenReplacements()
{
	int i;
	for(i = 0; i < BASE_COUNT; i++)
	{
		// if player is not in his home base
		if(stateInfo.localGameInfo->playerInfo[stateInfo.localGameInfo->pII.catcherOnBaseIndex[i]].cTPI.isNearHomeLocation == 0)
		{
			// start moving replacer there instead if he is not moving already or if player does not control him
			if(stateInfo.localGameInfo->pII.catcherReplacerOnBaseIndex[i] != stateInfo.localGameInfo->pII.controlIndex)
			{
				if(stateInfo.localGameInfo->playerInfo[stateInfo.localGameInfo->pII.catcherReplacerOnBaseIndex[i]].cTPI.replacingStage == 0)
				{
					moveToTarget(stateInfo.localGameInfo->pII.catcherReplacerOnBaseIndex[i],
						&(stateInfo.localGameInfo->playerInfo[stateInfo.localGameInfo->pII.catcherOnBaseIndex[i]].tPI.homeLocation));

					stateInfo.localGameInfo->playerInfo[stateInfo.localGameInfo->pII.catcherReplacerOnBaseIndex[i]].cTPI.replacingStage = 1;
					stateInfo.localGameInfo->playerInfo[stateInfo.localGameInfo->pII.catcherReplacerOnBaseIndex[i]].cTPI.replacingBase = i;
					// can go replacing even if was busy catching before.
					stateInfo.localGameInfo->playerInfo[stateInfo.localGameInfo->pII.catcherReplacerOnBaseIndex[i]].cTPI.busyCatching = 0;
				}
			}
		}
		// if player is on his base
		else if(stateInfo.localGameInfo->playerInfo[stateInfo.localGameInfo->pII.catcherOnBaseIndex[i]].cTPI.isNearHomeLocation == 1)
		{
			// and there's someone there or going there, move him back ( if player doesnt control )
			if(stateInfo.localGameInfo->pII.catcherReplacerOnBaseIndex[i] != stateInfo.localGameInfo->pII.controlIndex)
			{
				if(stateInfo.localGameInfo->playerInfo[stateInfo.localGameInfo->pII.catcherReplacerOnBaseIndex[i]].cTPI.replacingStage != 0)
				{
					if(stateInfo.localGameInfo->playerInfo[stateInfo.localGameInfo->pII.catcherReplacerOnBaseIndex[i]].cTPI.replacingBase == i)
					{
						moveToTarget(stateInfo.localGameInfo->pII.catcherReplacerOnBaseIndex[i],
							&(stateInfo.localGameInfo->playerInfo[stateInfo.localGameInfo->pII.catcherReplacerOnBaseIndex[i]].tPI.homeLocation));

						stateInfo.localGameInfo->playerInfo[stateInfo.localGameInfo->pII.catcherReplacerOnBaseIndex[i]].cTPI.replacingStage = 0;
						stateInfo.localGameInfo->playerInfo[stateInfo.localGameInfo->pII.catcherReplacerOnBaseIndex[i]].cTPI.replacingBase = -1;
					}
				}
			}
		}
	}
}

static __inline void moveIdlingPlayersToHomeLocation()
{
	int i;
	// if not catching ball or replacing or in home location, move to home location.
	for(i = PLAYERS_IN_TEAM + JOKER_COUNT; i < PLAYERS_IN_TEAM * 2 + JOKER_COUNT; i++)
	{
		if(stateInfo.localGameInfo->playerInfo[i].cPI.moving == 0)
		{
			if(i != stateInfo.localGameInfo->pII.controlIndex)
			{
				if(stateInfo.localGameInfo->playerInfo[i].cTPI.isNearHomeLocation == 0)
				{
					if(stateInfo.localGameInfo->playerInfo[i].cTPI.replacingStage == 0)
					{
						if(stateInfo.localGameInfo->playerInfo[i].cTPI.busyCatching == 0)
						{
							moveToTarget(i, &(stateInfo.localGameInfo->playerInfo[i].tPI.homeLocation));
						}
					}
				}
			}
		}
	}
}

static __inline void rankPlayersAndMoveThem()
{
	// calculating these things only makes sense when no one has ball. even though these usually should be mutually exclusive
	// theres a small change that refreshCatchAndchange == 1 and ball has been caught.
	// refreshCatchAndChange is set 1 when ball is thrown or bat or dropped but also when ball hits ground for the first time
	// and when it stops and also when player is changed. there could be more.
	if(stateInfo.localGameInfo->pRAI.refreshCatchAndChange == 1 && stateInfo.localGameInfo->pII.hasBallIndex == -1)
	{

		int i, j;
		int rankedIndices[RANKED_FIELDERS_COUNT];
		float rankedEvals[RANKED_FIELDERS_COUNT];
		float v = stateInfo.localGameInfo->ballInfo.velocity.y;
		float a = -GRAVITY;
		float s = 0.0f;
		float ballDropX; // where the ball will land
		float ballDropZ;
		float evalBallX; // place that we use to calculate player rankings
		float evalBallZ;
		float dirX; // ball's velocity's direction
		float dirZ;
		float time; // used to calculate ballDrop. framecount it takes from ball to land after movement's start ( like in throwin or batting )
		float dot;
		float speed;
		float distance;
		float evaluation;
		if(stateInfo.localGameInfo->ballInfo.hasHitGround == 0)
		{
			if(stateInfo.localGameInfo->pRAI.throwGoingToBase == -1)
			{
				s = 1.1f; // when batting the height is something like this
			}
			else
			{
				s = BALL_HEIGHT_WITH_PLAYER;
			}
		}
		else
		{
			s = 0;
		}
		// initialize rankedIndices and rankedEvals
		for(i = 0; i < RANKED_FIELDERS_COUNT; i++)
		{
			rankedIndices[i] = 0;
			rankedEvals[i] = 500.0f; // infinity
		}
		// calculate how long it takes for ball come back down from heights
		time = (float)(-v - sqrt(v*v - 2*a*s))/a;
		// ballDropX is the point where ball will approximately land after the air time
		ballDropX = stateInfo.localGameInfo->ballInfo.location.x + time*stateInfo.localGameInfo->ballInfo.velocity.x;
		ballDropZ = stateInfo.localGameInfo->ballInfo.location.z + time*stateInfo.localGameInfo->ballInfo.velocity.z;

		dirX = stateInfo.localGameInfo->ballInfo.velocity.x;
		dirZ = stateInfo.localGameInfo->ballInfo.velocity.z;
		speed = (float)sqrt(dirX*dirX + dirZ*dirZ);
		if(speed < EPSILON) speed = 0.1f;
		dirX = dirX/speed;
		dirZ = dirZ/speed;

		// here some approximations for situations when ball is still in air and when ball has hit the ground.
		// targetpoint is the place where we will move the catchers and what we will use for ranking the fielders.
		if(stateInfo.localGameInfo->ballInfo.hasHitGround == 0)
		{
			float finalPointXApprox = ballDropX + stateInfo.localGameInfo->ballInfo.velocity.x*BALL_FINAL_POINT_APPROXIMATION_CONSTANT;
			float finalPointZApprox = ballDropZ + stateInfo.localGameInfo->ballInfo.velocity.z*BALL_FINAL_POINT_APPROXIMATION_CONSTANT;
			float multiplier = speed/(time*0.01f);
			if(multiplier < 0.35f) multiplier = 0.0f;
			else if(multiplier > 0.6f) multiplier = 0.6f;
			else multiplier = (multiplier - 0.35f)*0.6f/0.25f;

			stateInfo.localGameInfo->gAI.targetPoint.x = (1-multiplier)*ballDropX + multiplier*finalPointXApprox;
			stateInfo.localGameInfo->gAI.targetPoint.z = (1-multiplier)*ballDropZ + multiplier*finalPointZApprox;

			evalBallX = ballDropX + stateInfo.localGameInfo->ballInfo.velocity.x*BALL_DROP_EVAL_CONSTANT*0.25f/(time*0.01f);
			evalBallZ = ballDropZ + stateInfo.localGameInfo->ballInfo.velocity.z*BALL_DROP_EVAL_CONSTANT*0.25f/(time*0.01f);
		}
		else if(stateInfo.localGameInfo->ballInfo.hasHitGround == 1 && stateInfo.localGameInfo->ballInfo.onGround == 0)
		{
			stateInfo.localGameInfo->gAI.targetPoint.x = ballDropX + stateInfo.localGameInfo->ballInfo.velocity.x*BALL_DROP_TO_FINAL_POINT_APPROXIMATION_CONSTANT;
			stateInfo.localGameInfo->gAI.targetPoint.z = ballDropZ + stateInfo.localGameInfo->ballInfo.velocity.z*BALL_DROP_TO_FINAL_POINT_APPROXIMATION_CONSTANT;

			evalBallX = stateInfo.localGameInfo->gAI.targetPoint.x;
			evalBallZ = stateInfo.localGameInfo->gAI.targetPoint.z;
		}
		else
		{
			stateInfo.localGameInfo->gAI.targetPoint.x = ballDropX + stateInfo.localGameInfo->ballInfo.velocity.x*BALL_FINAL_POINT_APPROXIMATION_CONSTANT;
			stateInfo.localGameInfo->gAI.targetPoint.z = ballDropZ + stateInfo.localGameInfo->ballInfo.velocity.z*BALL_FINAL_POINT_APPROXIMATION_CONSTANT;

			evalBallX = stateInfo.localGameInfo->gAI.targetPoint.x;
			evalBallZ = stateInfo.localGameInfo->gAI.targetPoint.z;
		}

		// check only catching team players
		for(i = PLAYERS_IN_TEAM + JOKER_COUNT; i < PLAYERS_IN_TEAM * 2 + JOKER_COUNT; i++)
		{
			float dx; // vector from ball drop to player
			float dz;
			float distance2;
			dx = stateInfo.localGameInfo->playerInfo[i].tPI.location.x - evalBallX;
			dz = stateInfo.localGameInfo->playerInfo[i].tPI.location.z - evalBallZ;
			distance = (float)sqrt(dx*dx + dz*dz);
			if( distance < EPSILON) distance = 1.0f;
			dx = stateInfo.localGameInfo->playerInfo[i].tPI.location.x - stateInfo.localGameInfo->ballInfo.location.x;
			dz = stateInfo.localGameInfo->playerInfo[i].tPI.location.z - stateInfo.localGameInfo->ballInfo.location.z;
			distance2 = (float)sqrt(dx*dx + dz*dz);
			if( distance2 < EPSILON) distance2 = 1.0f;
			dx = dx / distance2;
			dz = dz / distance2;
			dot = (dx*dirX + dz*dirZ)*(dx*dirX + dz*dirZ);
			if(stateInfo.localGameInfo->ballInfo.hasHitGround == 0)
			{
				evaluation = distance - dot*EVALUATION_CONSTANT_IN_AIR*speed*speed;
			}
			else if(stateInfo.localGameInfo->ballInfo.hasHitGround == 1 && stateInfo.localGameInfo->ballInfo.onGround == 0)
			{
				evaluation = distance - dot*EVALUATION_CONSTANT_AFTER_HIT_ONCE;
			}
			else
			{
				evaluation = distance;
			}
			// and then we create a array of 5 of these players ranked on their evaluation number
			for(j = 0; j < RANKED_FIELDERS_COUNT; j++)
			{
				if(evaluation < rankedEvals[j])
				{
					int k;
					for(k = 4; k > j; k--)
					{
						rankedEvals[k] = rankedEvals[k-1];
						rankedIndices[k] = rankedIndices[k-1];
					}
					rankedEvals[j] = evaluation;
					rankedIndices[j] = i;
					break;
				}
			}
		}
		// so now we have ranked indices -array
		for(j = 0; j < RANKED_FIELDERS_COUNT; j++)
		{
			stateInfo.localGameInfo->pII.fielderRankedIndices[j] = rankedIndices[j];
		}


		// then change cursor to best choice of players if flags says so
		// set for example when ball is first bat/thrown etc or when change player key is pressed.
		if(stateInfo.localGameInfo->pRAI.initPlayerSelection == 1)
		{
			stateInfo.localGameInfo->pII.changePlayerArrayIndex = 0;
			changePlayer();
			stateInfo.localGameInfo->pRAI.initPlayerSelection = 0;
		}
		else
		{
			stateInfo.localGameInfo->pII.changePlayerArrayIndex = -1;
		}

		// move these critical players to ball's final position
		moveRankedToCatch();


	}
	stateInfo.localGameInfo->pRAI.refreshCatchAndChange = 0;

}

static __inline void playerLocationOrientationAndTargets()
{
	int i;

	for(i = 0; i < PLAYERS_IN_TEAM * 2 + JOKER_COUNT; i++)
	{
		// orientation updated only if ball or player moving
		if(stateInfo.localGameInfo->ballInfo.moving == 1 ||
			stateInfo.localGameInfo->playerInfo[i].cPI.moving == 1)
		{
			if(i != stateInfo.localGameInfo->pII.hasBallIndex)
			{
				if(stateInfo.localGameInfo->playerInfo[i].cPI.running == 0)
				{
					if(i != stateInfo.localGameInfo->pII.batterIndex)
					{
						setOrientation(i);
					}
				}
			}
		}
	}
	// move players
	for(i = 0; i < PLAYERS_IN_TEAM * 2 + JOKER_COUNT; i++)
	{
		// players only moved if moving-flag is set to 1.
		if(stateInfo.localGameInfo->playerInfo[i].cPI.moving == 1)
		{
			setVectorXZ(&stateInfo.localGameInfo->playerInfo[i].tPI.lastLocation, stateInfo.localGameInfo->playerInfo[i].tPI.location.x,
				stateInfo.localGameInfo->playerInfo[i].tPI.location.z);
			// if we are pressing against the fences, fence wont give up. we need to tell that to ball also.
			// this can happen only for fielders.
			if( i >= PLAYERS_IN_TEAM + JOKER_COUNT)
			{
				if((stateInfo.localGameInfo->playerInfo[i].tPI.location.z > FIELD_FRONT - FENCE_OFFSET && stateInfo.localGameInfo->playerInfo[i].tPI.velocity.z > 0) ||
					(stateInfo.localGameInfo->playerInfo[i].tPI.location.z < FIELD_BACK + FENCE_OFFSET && stateInfo.localGameInfo->playerInfo[i].tPI.velocity.z < 0))
				{
					stateInfo.localGameInfo->ballInfo.needsMoveUpdate = 1;
					stateInfo.localGameInfo->playerInfo[i].tPI.velocity.z = 0.0f;
				}
				if((stateInfo.localGameInfo->playerInfo[i].tPI.location.x > FIELD_RIGHT - FENCE_OFFSET && stateInfo.localGameInfo->playerInfo[i].tPI.velocity.x > 0) ||
					(stateInfo.localGameInfo->playerInfo[i].tPI.location.x < FIELD_LEFT + FENCE_OFFSET && stateInfo.localGameInfo->playerInfo[i].tPI.velocity.x < 0))
				{
					stateInfo.localGameInfo->playerInfo[i].tPI.velocity.x = 0.0f;
					stateInfo.localGameInfo->ballInfo.needsMoveUpdate = 1;
				}
			}
			// update the location by player's current velocity.
			addToVectorXZ(&stateInfo.localGameInfo->playerInfo[i].tPI.location, stateInfo.localGameInfo->playerInfo[i].tPI.velocity.x,
				stateInfo.localGameInfo->playerInfo[i].tPI.velocity.z);
			// a lot of movement in the game is handled by these se called targets. we set target location and looksToTarget flag
			// for a player and he will move to that direction with constant velocity until he reaches the target.
			// reaching is decided here.
			// but first step to optimize this process is to have this looksForTarget flag so that we can distinct between
			// user moved players and automatically moved players.
			if(stateInfo.localGameInfo->playerInfo[i].cPI.looksForTarget == 1)
			{
				// so here we now check if we happened to be close enough to our target so that we could wrap this
				// journey up.
				float dx = stateInfo.localGameInfo->playerInfo[i].tPI.location.x - stateInfo.localGameInfo->playerInfo[i].tPI.targetLocation.x;
				float dz = stateInfo.localGameInfo->playerInfo[i].tPI.location.z - stateInfo.localGameInfo->playerInfo[i].tPI.targetLocation.z;
				if(isVectorSmallEnoughCircleXZ(dx, dz, TARGET_ACHIEVED_THRESHOLD) == 1)
				{
					int needToStop = 1;
					if(	i == stateInfo.localGameInfo->pII.batterIndex &&
						stateInfo.localGameInfo->playerInfo[i].bTPI.goingForward == 0)
					{
						prepareBatter();
					}
					else
					{
						// select correct model for catchers.
						if(i >= PLAYERS_IN_TEAM + JOKER_COUNT)
						{
							stateInfo.localGameInfo->playerInfo[i].cPI.model = 0;
						}
						else
						{
							stateInfo.localGameInfo->playerInfo[i].cPI.model = 10;
						}
					}
					// if no out is made of this player ( batting team player )
					if(i < PLAYERS_IN_TEAM + JOKER_COUNT)
					{
						// so if we are moving out of the field after being wounded or tagged.
						// here we first move to a some point outside the limits of game area
						// and then back to a point in homebase, and then to own place on the circle.
						// check for running because otherwise we players who were wounded before arriving a base
						// would be handled by this, which is of course a mistake.
						if(stateInfo.localGameInfo->playerInfo[i].bTPI.out == 1 ||
							(stateInfo.localGameInfo->playerInfo[i].bTPI.wounded == 1 &&
							stateInfo.localGameInfo->playerInfo[i].cPI.running == 0))
						{
							Vector3D target;

							if(stateInfo.localGameInfo->playerInfo[i].bTPI.passedPathPoint == 0)
							{
								stateInfo.localGameInfo->playerInfo[i].bTPI.passedPathPoint = 1;
								target.x = stateInfo.fieldPositions->pitchPlate.x;
								target.z = stateInfo.fieldPositions->pitchPlate.z + HOME_RADIUS;
							}
							else if(stateInfo.localGameInfo->playerInfo[i].bTPI.passedPathPoint == 1)
							{
								stateInfo.localGameInfo->playerInfo[i].bTPI.passedPathPoint = 2;
								target.x = stateInfo.localGameInfo->playerInfo[i].tPI.homeLocation.x;
								target.z = stateInfo.localGameInfo->playerInfo[i].tPI.homeLocation.z;
							}
							else
							{
								stopTargetLookingPlayer(i);
								setOrientation(i);
								continue;
							}
							// and move to target
							moveToTarget(i, &target);
							continue;
						}
						// if we are legally running bases
						else
						{
							// if we are at homebase, first base or second base and not leading
							if(stateInfo.localGameInfo->playerInfo[i].bTPI.base >= 0 &&
								stateInfo.localGameInfo->playerInfo[i].bTPI.base < 3 &&
								stateInfo.localGameInfo->playerInfo[i].bTPI.leading == 0)
							{
								// and moving forward
								if(stateInfo.localGameInfo->playerInfo[i].bTPI.goingForward == 1 )
								{
									// set going forward flag to 0 as we are stopped now
									stateInfo.localGameInfo->playerInfo[i].bTPI.goingForward = 0;
									// set new base-value and isOnBase-vaule to 1.
									stateInfo.localGameInfo->playerInfo[i].bTPI.base =
										stateInfo.localGameInfo->playerInfo[i].bTPI.base + 1;
									stateInfo.localGameInfo->playerInfo[i].bTPI.isOnBase = 1;
									// also these are needed to do continued calculations only when needed.
									stateInfo.localGameInfo->gAI.playerArrivedToBase = 1;
									stateInfo.localGameInfo->playerInfo[i].bTPI.arrivedToBase = 1;

								}
								else
								{
									stateInfo.localGameInfo->playerInfo[i].bTPI.isOnBase = 1;
								}
							}
							// so if we are on base 3
							else if(stateInfo.localGameInfo->playerInfo[i].bTPI.base == 3)
							{
								if(stateInfo.localGameInfo->playerInfo[i].bTPI.goingForward == 1)
								{
									// if we are moving forward and have not passed the flag yet, change the direction
									if(stateInfo.localGameInfo->playerInfo[i].bTPI.passedPathPoint == 0)
									{
										stateInfo.localGameInfo->playerInfo[i].bTPI.passedPathPoint = 1;
										runToNextBase(i, 3);
										needToStop = 0;
									}
									// if have passed the flag we are at homebase, so set the base to 4
									// and move player to its circle.
									else if(stateInfo.localGameInfo->playerInfo[i].bTPI.passedPathPoint == 1)
									{
										Vector3D target;
										stateInfo.localGameInfo->playerInfo[i].bTPI.passedPathPoint = 2;

										stateInfo.localGameInfo->playerInfo[i].bTPI.base = 4;
										stateInfo.localGameInfo->playerInfo[i].bTPI.isOnBase = 1;
										stateInfo.localGameInfo->playerInfo[i].bTPI.goingForward = 0;
										stateInfo.localGameInfo->gAI.playerArrivedToBase = 1;
										stateInfo.localGameInfo->playerInfo[i].bTPI.arrivedToBase = 1;
										target.x = stateInfo.localGameInfo->playerInfo[i].tPI.homeLocation.x;
										target.z = stateInfo.localGameInfo->playerInfo[i].tPI.homeLocation.z;
										moveToTarget(i, &target);
										needToStop = 0;
									}
								}
								else
								{
									// if we are coming back and have passed the point, change direction
									if(stateInfo.localGameInfo->playerInfo[i].bTPI.passedPathPoint == 1)
									{
										stateInfo.localGameInfo->playerInfo[i].bTPI.passedPathPoint = 0;
										runToPreviousBase(i, 3);
										needToStop = 0;
									}
									else
									{
										// otherwise we are at the third base

										stateInfo.localGameInfo->playerInfo[i].bTPI.isOnBase = 1;
										stopTargetLookingPlayer(i);
										needToStop = 0;
									}
								}
							}
						}
					}
					// if procedures higher didnt handle stopping themselves
					// we stop the player here.
					if(needToStop == 1)
					{
						stopTargetLookingPlayer(i);
						setOrientation(i);
					}
				}
			}
		}
		// if we dont move, it could be that we just stopped, so lets see if we should update our lastLocation.
		if(stateInfo.localGameInfo->playerInfo[i].cPI.lastLastLocationUpdate == 1)
		{
				setVectorXZ(&stateInfo.localGameInfo->playerInfo[i].tPI.lastLocation, stateInfo.localGameInfo->playerInfo[i].tPI.location.x,
					stateInfo.localGameInfo->playerInfo[i].tPI.location.z);
				stateInfo.localGameInfo->playerInfo[i].cPI.lastLastLocationUpdate = 0;
		}
	}
}
// so for animations we need to change mesh after certain intervals.
static __inline void updateModels()
{
	int i;
	// every player has some animations.
	for(i = 0; i < PLAYERS_IN_TEAM*2 + JOKER_COUNT; i++)
	{
		switch(stateInfo.localGameInfo->playerInfo[i].cPI.model) {
		case 2:
		case 3:
		case 4:
		case 5:
			// so first animations for walking and running. they are loops, the stage will be in range of 0 to count*freq.
			// reason this seemingly complicated way of doing this is just to provide possibility to have animations
			// with same meshcounts to have animations of different speeds.
			stateInfo.localGameInfo->playerInfo[i].cPI.animationStage = (stateInfo.localGameInfo->playerInfo[i].cPI.animationStage + 1) %
				(stateInfo.localGameInfo->playerInfo[i].cPI.animationFrequency*stateInfo.localGameInfo->playerInfo[i].cPI.animationStageCount);
			break;
		case 6:
		case 7:
		case 8:
			// throwing animations are not loops and they must end when the last stage is encountered.
			if(stateInfo.localGameInfo->playerInfo[i].cPI.animationStage != stateInfo.localGameInfo->playerInfo[i].cPI.animationStageCount*stateInfo.localGameInfo->playerInfo[i].cPI.animationFrequency - 1)
			{
				stateInfo.localGameInfo->playerInfo[i].cPI.animationStage = (stateInfo.localGameInfo->playerInfo[i].cPI.animationStage + 1);
			}
			break;
		case 9:
			// here we do the same thing but set recoil to 0 after animation so that player can start moving and stuff again.
			if(stateInfo.localGameInfo->playerInfo[i].cPI.animationStage != stateInfo.localGameInfo->playerInfo[i].cPI.animationStageCount*stateInfo.localGameInfo->playerInfo[i].cPI.animationFrequency - 1)
			{
				stateInfo.localGameInfo->playerInfo[i].cPI.animationStage = (stateInfo.localGameInfo->playerInfo[i].cPI.animationStage + 1);
			}
			else
			{
				stateInfo.localGameInfo->playerInfo[i].cTPI.throwRecoil = 0;
				stateInfo.localGameInfo->playerInfo[i].cPI.model = 0;
			}
			break;
		case 11:
		case 12:
			// running and walking animations for batting team
			stateInfo.localGameInfo->playerInfo[i].cPI.animationStage = (stateInfo.localGameInfo->playerInfo[i].cPI.animationStage + 1) %
				(stateInfo.localGameInfo->playerInfo[i].cPI.animationFrequency*stateInfo.localGameInfo->playerInfo[i].cPI.animationStageCount);
			break;
		case 14:
		case 15:
		case 16:
			// batting animations are not loops, so theyll end after finishing. swinging, bunting, and spreading hands.
			if(stateInfo.localGameInfo->playerInfo[i].cPI.animationStage != stateInfo.localGameInfo->playerInfo[i].cPI.animationStageCount*stateInfo.localGameInfo->playerInfo[i].cPI.animationFrequency - 1)
			{
				stateInfo.localGameInfo->playerInfo[i].cPI.animationStage = (stateInfo.localGameInfo->playerInfo[i].cPI.animationStage + 1);
			}
			break;

		}
	}

}