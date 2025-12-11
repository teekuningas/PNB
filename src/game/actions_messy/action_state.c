#include "action_state.h"

unsigned int meterCounter = 0;
unsigned int meterCounterMax = 0;

int throwGoingOn = 0;
int runBatFlag = 0;
int aiWrongPitch = 0;

int aiActionEventLock = AI_NO_LOCK;
int aiLockUpdate = 0;
int aiLockTimeoutCounter = -1;
