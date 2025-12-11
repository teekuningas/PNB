#ifndef ACTION_STATE_H
#define ACTION_STATE_H

// AI Lock Constants
#define AI_NO_LOCK -1
#define AI_PITCH_LOCK 0
#define AI_THROW_LOCK 1
#define AI_DROP_LOCK 2

#define AI_WAITING_BATTER_LOCK 3
#define AI_WAITING_WALK_LOCK 4
#define AI_BATTING_LOCK 5
#define AI_CHANGE_LOCK 6

#define AI_CLICK_LOCK 7
#define AI_DOUBLE_CLICK_LOCK 8
#define AI_COME_BACK_LOCK 9
#define AI_COME_BACK_WRONG_PITCH_LOCK 10

extern unsigned int meterCounter;
extern unsigned int meterCounterMax;

extern int throwGoingOn;
extern int runBatFlag;
extern int aiWrongPitch;

extern int aiActionEventLock;
extern int aiLockUpdate;
extern int aiLockTimeoutCounter;

#endif
