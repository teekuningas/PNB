#ifndef COMMON_LOGIC_H
#define COMMON_LOGIC_H

#include "globals.h"

int isVectorSmallEnoughSphere(Vector3D *vector, float limit);
int isVectorSmallEnoughCircleXZV(Vector3D *vector, float limit);
int isVectorSmallEnoughCircleXZ(float dx, float dz, float limit);
void setVectorXYZ(Vector3D *vector, float x, float y, float z);
void setVectorV(Vector3D *vector1, Vector3D *vector2);
void setVectorXZ(Vector3D *vector, float x, float z);
void addToVectorXZ(Vector3D *vector, float x, float z);
void addToVectorV(Vector3D *vector1, Vector3D *vector2);
void stopMovement(StateInfo* stateInfo, int index);
void smoothOutMovement(StateInfo* stateInfo);
void stopTargetLookingPlayer(StateInfo* stateInfo, int index);
void setOrientation(StateInfo* stateInfo, int i);
void runToTarget(StateInfo* stateInfo, int index, Vector3D *target);
void moveToTarget(StateInfo* stateInfo, int index, Vector3D *target);
void movePlayerOut(StateInfo* stateInfo, int index);
void moveRankedToCatch(StateInfo* stateInfo);
void runToNextBase(StateInfo* stateInfo, int index, int base);
void runToPreviousBase(StateInfo* stateInfo, int index, int base);
void lead(StateInfo* stateInfo, int index);
int checkIfBallIsOutOfBounds(StateInfo* stateInfo);
void changePlayer(StateInfo* stateInfo);
void prepareBatter(StateInfo* stateInfo);
void calculateFreeWalk(StateInfo* stateInfo);
void initializeSpatialPlayerInformation(StateInfo* stateInfo);
void initializeInningPermanentPlayerInformation(StateInfo* stateInfo);
void initializeNonCriticalPlayerInformation(StateInfo* stateInfo);
void initializeCriticalBattingTeamInformation(StateInfo* stateInfo);
void initializeBallInfo(StateInfo* stateInfo);
void initializeActionInfo(StateInfo* stateInfo);
void initializeTemporaryGameAnalysisInfo(StateInfo* stateInfo);
void initializeCriticalGameInfo(StateInfo* stateInfo);
void initializeIndexInformation(StateInfo* stateInfo);
void initializePRAIInformation(StateInfo* stateInfo);
void setRunnerAndBatter(StateInfo* stateInfo);
void loadMutableWorldSettings(StateInfo* stateInfo);
#endif /* COMMON_LOGIC_H */
