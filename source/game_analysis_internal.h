#ifndef GAME_ANALYSIS_INTERNAL_H
#define GAME_ANALYSIS_INTERNAL_H

#define BASE_RADIUS 2.0f
#define WOUNDING_CATCH_THRESHOLD (1.0f * (1 / (UPDATE_INTERVAL*1.0f/1000)))
#define OUT_OF_BOUNDS_THRESHOLD (2.0f * (1 / (UPDATE_INTERVAL*1.0f/1000)))

extern StateInfo stateInfo;

static int woundingCatchCounter;
static int outOfBoundsCounter;
static int endOfInningCounter;
static int nextPairCounter;
static int foulPlayEventFlag;
static int homeRunCameraCounter;

static __inline void checkForOuts();
static __inline void checkIfNextBatterDecision();
static __inline void strikesAndBalls();
static __inline void checkIfEndOfInning();
static __inline void woundingCatchEffects();
static __inline void foulPlay();
static __inline void checkForRuns();
static __inline void checkIfNextPair();

#endif /* GAME_ANALYSIS_INTERNAL_H */