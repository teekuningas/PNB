#ifndef BATTING_SYSTEM_H
#define BATTING_SYSTEM_H

extern int batterSelect;
extern int battingFrameCount;
extern int increaseBattingFrameCount;
extern int selectedBattingPowerCount;
extern int selectedBattingAngleCount;
extern float batterAngle;
extern int batterAngleSpeed;
extern float batterAdvanceSpeed;
extern float batterAdvance;
extern int battingMode;
extern float batterAdvanceLimit;
extern int battingStopped;
extern int batterMoving;
extern int updateBatterLocationAndOrientation;

void initBattingSystem(void);

#endif