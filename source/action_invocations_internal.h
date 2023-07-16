#ifndef ACTION_INVOCATIONS_INTERNAL_H
#define ACTION_INVOCATIONS_INTERNAL_H

extern StateInfo stateInfo;

static __inline void checkThrow(int key, int actionKey, int control, int base);

static __inline void checkDrop(int key, int control);

static __inline void checkMove(int key, int control, int direction);

static __inline void checkChangePlayer(int key, int control);

static __inline void checkPitch(int key, int control);

static __inline void checkBatterSelection(int change, int select, int control);

static __inline void checkFreeWalkDecision(int accept, int reject, int control);

static __inline void checkBatterAngle(int increase, int decrease, int control);

static __inline void checkSwing(int key, int control);

static __inline void checkBattingTeamRun(int key, int control, int base);
#endif /* ACTION_INVOCATIONS_INTERNAL_H */