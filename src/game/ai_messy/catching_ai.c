#include "ai_messy/catching_ai.h"
#include "action_implementation.h"
#include "actions_messy/throwing_system.h"
#include "actions_messy/pitching_system.h"
#include "common_logic.h"
#include "vector_math.h"
#include "catching_ai_strategy.h"
#include "rng.h"
#include "base_logic.h"

void initCatchingAI(AIState* aiState)
{
	aiState->dropStage = 0;
	aiState->throwStage = 0;
	aiState->moveCounter = 0;
}

// we move towards the target position by simulating movement intent directly.
void moveControlledPlayerToLocation(StateInfo* stateInfo, Vector3D* target)
{
	float px = stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].tPI.location.x;
	float pz = stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].tPI.location.z;
	float tx = target->x;
	float tz = target->z;
	float dx = tx - px;
	float dz = tz - pz;

	if(!isVectorSmallEnoughCircleXZ(dx, dz, 1.0f) && stateInfo->localGameInfo->pendingActionState.throwGoingOn == 0) {
		if(stateInfo->localGameInfo->aiState.moveCounter >= 10) {
			MovementKeys keys = calculate_movement_keys(dx, dz);
			// Set triggers only if NOT already moving in that direction to avoid unnecessary resets
			if (keys.up) {
				if (stateInfo->localGameInfo->aF.cTAF.move[0] == ACTION_IDLE)
					stateInfo->localGameInfo->aF.cTAF.move[0] = ACTION_TRIGGER_START;
			} else {
				if (stateInfo->localGameInfo->aF.cTAF.move[0] != ACTION_IDLE)
					stateInfo->localGameInfo->aF.cTAF.move[0] = ACTION_TRIGGER_STOP;
			}

			if (keys.right) {
				if (stateInfo->localGameInfo->aF.cTAF.move[1] == ACTION_IDLE)
					stateInfo->localGameInfo->aF.cTAF.move[1] = ACTION_TRIGGER_START;
			} else {
				if (stateInfo->localGameInfo->aF.cTAF.move[1] != ACTION_IDLE)
					stateInfo->localGameInfo->aF.cTAF.move[1] = ACTION_TRIGGER_STOP;
			}

			if (keys.down) {
				if (stateInfo->localGameInfo->aF.cTAF.move[2] == ACTION_IDLE)
					stateInfo->localGameInfo->aF.cTAF.move[2] = ACTION_TRIGGER_START;
			} else {
				if (stateInfo->localGameInfo->aF.cTAF.move[2] != ACTION_IDLE)
					stateInfo->localGameInfo->aF.cTAF.move[2] = ACTION_TRIGGER_STOP;
			}

			if (keys.left) {
				if (stateInfo->localGameInfo->aF.cTAF.move[3] == ACTION_IDLE)
					stateInfo->localGameInfo->aF.cTAF.move[3] = ACTION_TRIGGER_START;
			} else {
				if (stateInfo->localGameInfo->aF.cTAF.move[3] != ACTION_IDLE)
					stateInfo->localGameInfo->aF.cTAF.move[3] = ACTION_TRIGGER_STOP;
			}

			stateInfo->localGameInfo->aiState.moveCounter = 0;
		}
	} else {
		int i;
		for(i=0; i<4; i++) {
			if(stateInfo->localGameInfo->aF.cTAF.move[i] != ACTION_IDLE) {
				stateInfo->localGameInfo->aF.cTAF.move[i] = ACTION_TRIGGER_STOP;
			}
		}
		stateInfo->localGameInfo->aiState.moveCounter = 0;
	}
	stateInfo->localGameInfo->aiState.moveCounter++;
}

void throwBallToBase(StateInfo* stateInfo, BaseID base)
{
	if(stateInfo->localGameInfo->aiState.throwStage == 0) {
		if(stateInfo->localGameInfo->pendingActionState.aiActionEventLock == AI_NO_LOCK && stateInfo->localGameInfo->pendingActionState.aiLockUpdate == 0) {
			int catcherIndex = stateInfo->localGameInfo->pII.catcherOnBaseIndex[base];
			int catcherNearHome = 0;
			if(catcherIndex != -1) {
				catcherNearHome = stateInfo->localGameInfo->playerInfo[catcherIndex].cTPI.isNearHomeLocation;
			}

			int replacerIndex = stateInfo->localGameInfo->pII.catcherReplacerOnBaseIndex[base];
			ReplacementState replacerStage = REPLACEMENT_IDLE;
			int replacerBase = -1;
			int replacerMoving = 0;
			if(replacerIndex != -1) {
				replacerStage = stateInfo->localGameInfo->playerInfo[replacerIndex].cTPI.replacingStage;
				replacerBase = stateInfo->localGameInfo->playerInfo[replacerIndex].cTPI.replacingBase;
				replacerMoving = stateInfo->localGameInfo->playerInfo[replacerIndex].cPI.moving;
			}

			int shouldThrow = should_ai_throw(&(stateInfo->localGameInfo->pII), catcherNearHome,
			                                  replacerIndex, replacerStage, replacerBase, replacerMoving,
			                                  base);

			if(shouldThrow == 1) {
				stateInfo->localGameInfo->aiState.throwStage = 1;
				stateInfo->localGameInfo->pendingActionState.aiLockUpdate = 1;
				stateInfo->localGameInfo->pendingActionState.aiActionEventLock = AI_THROW_LOCK;

				// AI sets trigger directly
				stateInfo->localGameInfo->aF.cTAF.throwToBase[base] = ACTION_TRIGGER_START;
			}
		}
	}
}

void updateCatchingAI(StateInfo* stateInfo, unsigned int* rng_seed)
{
	// Update AI pitching
	updateAIPitching(stateInfo, rng_seed);

	// finish dropping
	if(stateInfo->localGameInfo->aiState.dropStage == 1) {
		stateInfo->localGameInfo->pendingActionState.aiActionEventLock = AI_NO_LOCK;
		stateInfo->localGameInfo->aiState.dropStage = 0;
		stateInfo->localGameInfo->pendingActionState.aiLockUpdate = 1;
	}
	// finish throwing
	if(stateInfo->localGameInfo->aiState.throwStage == 1) {
		if(stateInfo->localGameInfo->pendingActionState.aiLockTimeoutCounter == -1) {
			stateInfo->localGameInfo->pendingActionState.aiLockTimeoutCounter = 0;
		}
		if(stateInfo->localGameInfo->pendingActionState.meterCounter > THROW_MAX*(3.0f/4)) {
			stateInfo->localGameInfo->aiState.throwStage = 0;
			stateInfo->localGameInfo->pendingActionState.aiActionEventLock = AI_NO_LOCK;
			stateInfo->localGameInfo->pendingActionState.aiLockUpdate = 1;
			stateInfo->localGameInfo->pendingActionState.aiLockTimeoutCounter = -1;

			// AI sets trigger stop directly
			int i;
			for(i=0; i<4; i++) {
				if(stateInfo->localGameInfo->aF.cTAF.throwToBase[i] != ACTION_IDLE) {
					stateInfo->localGameInfo->aF.cTAF.throwToBase[i] = ACTION_TRIGGER_STOP;
				}
			}
		} else {
			stateInfo->localGameInfo->pendingActionState.aiLockTimeoutCounter++;
			if(stateInfo->localGameInfo->pendingActionState.aiLockTimeoutCounter > TIMEOUT_CONSTANT) {
				stateInfo->localGameInfo->aiState.throwStage = 0;
				stateInfo->localGameInfo->pendingActionState.aiActionEventLock = AI_NO_LOCK;
				stateInfo->localGameInfo->pendingActionState.aiLockUpdate = 1;
				stateInfo->localGameInfo->pendingActionState.aiLockTimeoutCounter = -1;

				int i;
				for(i=0; i<4; i++) {
					stateInfo->localGameInfo->aF.cTAF.throwToBase[i] = ACTION_IDLE;
				}
			}
		}
	}
	// if noone has ball and someone is controlled, ai will try to move towards the target point calculated
	// in game_manipulation.
	if(stateInfo->localGameInfo->pII.hasBallIndex == -1 && stateInfo->localGameInfo->pII.controlIndex != -1) {
		if(stateInfo->localGameInfo->pendingActionState.aiActionEventLock == AI_NO_LOCK && stateInfo->localGameInfo->pendingActionState.aiLockUpdate == 0) {
			if(stateInfo->localGameInfo->pRAI.throwGoingToBase == -1 || stateInfo->localGameInfo->
			        ballInfo.hasHitGround == 1) {
				moveControlledPlayerToLocation(stateInfo, &(stateInfo->localGameInfo->cameraState.targetPoint));
			}
		}

	}
	// if someone has ball
	if(stateInfo->localGameInfo->pII.hasBallIndex != -1) {
		int index3 = stateInfo->localGameInfo->pII.baseControlIndex[3];
		int index2 = stateInfo->localGameInfo->pII.baseControlIndex[2];

		BaseID r3BaseAtPitchStart = BASE_NONE;
		int r3IsOnBase = 0;
		if (index3 != -1) {
			r3BaseAtPitchStart = stateInfo->localGameInfo->referee.battingPlayers[index3].baseAtPitchStart;
			r3IsOnBase = (stateInfo->localGameInfo->playerInfo[index3].bTPI.state == PLAYER_STATE_SAFE_ON_BASE);
		}

		BaseID r2BaseAtPitchStart = BASE_NONE;
		int r2IsOnBase = 0;
		if (index2 != -1) {
			r2BaseAtPitchStart = stateInfo->localGameInfo->referee.battingPlayers[index2].baseAtPitchStart;
			r2IsOnBase = (stateInfo->localGameInfo->playerInfo[index2].bTPI.state == PLAYER_STATE_SAFE_ON_BASE);
		}

		int catcherHomeIndex = stateInfo->localGameInfo->pII.catcherOnBaseIndex[0];
		int hasBallIndex = stateInfo->localGameInfo->pII.hasBallIndex;

		if(should_ai_drop_ball(&(stateInfo->localGameInfo->woundingState),
		                       &(stateInfo->localGameInfo->gameControl),
		                       r3BaseAtPitchStart, r3IsOnBase,
		                       r2BaseAtPitchStart, r2IsOnBase,
		                       catcherHomeIndex, hasBallIndex)) {
			if(stateInfo->localGameInfo->pendingActionState.aiActionEventLock == AI_NO_LOCK && stateInfo->localGameInfo->pendingActionState.aiLockUpdate == 0) {
				stateInfo->localGameInfo->aiState.dropStage = 1;
				stateInfo->localGameInfo->pendingActionState.aiLockUpdate = 1;
				stateInfo->localGameInfo->pendingActionState.aiActionEventLock = AI_DROP_LOCK;
				stateInfo->localGameInfo->aF.cTAF.dropBall = ACTION_TRIGGER_START;
			}
		}
		// otherwise we throw or move towards a base where lead player is going. if lead player is going nowhere
		// we take ball to home base.
		else {
			BaseID leadBase = BASE_NONE;
			int throwBase = 0;

			CatchingRunnerInfo runners[PLAYERS_IN_TEAM + JOKER_COUNT];
			int runnerCount = 0;
			int i;

			for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
				PlayerUnitState s = stateInfo->localGameInfo->playerInfo[i].bTPI.state;
				BaseID bid = stateInfo->localGameInfo->playerInfo[i].bTPI.baseId;

				if (bid != BASE_NONE) {
					runners[runnerCount].isOnBase = (s == PLAYER_STATE_SAFE_ON_BASE || s == PLAYER_STATE_AT_BAT);
					runners[runnerCount].takingFreeWalk = (s == PLAYER_STATE_ADVANCING_FREELY);
					runners[runnerCount].base = bid;
					runners[runnerCount].leading = (s == PLAYER_STATE_LEADING);
					runnerCount++;
				}
			}

			int randomVal = seeded_rand(rng_seed, 500);
			leadBase = determine_lead_base(runners, runnerCount, randomVal);

			if(leadBase != BASE_NONE && base_cmp(leadBase, BASE_THIRD) < 0) throwBase = (int)base_get_next(leadBase);
			else throwBase = 0;

			if(stateInfo->localGameInfo->pendingActionState.aiActionEventLock == AI_NO_LOCK && stateInfo->localGameInfo->pendingActionState.aiLockUpdate == 0) {
				Vector3D target;
				target.x = stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->
				           pII.catcherOnBaseIndex[throwBase]].tPI.homeLocation.x;
				target.z = stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->
				           pII.catcherOnBaseIndex[throwBase]].tPI.homeLocation.z;
				moveControlledPlayerToLocation(stateInfo, &target);
			}
			throwBallToBase(stateInfo, (BaseID)throwBase);
		}
	}
	if(stateInfo->localGameInfo->pendingActionState.aiLockUpdate == 1) {
		stateInfo->localGameInfo->pendingActionState.aiLockUpdate = 0;
	}
}
