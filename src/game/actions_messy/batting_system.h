#ifndef BATTING_SYSTEM_H
#define BATTING_SYSTEM_H

#include "globals.h"

#define BAT_LOAD_MAX (4*9)
#define BAT_SWING_MAX (4*13)

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

void selectBatter(StateInfo* stateInfo);
void startIncreaseBatterAngle(StateInfo* stateInfo);
void stopIncreaseBatterAngle(StateInfo* stateInfo);
void startDecreaseBatterAngle(StateInfo* stateInfo);
void stopDecreaseBatterAngle(StateInfo* stateInfo);
void selectPower(StateInfo* stateInfo);
void selectAngle(StateInfo* stateInfo);
void updateBatting(StateInfo* stateInfo);
void updateBattingMeter(StateInfo* stateInfo);

#endif
