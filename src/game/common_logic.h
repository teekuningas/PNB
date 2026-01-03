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
void stopMovement(LocalGameInfo* localGameInfo, int index);
void smoothOutMovement(LocalGameInfo* localGameInfo);
void stopTargetLookingPlayer(LocalGameInfo* localGameInfo, int index);
void setOrientation(LocalGameInfo* localGameInfo, int i);
void runToTarget(LocalGameInfo* localGameInfo, int index, Vector3D *target);
void moveToTarget(LocalGameInfo* localGameInfo, int index, Vector3D *target);
void movePlayerOut(LocalGameInfo* localGameInfo, FieldPositions* fieldPositions, int index);
void moveRankedToCatch(LocalGameInfo* localGameInfo);
void runToNextBase(LocalGameInfo* localGameInfo, FieldPositions* fieldPositions, int index, int base);
void runToPreviousBase(LocalGameInfo* localGameInfo, FieldPositions* fieldPositions, int index, int base);
void lead(LocalGameInfo* localGameInfo, FieldPositions* fieldPositions, int index);
int checkIfBallIsOutOfBounds(LocalGameInfo* localGameInfo, FieldPositions* fieldPositions);
void changePlayer(LocalGameInfo* localGameInfo);
void prepareBatter(LocalGameInfo* localGameInfo);
void calculateFreeWalk(LocalGameInfo* localGameInfo);
void initializeSpatialPlayerInformation(LocalGameInfo* localGameInfo, FieldPositions* fieldPositions, unsigned int* rng_seed);
void initializeInningPermanentPlayerInformation(LocalGameInfo* localGameInfo, GlobalGameInfo* globalGameInfo, TeamData* teamData);
void initializeNonCriticalPlayerInformation(LocalGameInfo* localGameInfo);
void initializeCriticalBattingTeamInformation(LocalGameInfo* localGameInfo);
void initializeBallInfo(LocalGameInfo* localGameInfo);
void initializeActionInfo(LocalGameInfo* localGameInfo);
void initializeTemporaryGameAnalysisInfo(LocalGameInfo* localGameInfo);
void initializeCriticalGameInfo(LocalGameInfo* localGameInfo, GlobalGameInfo* globalGameInfo);
void initializeIndexInformation(LocalGameInfo* localGameInfo);
void initializePRAIInformation(LocalGameInfo* localGameInfo);
void setRunnerAndBatter(LocalGameInfo* localGameInfo, GlobalGameInfo* globalGameInfo, FieldPositions* fieldPositions);
void loadMutableWorldSettings(StateInfo* stateInfo, unsigned int* rng_seed);

#endif /* COMMON_LOGIC_H */
