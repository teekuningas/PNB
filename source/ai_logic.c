#include "globals.h"

#include "ai_logic.h"
#include "ai_logic_internal.h"

// we are going to implement this as if the AI would be pressing the keys. So we are analyzing flags here and setting keyStates accordingly.
// of course the input.c things must be locked when doing this.

void aiLogic()
{
	int battingTeamIndex = (stateInfo.globalGameInfo->
		inning+stateInfo.globalGameInfo->playsFirst+stateInfo.globalGameInfo->period)%2;
	int catchingTeamIndex = (battingTeamIndex+1)%2;
	if(stateInfo.globalGameInfo->teams[battingTeamIndex].control == 2)
	{

	}
	if(stateInfo.globalGameInfo->teams[catchingTeamIndex].control == 2)
	{

	}
}

