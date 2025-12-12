#include "batting_system.h"

int batterSelect;
int battingFrameCount;
int increaseBattingFrameCount;
int selectedBattingPowerCount;
int selectedBattingAngleCount;
float batterAngle;
int batterAngleSpeed;
float batterAdvanceSpeed;
float batterAdvance;
int battingMode;
float batterAdvanceLimit;
int battingStopped;
int batterMoving;
int updateBatterLocationAndOrientation;

void initBattingSystem(void)
{
	batterSelect = 0;
	battingFrameCount = 0;
	increaseBattingFrameCount = 0;
	selectedBattingPowerCount = 0;
	selectedBattingAngleCount = 0;
	batterAngle = 0;
	batterAngleSpeed = 0;
	batterAdvanceSpeed = 0;
	batterAdvance = 0;
	battingMode = 0;
	batterAdvanceLimit = 0;
	battingStopped = 0;
	batterMoving = 0;
	updateBatterLocationAndOrientation = 0;
}