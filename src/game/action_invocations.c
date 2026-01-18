/*
	the main purpose of this code is to set flags for action_implementation when key combinations trigger some events.
	everything here is pretty straightforward.
*/

#include "globals.h"
#include "action_invocations.h"

static void checkThrow(StateInfo* stateInfo, int key, int actionKey, TeamControlMode control, BaseID base);
static void checkDrop(StateInfo* stateInfo, int key, TeamControlMode control);
static void checkMove(StateInfo* stateInfo, int key, TeamControlMode control, int direction);
static void checkChangePlayer(StateInfo* stateInfo, int key, TeamControlMode control);
static void checkPitch(StateInfo* stateInfo, int key, TeamControlMode control);
static void checkBatterSelection(StateInfo* stateInfo, int change, int select, TeamControlMode control);
static void checkFreeWalkDecision(StateInfo* stateInfo, int accept, int reject, TeamControlMode control);
static void checkBatterAngle(StateInfo* stateInfo, int increase, int decrease, TeamControlMode control);
static void checkSwing(StateInfo* stateInfo, int key, TeamControlMode control);
static void checkBattingTeamRun(StateInfo* stateInfo, int key, TeamControlMode control, BaseID base);

void initActionInvocations(StateInfo* stateInfo)
{
	// Placeholder for... future?
}

void actionInvocations(StateInfo* stateInfo)
{
	int battingTeamIndex = (stateInfo->match->scoreboard.
	                        inning+stateInfo->match->scoreboard.playsFirst+stateInfo->match->scoreboard.period)%2;
	TeamControlMode battingControl = stateInfo->match->scoreboard.teams[battingTeamIndex].control;
	TeamControlMode catchingControl = stateInfo->match->scoreboard.teams[(battingTeamIndex+1)%2].control;

	checkThrow(stateInfo, KEY_DOWN, KEY_2, catchingControl, BASE_HOME);
	checkThrow(stateInfo, KEY_LEFT, KEY_2, catchingControl, BASE_FIRST);
	checkThrow(stateInfo, KEY_RIGHT, KEY_2, catchingControl, BASE_SECOND);
	checkThrow(stateInfo, KEY_UP, KEY_2, catchingControl, BASE_THIRD);

	if(stateInfo->match->pII.hasBallIndex == -1) {
		checkChangePlayer(stateInfo, KEY_2, catchingControl);
	} else if(stateInfo->match->pII.controlIndex !=
	          stateInfo->match->pII.catcherOnBaseIndex[0]) {
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
	if(stateInfo->match->flowControl.waitingForFreeWalkDecision == 1) {
		checkFreeWalkDecision(stateInfo, KEY_2, KEY_1, battingControl);
	} else if(stateInfo->match->flowControl.waitingForBatterDecision == 1) {
		checkBatterSelection(stateInfo, KEY_1, KEY_2, battingControl);
	}
	checkBatterAngle(stateInfo, KEY_PLUS, KEY_MINUS, battingControl);
	checkSwing(stateInfo, KEY_2, battingControl);

	checkBattingTeamRun(stateInfo, KEY_DOWN, battingControl, BASE_HOME);
	checkBattingTeamRun(stateInfo, KEY_LEFT, battingControl, BASE_FIRST);
	checkBattingTeamRun(stateInfo, KEY_RIGHT, battingControl, BASE_SECOND);
	checkBattingTeamRun(stateInfo, KEY_UP, battingControl, BASE_THIRD);

}

static void checkThrow(StateInfo* stateInfo, int key, int actionKey, TeamControlMode control, BaseID base)
{
	if(control != CONTROL_AI) {
		if(stateInfo->keyStates->down[control][key] &&stateInfo->keyStates->down[control][actionKey]) {
			if(stateInfo->match->aF.cTAF.throwToBase[base] == ACTION_IDLE) {
				stateInfo->match->aF.cTAF.throwToBase[base] = ACTION_TRIGGER_START;
				// prevent change player event or drop event from happening when we are throwing
				stateInfo->match->aF.cTAF.actionKeyLock = 1;
			}
		} else if((stateInfo->keyStates)->released[control][actionKey] &&(stateInfo->match->aF.cTAF.throwToBase[base] == ACTION_ACTIVE ||
		          stateInfo->match->aF.cTAF.throwToBase[base] == ACTION_TRIGGER_START)) {
			stateInfo->match->aF.cTAF.throwToBase[base] = ACTION_TRIGGER_STOP;
		}
	} else {
		// AI sets flags directly in AI logic files
	}
}

static void checkMove(StateInfo* stateInfo, int key, TeamControlMode control, int direction)
{
	if(control != CONTROL_AI) {
		if(stateInfo->keyStates->down[control][key] == 1 &&stateInfo->keyStates->down[control][KEY_2] == 0) {
			if(stateInfo->match->aF.cTAF.move[direction] == ACTION_IDLE) {
				stateInfo->match->aF.cTAF.move[direction] = ACTION_TRIGGER_START;

			}
		} else if(stateInfo->keyStates->released[control][key] == 1 || (stateInfo->keyStates->down[control][key] == 1 &&stateInfo->keyStates->down[control][KEY_2] == 1)) {
			if(stateInfo->match->aF.cTAF.move[direction] != ACTION_IDLE) { // to avoid something weird when this is changed to 1 when ball is catched
				stateInfo->match->aF.cTAF.move[direction] = ACTION_TRIGGER_STOP;
			}
		}
	} else {
		// AI sets flags directly
	}
}

static void checkChangePlayer(StateInfo* stateInfo, int key, TeamControlMode control)
{
	if(control != CONTROL_AI) {
		if(stateInfo->match->aF.cTAF.actionKeyLock == 0) {
			if(stateInfo->keyStates->released[control][key] == 1) {
				if(stateInfo->match->aF.cTAF.changePlayer == ACTION_IDLE) {
					stateInfo->match->aF.cTAF.changePlayer = ACTION_TRIGGER_START;
					stateInfo->match->aF.cTAF.actionKeyLock = 1;
				}
			}
		}
	} else {
		// AI sets flags directly
	}
}

static void checkDrop(StateInfo* stateInfo, int key, TeamControlMode control)
{
	if(control != CONTROL_AI) {
		if(stateInfo->match->aF.cTAF.actionKeyLock == 0) {
			if(stateInfo->keyStates->released[control][key] == 1) {
				if(stateInfo->match->aF.cTAF.dropBall == ACTION_IDLE) {
					stateInfo->match->aF.cTAF.dropBall = ACTION_TRIGGER_START;
					stateInfo->match->aF.cTAF.actionKeyLock = 1;
				}
			}
		}
	} else {
		// AI sets flags directly
	}
}

static void checkPitch(StateInfo* stateInfo, int key, TeamControlMode control)
{
	if(control != CONTROL_AI) {
		if(stateInfo->keyStates->down[control][key] == 1) {
			if(stateInfo->match->aF.cTAF.pitch == PITCH_ACTION_IDLE) {
				if(stateInfo->match->aF.cTAF.actionKeyLock == 0) {
					stateInfo->match->aF.cTAF.pitch = PITCH_ACTION_START;
					stateInfo->match->aF.cTAF.actionKeyLock = 1;
				}
			} else if(stateInfo->match->aF.cTAF.pitch == PITCH_ACTION_ANGLE_WAIT) {
				stateInfo->match->aF.cTAF.pitch = PITCH_ACTION_ANGLE_SET;
			}
		} else if(stateInfo->keyStates->released[control][key] == 1) {
			if(stateInfo->match->aF.cTAF.pitch == PITCH_ACTION_POWER_WAIT) {
				stateInfo->match->aF.cTAF.pitch = PITCH_ACTION_POWER_SET;
			}
		}
	} else {
		// AI sets flags directly
	}
}

static void checkBatterSelection(StateInfo* stateInfo, int change, int select, TeamControlMode control)
{
	if(control != CONTROL_AI) {
		if(stateInfo->keyStates->released[control][change] == 1) {

			if(stateInfo->match->aF.bTAF.chooseBatter == CHOOSE_BATTER_IDLE) {
				stateInfo->match->aF.bTAF.chooseBatter = CHOOSE_BATTER_NEXT;
			}
		} else if(stateInfo->keyStates->released[control][select] == 1) {
			if(stateInfo->match->aF.bTAF.chooseBatter == CHOOSE_BATTER_IDLE) {
				stateInfo->match->aF.bTAF.chooseBatter = CHOOSE_BATTER_SELECT;
			}
		}
	} else {
		// AI sets flags directly
	}
}

static void checkFreeWalkDecision(StateInfo* stateInfo, int accept, int reject, TeamControlMode control)
{
	if(control != CONTROL_AI) {
		if(stateInfo->keyStates->released[control][accept] == 1) {

			if(stateInfo->match->aF.bTAF.takeFreeWalk == FREE_WALK_IDLE) {
				stateInfo->match->aF.bTAF.takeFreeWalk = FREE_WALK_ACCEPT;
			}
		} else if(stateInfo->keyStates->released[control][reject] == 1) {
			if(stateInfo->match->aF.bTAF.takeFreeWalk == FREE_WALK_IDLE) {
				stateInfo->match->aF.bTAF.takeFreeWalk = FREE_WALK_REJECT;
			}
		}
	} else {
		// AI sets flags directly
	}
}

static void checkBatterAngle(StateInfo* stateInfo, int increase, int decrease, TeamControlMode control)
{
	if(control != CONTROL_AI) {
		if(stateInfo->keyStates->down[control][increase] == 1) {
			if(stateInfo->match->pRAI.battingGoingOn == 1) {
				if(stateInfo->match->aF.bTAF.increaseBatterAngle == ACTION_IDLE) {
					stateInfo->match->aF.bTAF.increaseBatterAngle = ACTION_TRIGGER_START;
				}
			}
		} else if(stateInfo->keyStates->released[control][increase] == 1) {
			if(stateInfo->match->pRAI.battingGoingOn == 1) {
				if(stateInfo->match->aF.bTAF.increaseBatterAngle != ACTION_IDLE) {
					stateInfo->match->aF.bTAF.increaseBatterAngle = ACTION_TRIGGER_STOP;
				}
			}
		}
		if(stateInfo->keyStates->down[control][decrease] == 1) {
			if(stateInfo->match->pRAI.battingGoingOn == 1) {
				if(stateInfo->match->aF.bTAF.decreaseBatterAngle == ACTION_IDLE) {
					stateInfo->match->aF.bTAF.decreaseBatterAngle = ACTION_TRIGGER_START;
				}
			}
		} else if(stateInfo->keyStates->released[control][decrease] == 1) {
			if(stateInfo->match->pRAI.battingGoingOn == 1) {
				if(stateInfo->match->aF.bTAF.decreaseBatterAngle != ACTION_IDLE) {
					stateInfo->match->aF.bTAF.decreaseBatterAngle = ACTION_TRIGGER_STOP;
				}
			}
		}
	} else {
		// AI sets flags directly
	}
}

static void checkSwing(StateInfo* stateInfo, int key, TeamControlMode control)
{
	if(control != CONTROL_AI) {
		if(stateInfo->keyStates->down[control][key] == 1) {
			if(stateInfo->match->aF.bTAF.swing == BAT_ACTION_WAIT_FOR_BALL) {
				stateInfo->match->aF.bTAF.swing = BAT_ACTION_POWER_SET;
			}
		} else if(stateInfo->keyStates->released[control][key] == 1) {
			if(stateInfo->match->aF.bTAF.swing == BAT_ACTION_ANGLE_WAIT) {
				stateInfo->match->aF.bTAF.swing = BAT_ACTION_ANGLE_SET;
			}
		}
	} else {
		// AI sets flags directly
	}
}

static void checkBattingTeamRun(StateInfo* stateInfo, int key, TeamControlMode control, BaseID base)
{
	if(control != CONTROL_AI) {
		if((stateInfo->keyStates)->released[control][key]) {
			stateInfo->match->aF.bTAF.baseRun[base] = ACTION_TRIGGER_START;
		}
	} else {
		// AI sets flags directly in AI logic files
	}
}
