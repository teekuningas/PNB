#ifndef CATCHING_AI_STRATEGY_H
#define CATCHING_AI_STRATEGY_H

#include "globals.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int up;
	int down;
	int left;
	int right;
} MovementKeys;

typedef struct {
	int isOnBase;
	int takingFreeWalk;
	int base;
	int leading;
} CatchingRunnerInfo;

MovementKeys calculate_movement_keys(float dx, float dz);

int should_ai_throw(const PlayerIndexInfo* playerIndices, int catcherNearHome,
                    int replacerIndex, int replacerStage, int replacerBase, int replacerMoving,
                    int targetBase);

int should_ai_drop_ball(const WoundingState* woundingState, const GameControlFlags* gameControl,
                        int runner3OriginalBase, int runner3IsOnBase,
                        int runner2OriginalBase, int runner2IsOnBase,
                        int catcherHomeIndex, int hasBallIndex);

int determine_lead_base(const CatchingRunnerInfo* runners, int runnerCount, int randomValue);

#ifdef __cplusplus
}
#endif

#endif
