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
void stopMovement(int index);
void smoothOutMovement();
void stopTargetLookingPlayer(int index);
void setOrientation(int i);
void runToTarget(int index, Vector3D *target);
void moveToTarget(int index, Vector3D *target);
void movePlayerOut(int index);
void moveRankedToCatch();
void runToNextBase(int index, int base);
void runToPreviousBase(int index, int base);
void lead(int index);
int checkIfBallIsOutOfBounds();
void changePlayer();
void prepareBatter();
void calculateFreeWalk();
void initializeSpatialPlayerInformation();
void initializeInningPermanentPlayerInformation();
void initializeNonCriticalPlayerInformation();
void initializeCriticalBattingTeamInformation();
void initializeBallInfo();
void initializeActionInfo();
void initializeTemporaryGameAnalysisInfo();
void initializeCriticalGameInfo();
void initializeIndexInformation();
void initializePRAIInformation();
void setRunnerAndBatter();
void loadMutableWorldSettings();
#endif /* COMMON_LOGIC_H */
