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
void stopMovement(PlayerInfo* playerInfo, int index);
void smoothOutMovement(MatchSession* match); // Still needs ActionFlags
void stopTargetLookingPlayer(PlayerInfo* playerInfo, PlayerRuntimeState* playerRuntime, int index);
void setOrientation(PlayerInfo* playerInfo, BallInfo* ballInfo, int i);
void runToTarget(PlayerInfo* playerInfo, int index, Vector3D *target);
void moveToTarget(PlayerInfo* playerInfo, int index, Vector3D *target);
void movePlayerOut(PlayerInfo* playerInfo, PlayerRuntimeState* playerRuntime, FieldPositions* fieldPositions, int index);
void moveRankedToCatch(MatchSession* match);
void runToNextBase(MatchSession* match, FieldPositions* fieldPositions, int index, BaseID base); // Needs GameControl, PRAI
void runToPreviousBase(MatchSession* match, FieldPositions* fieldPositions, int index, BaseID base); // Needs PRAI
void lead(PlayerInfo* playerInfo, PlayerRuntimeState* playerRuntime, FieldPositions* fieldPositions, int index);
void changePlayer(MatchSession* match);
void prepareBatter(MatchSession* match);
void calculateFreeWalk(MatchSession* match);
void initializeSpatialPlayerInformation(MatchSession* match, FieldPositions* fieldPositions, unsigned int* rng_seed);
void initializeInningPermanentPlayerInformation(MatchSession* match, Scoreboard* scoreboard, TeamData* teamData);
void initializeNonCriticalPlayerInformation(MatchSession* match);
void initializeBallInfo(MatchSession* match);
void initializeActionInfo(MatchSession* match);
void initializeTemporaryGameAnalysisInfo(MatchSession* match);
void initializeCriticalGameInfo(MatchSession* match, Scoreboard* scoreboard);
void initializeIndexInformation(MatchSession* match);
void initializePRAIInformation(MatchSession* match);
void setRunnerAndBatter(MatchSession* match, Scoreboard* scoreboard, FieldPositions* fieldPositions);
void clearFrameEvents(GameEvents* events);
void loadMutableWorldSettings(StateInfo* stateInfo, unsigned int* rng_seed);

#endif /* COMMON_LOGIC_H */
