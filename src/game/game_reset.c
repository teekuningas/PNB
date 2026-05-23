#include "game_reset.h"
#include "common_logic.h"
#include "referee.h"

void resetPhysicalWorld(StateInfo* stateInfo, unsigned int* rng_seed) {
    MatchSession* game = stateInfo->match;
    initializeBallInfo(game);
    initializeActionInfo(game);
    initializeIndexInformation(game);
    initializePRAIInformation(game);
    initializeSpatialPlayerInformation(game, stateInfo->fieldPositions, rng_seed);
    initializeNonCriticalPlayerInformation(game);
}

// Recipe 1: Full physical + flow + team reset for new half-inning.
// Called by consolidation when referee signals END_INNING_STATE_RESETTING.
// NOTE: Referee has already cleared its own legal state at DETECTED→RESETTING.
// This function does NOT touch referee-owned state (ownership boundary).
void resetForNewHalfInning(StateInfo* stateInfo, unsigned int* rng_seed) {
    resetPhysicalWorld(stateInfo, rng_seed);
    resetFlowState(stateInfo->match);

    // Team setup
    initializeCriticalGameInfo(stateInfo->match, &stateInfo->match->scoreboard);
    initializeInningPermanentPlayerInformation(stateInfo->match, &stateInfo->match->scoreboard, stateInfo->teamData);
    
    if (stateInfo->match->scoreboard.period >= 4) {
        if (!(stateInfo->match->homeRunContestState.runnerBatterPairCounter > 0 &&
              stateInfo->match->homeRunContestState.runnerBatterPairCounter < stateInfo->match->scoreboard.pairCount)) {
            stateInfo->match->homeRunContestState.runnerBatterPairCounter = 0;
        }
        setupHomerunPhysicalState(stateInfo->match, &stateInfo->match->scoreboard, stateInfo->fieldPositions);
    }
}

// Recipe 2: Foul play — referee already restored legal state from snapshot
void resetForFoulPlay(StateInfo* stateInfo, unsigned int* rng_seed) {
    resetPhysicalWorld(stateInfo, rng_seed);
    resetFlowState(stateInfo->match);
    
    MatchSession* game = stateInfo->match;
    if (game->scoreboard.period >= 4) {
        // Homerun Contest special initialization
        setupHomerunPhysicalState(game, &game->scoreboard, stateInfo->fieldPositions);
    } else {
        // restorePlayersToRefereePositions: Restore players to their bases at the start of the pitch
        for (int j = 0; j < PLAYERS_IN_TEAM + JOKER_COUNT; j++) {
            if (game->referee.battingPlayers[j].baseAtPitchStart != BASE_NONE) {
                BaseID restoreBase = game->referee.battingPlayers[j].baseAtPitchStart;

                // 1. Restore Player State and ID (Physical/Logical State)
                if (restoreBase == BASE_HOME) {
                    game->playerInfo[j].bTPI.state = PLAYER_STATE_AT_BAT;
                } else {
                    game->playerInfo[j].bTPI.state = PLAYER_STATE_ON_BASE;
                }
                game->playerInfo[j].bTPI.baseId = restoreBase;

                // 5. Handle the Batter - Physical State Only
                if (restoreBase == BASE_HOME) {
                    // Prepare batter for next pitch (animation etc)
                    prepareBatter(game);
                }

                // 6. Restore Physical Locations for field runners
                if (game->playerInfo[j].bTPI.baseId == BASE_FIRST) {
                    game->playerInfo[j].tPI.location.x = stateInfo->fieldPositions->firstBaseRun.x;
                    game->playerInfo[j].tPI.location.z = stateInfo->fieldPositions->firstBaseRun.z;
                } else if (game->playerInfo[j].bTPI.baseId == BASE_SECOND) {
                    game->playerInfo[j].tPI.location.x = stateInfo->fieldPositions->secondBaseRun.x;
                    game->playerInfo[j].tPI.location.z = stateInfo->fieldPositions->secondBaseRun.z;
                } else if (game->playerInfo[j].bTPI.baseId == BASE_THIRD) {
                    game->playerInfo[j].tPI.location.x = stateInfo->fieldPositions->thirdBaseRun.x;
                    game->playerInfo[j].tPI.location.z = stateInfo->fieldPositions->thirdBaseRun.z;
                }
            } else {
                // Restore OUT/SCORED/WOUNDED states to avoid re-triggering animations
                // This is physical state sync
                if (game->referee.battingPlayers[j].status == PLAYER_STATUS_OUT) {
                    game->playerInfo[j].bTPI.state = PLAYER_STATE_OUT;
                    game->playerInfo[j].bTPI.baseId = BASE_NONE;
                } else if (game->referee.battingPlayers[j].hasScored) {
                    game->playerInfo[j].bTPI.state = PLAYER_STATE_SCORED;
                    game->playerInfo[j].bTPI.baseId = BASE_NONE;
                } else if (game->referee.battingPlayers[j].status == PLAYER_STATUS_WOUNDED) {
                    game->playerInfo[j].bTPI.state = PLAYER_STATE_WOUNDED;
                    game->playerInfo[j].bTPI.baseId = BASE_NONE;
                }
            }
        }
    }
}

// Recipe 3: Next HR pair — referee already cleared per-pair state
void resetForNextPair(StateInfo* stateInfo, unsigned int* rng_seed) {
    resetPhysicalWorld(stateInfo, rng_seed);
    resetFlowState(stateInfo->match);
    setupHomerunPhysicalState(stateInfo->match, &stateInfo->match->scoreboard, stateInfo->fieldPositions);
}
