/*
	the main purpose of this code is to set flags for action_implementation when key combinations trigger some events.
	everything here is pretty straightforward.
*/

#include "globals.h"
#include "action_invocations.h"

static void checkThrow(StateInfo* stateInfo, int key, int actionKey, TeamControlMode control, int base);
static void checkDrop(StateInfo* stateInfo, int key, TeamControlMode control);
static void checkMove(StateInfo* stateInfo, int key, TeamControlMode control, int direction);
static void checkChangePlayer(StateInfo* stateInfo, int key, TeamControlMode control);
static void checkPitch(StateInfo* stateInfo, int key, TeamControlMode control);
static void checkBatterSelection(StateInfo* stateInfo, int change, int select, TeamControlMode control);
static void checkFreeWalkDecision(StateInfo* stateInfo, int accept, int reject, TeamControlMode control);
static void checkBatterAngle(StateInfo* stateInfo, int increase, int decrease, TeamControlMode control);
static void checkSwing(StateInfo* stateInfo, int key, TeamControlMode control);
static void checkBattingTeamRun(StateInfo* stateInfo, int key, TeamControlMode control, int base);

void initActionInvocations(StateInfo* stateInfo)
{
	// Placeholder for... future?
}

void actionInvocations(StateInfo* stateInfo)
{
	int battingTeamIndex = (stateInfo->globalGameInfo->
	                        inning+stateInfo->globalGameInfo->playsFirst+stateInfo->globalGameInfo->period)%2;
	TeamControlMode battingControl = stateInfo->globalGameInfo->teams[battingTeamIndex].control;
	TeamControlMode catchingControl = stateInfo->globalGameInfo->teams[(battingTeamIndex+1)%2].control;

	checkThrow(stateInfo, KEY_DOWN, KEY_2, catchingControl, 0);
	checkThrow(stateInfo, KEY_LEFT, KEY_2, catchingControl, 1);
	checkThrow(stateInfo, KEY_RIGHT, KEY_2, catchingControl, 2);
	checkThrow(stateInfo, KEY_UP, KEY_2, catchingControl, 3);

	if(stateInfo->localGameInfo->pII.hasBallIndex == -1) {
		checkChangePlayer(stateInfo, KEY_2, catchingControl);
	} else if(stateInfo->localGameInfo->pII.controlIndex !=
	          stateInfo->localGameInfo->pII.catcherOnBaseIndex[0]) {
		checkDrop(stateInfo, KEY_2, catchingControl);
	} else {
		checkPitch(stateInfo, KEY_2, catchingControl);
	}

	checkMove(stateInfo, KEY_UP, catchingControl, 0);
	checkMove(stateInfo, KEY_RIGHT, catchingControl, 1);
	checkMove(stateInfo, KEY_DOWN, catchingControl, 2);
	checkMove(stateInfo, KEY_LEFT, catchingControl, 3);

	// check these only if necessary. also if it happened to be so that
	// they are both asked the same time, choose the free walk first
	if(stateInfo->localGameInfo->gameControl.waitingForFreeWalkDecision == 1) {
		checkFreeWalkDecision(stateInfo, KEY_2, KEY_1, battingControl);
	} else if(stateInfo->localGameInfo->gameControl.waitingForBatterDecision == 1) {
		checkBatterSelection(stateInfo, KEY_1, KEY_2, battingControl);
	}
	checkBatterAngle(stateInfo, KEY_PLUS, KEY_MINUS, battingControl);
	checkSwing(stateInfo, KEY_2, battingControl);

	checkBattingTeamRun(stateInfo, KEY_DOWN, battingControl, 0);
	checkBattingTeamRun(stateInfo, KEY_LEFT, battingControl, 1);
	checkBattingTeamRun(stateInfo, KEY_RIGHT, battingControl, 2);
	checkBattingTeamRun(stateInfo, KEY_UP, battingControl, 3);

}

static void checkThrow(StateInfo* stateInfo, int key, int actionKey, TeamControlMode control, int base)
{
	if(stateInfo->keyStates->down[control][key] == 1 && stateInfo->keyStates->down[control][actionKey] == 1) {
		if(stateInfo->localGameInfo->aF.cTAF.throwToBase[base] == ACTION_IDLE) {
			stateInfo->localGameInfo->aF.cTAF.throwToBase[base] = ACTION_TRIGGER_START;
			stateInfo->localGameInfo->aF.cTAF.actionKeyLock = 1;
		}
	} else if(stateInfo->keyStates->released[control][actionKey] == 1) {
		if(stateInfo->localGameInfo->aF.cTAF.throwToBase[base] == ACTION_ACTIVE) {
			stateInfo->localGameInfo->aF.cTAF.throwToBase[base] = ACTION_TRIGGER_STOP;
		}
	}
}

static void checkMove(StateInfo* stateInfo, int key, TeamControlMode control, int direction)
{
	if(stateInfo->keyStates->down[control][key] == 1) {
		if(stateInfo->localGameInfo->aF.cTAF.move[direction] == ACTION_IDLE) {
			stateInfo->localGameInfo->aF.cTAF.move[direction] = ACTION_TRIGGER_START;

		}
	} else if(stateInfo->keyStates->released[control][key] == 1) {
		if(stateInfo->localGameInfo->aF.cTAF.move[direction] != ACTION_IDLE) { // to avoid something weird when this is changed to 1 when ball is catched
			stateInfo->localGameInfo->aF.cTAF.move[direction] = ACTION_TRIGGER_STOP;
		}
	}
}

static void checkChangePlayer(StateInfo* stateInfo, int key, TeamControlMode control)
{
	if(stateInfo->localGameInfo->aF.cTAF.actionKeyLock == 0) {
		if(stateInfo->keyStates->released[control][key] == 1) {
			if(stateInfo->localGameInfo->aF.cTAF.changePlayer == ACTION_IDLE) {
				stateInfo->localGameInfo->aF.cTAF.changePlayer = ACTION_TRIGGER_START;
				stateInfo->localGameInfo->aF.cTAF.actionKeyLock = 1;
			}
		}
	}
}

static void checkDrop(StateInfo* stateInfo, int key, TeamControlMode control)
{
	if(stateInfo->localGameInfo->aF.cTAF.actionKeyLock == 0) {
		if(stateInfo->keyStates->released[control][key] == 1) {
			if(stateInfo->localGameInfo->aF.cTAF.dropBall == ACTION_IDLE) {
				stateInfo->localGameInfo->aF.cTAF.dropBall = ACTION_TRIGGER_START;
				stateInfo->localGameInfo->aF.cTAF.actionKeyLock = 1;
			}
		}
	}
}

static void checkPitch(StateInfo* stateInfo, int key, TeamControlMode control)
{
	if(stateInfo->keyStates->down[control][key] == 1) {
		if(stateInfo->localGameInfo->aF.cTAF.pitch == PITCH_ACTION_IDLE) {
			if(stateInfo->localGameInfo->aF.cTAF.actionKeyLock == 0) {
				stateInfo->localGameInfo->aF.cTAF.pitch = PITCH_ACTION_START;
				stateInfo->localGameInfo->aF.cTAF.actionKeyLock = 1;
			}
		} else if(stateInfo->localGameInfo->aF.cTAF.pitch == PITCH_ACTION_ANGLE_WAIT) {
			stateInfo->localGameInfo->aF.cTAF.pitch = PITCH_ACTION_ANGLE_SET;
		}
	} else if(stateInfo->keyStates->released[control][key] == 1) {
		if(stateInfo->localGameInfo->aF.cTAF.pitch == PITCH_ACTION_POWER_WAIT) {
			stateInfo->localGameInfo->aF.cTAF.pitch = PITCH_ACTION_POWER_SET;
		}
	}
}

static void checkBatterSelection(StateInfo* stateInfo, int change, int select, TeamControlMode control)
{
	if(stateInfo->keyStates->released[control][change] == 1) {

		if(stateInfo->localGameInfo->aF.bTAF.chooseBatter == CHOOSE_BATTER_IDLE) {
			stateInfo->localGameInfo->aF.bTAF.chooseBatter = CHOOSE_BATTER_NEXT;
		}
	} else if(stateInfo->keyStates->released[control][select] == 1) {
		if(stateInfo->localGameInfo->aF.bTAF.chooseBatter == CHOOSE_BATTER_IDLE) {
			stateInfo->localGameInfo->aF.bTAF.chooseBatter = CHOOSE_BATTER_SELECT;
		}
	}
}

static void checkFreeWalkDecision(StateInfo* stateInfo, int accept, int reject, TeamControlMode control)
{
	if(stateInfo->keyStates->released[control][accept] == 1) {

		if(stateInfo->localGameInfo->aF.bTAF.takeFreeWalk == FREE_WALK_IDLE) {
			stateInfo->localGameInfo->aF.bTAF.takeFreeWalk = FREE_WALK_ACCEPT;
		}
	} else if(stateInfo->keyStates->released[control][reject] == 1) {
		if(stateInfo->localGameInfo->aF.bTAF.takeFreeWalk == FREE_WALK_IDLE) {
			stateInfo->localGameInfo->aF.bTAF.takeFreeWalk = FREE_WALK_REJECT;
		}
	}
}

static void checkBatterAngle(StateInfo* stateInfo, int increase, int decrease, TeamControlMode control)
{
	if(stateInfo->keyStates->down[control][increase] == 1) {
		if(stateInfo->localGameInfo->pRAI.battingGoingOn == 1) {
			if(stateInfo->localGameInfo->aF.bTAF.increaseBatterAngle == ACTION_IDLE) {
				stateInfo->localGameInfo->aF.bTAF.increaseBatterAngle = ACTION_TRIGGER_START;
			}
		}
	} else if(stateInfo->keyStates->released[control][increase] == 1) {
		if(stateInfo->localGameInfo->pRAI.battingGoingOn == 1) {
			if(stateInfo->localGameInfo->aF.bTAF.increaseBatterAngle != ACTION_IDLE) {
				stateInfo->localGameInfo->aF.bTAF.increaseBatterAngle = ACTION_TRIGGER_STOP;
			}
		}
	}
	if(stateInfo->keyStates->down[control][decrease] == 1) {
		if(stateInfo->localGameInfo->pRAI.battingGoingOn == 1) {
			if(stateInfo->localGameInfo->aF.bTAF.decreaseBatterAngle == ACTION_IDLE) {
				stateInfo->localGameInfo->aF.bTAF.decreaseBatterAngle = ACTION_TRIGGER_START;
			}
		}
	} else if(stateInfo->keyStates->released[control][decrease] == 1) {
		if(stateInfo->localGameInfo->pRAI.battingGoingOn == 1) {
			if(stateInfo->localGameInfo->aF.bTAF.decreaseBatterAngle != ACTION_IDLE) {
				stateInfo->localGameInfo->aF.bTAF.decreaseBatterAngle = ACTION_TRIGGER_STOP;
			}
		}
	}
}

static void checkSwing(StateInfo* stateInfo, int key, TeamControlMode control)
{
	if(stateInfo->keyStates->down[control][key] == 1) {
		if(stateInfo->localGameInfo->aF.bTAF.swing == BAT_ACTION_WAIT_FOR_BALL) {
			stateInfo->localGameInfo->aF.bTAF.swing = BAT_ACTION_POWER_SET;
		}
	} else if(stateInfo->keyStates->released[control][key] == 1) {
		if(stateInfo->localGameInfo->aF.bTAF.swing == BAT_ACTION_ANGLE_WAIT) {
			stateInfo->localGameInfo->aF.bTAF.swing = BAT_ACTION_ANGLE_SET;
		}
	}
}

static void checkBattingTeamRun(StateInfo* stateInfo, int key, TeamControlMode control, int base)
{
	if(stateInfo->keyStates->released[control][key] == 1) {
		if(stateInfo->localGameInfo->aF.bTAF.baseRun[base] == ACTION_IDLE) {
			stateInfo->localGameInfo->aF.bTAF.baseRun[base] = ACTION_TRIGGER_START;
		}
	}
}
