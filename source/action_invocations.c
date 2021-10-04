#include "globals.h"

#include "action_invocations.h"
#include "action_invocations_internal.h"

/*
	the main purpose of this code is to set flags for action_implementation when key combinations trigger some events.
	everything here is pretty straightforward.
*/

void initActionInvocations()
{

}

void actionInvocations()
{
	int battingTeamIndex = (stateInfo.globalGameInfo->
		inning+stateInfo.globalGameInfo->playsFirst+stateInfo.globalGameInfo->period)%2;
	int battingControl = stateInfo.globalGameInfo->teams[battingTeamIndex].control;
	int catchingControl = stateInfo.globalGameInfo->teams[(battingTeamIndex+1)%2].control;
	
	if(stateInfo.localGameInfo->gAI.initLocals > 0)
	{
		initActionInvocations();
		stateInfo.localGameInfo->gAI.initLocals++;
		if(stateInfo.localGameInfo->gAI.initLocals == INIT_LOCALS_COUNT)
		{
			stateInfo.localGameInfo->gAI.initLocals = 0;
		}
	}

	checkThrow(KEY_DOWN, KEY_2, catchingControl, 0);
	checkThrow(KEY_LEFT, KEY_2, catchingControl, 1);
	checkThrow(KEY_RIGHT, KEY_2, catchingControl, 2);
	checkThrow(KEY_UP, KEY_2, catchingControl, 3);

	if(stateInfo.localGameInfo->pII.hasBallIndex == -1)
	{
		checkChangePlayer(KEY_2, catchingControl);
	}
	else if(stateInfo.localGameInfo->pII.controlIndex != 
		stateInfo.localGameInfo->pII.catcherOnBaseIndex[0])
	{
		checkDrop(KEY_2, catchingControl);
	}
	else
	{
		checkPitch(KEY_2, catchingControl);
	}

	checkMove(KEY_UP, catchingControl, 0);
	checkMove(KEY_RIGHT, catchingControl, 1);
	checkMove(KEY_DOWN, catchingControl, 2);
	checkMove(KEY_LEFT, catchingControl, 3);

	// check these only if necessary. also if it happened to be so that
	// they are both asked the same time, choose the free walk first
	if(stateInfo.localGameInfo->gAI.waitingForFreeWalkDecision == 1)
	{
		checkFreeWalkDecision(KEY_2, KEY_1, battingControl);
	}
	else if(stateInfo.localGameInfo->gAI.waitingForBatterDecision == 1)
	{
		checkBatterSelection(KEY_1, KEY_2, battingControl);
	}
	checkBatterAngle(KEY_PLUS, KEY_MINUS, battingControl);
	checkSwing(KEY_2, battingControl);

	checkBattingTeamRun(KEY_DOWN, battingControl, 0);
	checkBattingTeamRun(KEY_LEFT, battingControl, 1);
	checkBattingTeamRun(KEY_RIGHT, battingControl, 2);
	checkBattingTeamRun(KEY_UP, battingControl, 3);
	
}

static __inline void checkThrow(int key, int actionKey, int control, int base)
{
	if(stateInfo.keyStates->down[control][key] == 1 && stateInfo.keyStates->down[control][actionKey] == 1)
	{
		if(stateInfo.localGameInfo->aF.cTAF.throwToBase[base] == 0)
		{
			stateInfo.localGameInfo->aF.cTAF.throwToBase[base] = 1;
			stateInfo.localGameInfo->aF.cTAF.actionKeyLock = 1;
		}
	}
	else if(stateInfo.keyStates->released[control][actionKey] == 1)
	{
		if(stateInfo.localGameInfo->aF.cTAF.throwToBase[base] == 2)
		{
			stateInfo.localGameInfo->aF.cTAF.throwToBase[base] = 3;
		}
	}
}

static __inline void checkMove(int key, int control, int direction)
{
	if(stateInfo.keyStates->down[control][key] == 1)
	{
		if(stateInfo.localGameInfo->aF.cTAF.move[direction] == 0)
		{
			stateInfo.localGameInfo->aF.cTAF.move[direction] = 1;

		}
	}
	else if(stateInfo.keyStates->released[control][key] == 1)
	{
		if(stateInfo.localGameInfo->aF.cTAF.move[direction] != 0) // to avoid something weird when this is changed to 1 when ball is catched
		{
			stateInfo.localGameInfo->aF.cTAF.move[direction] = 3;
		}
	}
}

static __inline void checkChangePlayer(int key, int control)
{
	if(stateInfo.localGameInfo->aF.cTAF.actionKeyLock == 0)
	{
		if(stateInfo.keyStates->released[control][key] == 1)
		{
			if(stateInfo.localGameInfo->aF.cTAF.changePlayer == 0)
			{
				stateInfo.localGameInfo->aF.cTAF.changePlayer = 1;
				stateInfo.localGameInfo->aF.cTAF.actionKeyLock = 1;
			}
		}
	}
}

static __inline void checkDrop(int key, int control)
{
	if(stateInfo.localGameInfo->aF.cTAF.actionKeyLock == 0)
	{
		if(stateInfo.keyStates->released[control][key] == 1)
		{
			if(stateInfo.localGameInfo->aF.cTAF.dropBall == 0)
			{
				stateInfo.localGameInfo->aF.cTAF.dropBall = 1;
				stateInfo.localGameInfo->aF.cTAF.actionKeyLock = 1;
			}
		}
	}
}

static __inline void checkPitch(int key, int control)
{
	if(stateInfo.keyStates->down[control][key] == 1)
	{
		if(stateInfo.localGameInfo->aF.cTAF.pitch == 0)
		{
			if(stateInfo.localGameInfo->aF.cTAF.actionKeyLock == 0)
			{
				stateInfo.localGameInfo->aF.cTAF.pitch = 1;
				stateInfo.localGameInfo->aF.cTAF.actionKeyLock = 1;
			}
		}
		else if(stateInfo.localGameInfo->aF.cTAF.pitch == 4)
		{
			stateInfo.localGameInfo->aF.cTAF.pitch = 5;
		}
	}
	else if(stateInfo.keyStates->released[control][key] == 1)
	{
		if(stateInfo.localGameInfo->aF.cTAF.pitch == 2)
		{
			stateInfo.localGameInfo->aF.cTAF.pitch = 3;
		}
	}
}

static __inline void checkBatterSelection(int change, int select, int control)
{
	if(stateInfo.keyStates->released[control][change] == 1)
	{

		if(stateInfo.localGameInfo->aF.bTAF.chooseBatter == 0)
		{
			stateInfo.localGameInfo->aF.bTAF.chooseBatter = 1;
		}
	}
	else if(stateInfo.keyStates->released[control][select] == 1)
	{
		if(stateInfo.localGameInfo->aF.bTAF.chooseBatter == 0)
		{
			stateInfo.localGameInfo->aF.bTAF.chooseBatter = 2;
		}
	}
}

static __inline void checkFreeWalkDecision(int accept, int reject, int control)
{
	if(stateInfo.keyStates->released[control][accept] == 1)
	{

		if(stateInfo.localGameInfo->aF.bTAF.takeFreeWalk == 0)
		{
			stateInfo.localGameInfo->aF.bTAF.takeFreeWalk = 1;
		}
	}
	else if(stateInfo.keyStates->released[control][reject] == 1)
	{
		if(stateInfo.localGameInfo->aF.bTAF.takeFreeWalk == 0)
		{
			stateInfo.localGameInfo->aF.bTAF.takeFreeWalk = 2;
		}
	}
}

static __inline void checkBatterAngle(int increase, int decrease, int control)
{
	if(stateInfo.keyStates->down[control][increase] == 1)
	{
		if(stateInfo.localGameInfo->pRAI.battingGoingOn == 1)
		{
			if(stateInfo.localGameInfo->aF.bTAF.increaseBatterAngle == 0)
			{
				stateInfo.localGameInfo->aF.bTAF.increaseBatterAngle = 1;
			}
		}
	}
	else if(stateInfo.keyStates->released[control][increase] == 1)
	{
		if(stateInfo.localGameInfo->pRAI.battingGoingOn == 1)
		{
			if(stateInfo.localGameInfo->aF.bTAF.increaseBatterAngle != 0) 
			{
				stateInfo.localGameInfo->aF.bTAF.increaseBatterAngle = 3;
			}
		}
	}
	if(stateInfo.keyStates->down[control][decrease] == 1)
	{
		if(stateInfo.localGameInfo->pRAI.battingGoingOn == 1)
		{
			if(stateInfo.localGameInfo->aF.bTAF.decreaseBatterAngle == 0)
			{
				stateInfo.localGameInfo->aF.bTAF.decreaseBatterAngle = 1;
			}
		}
	}
	else if(stateInfo.keyStates->released[control][decrease] == 1)
	{
		if(stateInfo.localGameInfo->pRAI.battingGoingOn == 1)
		{
			if(stateInfo.localGameInfo->aF.bTAF.decreaseBatterAngle != 0) 
			{
				stateInfo.localGameInfo->aF.bTAF.decreaseBatterAngle = 3;
			}
		}
	}
}

static __inline void checkSwing(int key, int control)
{
	if(stateInfo.keyStates->down[control][key] == 1)
	{
		if(stateInfo.localGameInfo->aF.bTAF.swing == 1)
		{
			stateInfo.localGameInfo->aF.bTAF.swing = 2;
		}
	}
	else if(stateInfo.keyStates->released[control][key] == 1)
	{
		if(stateInfo.localGameInfo->aF.bTAF.swing == 3)
		{
			stateInfo.localGameInfo->aF.bTAF.swing = 4;
		}
	}
}

static __inline void checkBattingTeamRun(int key, int control, int base)
{
	if(stateInfo.keyStates->released[control][key] == 1)
	{
		if(stateInfo.localGameInfo->aF.bTAF.baseRun[base] == 0)
		{
			stateInfo.localGameInfo->aF.bTAF.baseRun[base] = 1;
		}
	}
}