#include <string.h>
#include "referee.h"
#include "rules_outs.h"
#include "rules_runs.h"
#include "rules_strikes.h"
#include "base_logic.h"
#include "geometry.h"
#include "vector_math.h"
#include "base_control.h"
#include "common_logic.h"

#define BASE_RADIUS 2.0f
#define HOME_RADIUS 6.0f
#define HOME_LINE_Z -0.65f
#define WOUNDING_CATCH_THRESHOLD (1.0f * (1 / (UPDATE_INTERVAL * 1.0f / 1000)))
#define OUT_OF_BOUNDS_THRESHOLD (2.0f * (1 / (UPDATE_INTERVAL * 1.0f / 1000)))

// ============================================================================
// Referee Update Pipeline (Milestone 15)
// ============================================================================

static void update_initialization_events(
    const StateInfo* stateInfo, RefereeState* referee, const GameEvents* events, BetweenPitchState* betweenPitchState,
    HalfInningState* halfInningState
)
{
    const MatchSession* game = stateInfo->match;

    // Batter Entered: Initialize safety for the new batter and reset count
    if (events->batterEntered) {
        for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
            if (game->playerInfo[i].bTPI.state == PLAYER_STATE_AT_BAT) {
                referee->battingPlayers[i].currentSafetyBase = BASE_HOME;
            }
        }
        // Reset strikes and balls for the new batter
        halfInningState->strikes = 0;
        halfInningState->balls = 0;
    }

    // Pitch Released: Snapshot state for the new pitch
    if (events->pitchReleased) {
        // 1. Reset Sticky Flags
        betweenPitchState->catchHasBeenMade = 0;
        betweenPitchState->hasBallHitGround = 0;
        betweenPitchState->resolutionProcessed = 0;
        referee->woundingEvaluationFinished = 0;
        referee->woundingEvaluationActive = 0;
        referee->woundingEvaluationTimer = -1;
        referee->foulTimer = -1;
        referee->foulState = FOUL_STATE_NONE;
        referee->ballInThirdBaseSincePitch = 0;

        // 2. Mark that a pitch has been released (for homerun contest pair tracking)
        if (stateInfo->match->scoreboard.period >= 4) {
            ((MatchSession*)game)->homeRunContestState.homerunPairHasPitch = 1;
        }

        // 3. Snapshot Strike Count
        referee->strikesAtPitchStart = halfInningState->strikes;

        // 4. Reset pending run flags
        for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
            referee->battingPlayers[i].hasPendingRun = 0;
            referee->battingPlayers[i].hasPendingRunOfHonor = 0;
        }

        // 5. Snapshot Base Positions (The "Legal Baseline" for this pitch)
        for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
            int index = i;
            if (game->playerInfo[index].bTPI.baseId != BASE_NONE) {
                BaseID baseId = game->playerInfo[index].bTPI.baseId;
                int base = base_to_int_index(baseId);

                // Special case: Player is advancing freely.
                // Their "pitch start" base is effectively the destination they are guaranteed to reach.
                if (game->playerInfo[index].bTPI.state == PLAYER_STATE_ADVANCING_FREELY) {
                    BaseID destBase;
                    if (baseId == BASE_THIRD)
                        destBase = BASE_HOME_SCORED;
                    else
                        destBase = base_get_next(baseId);

                    referee->battingPlayers[index].baseAtPitchStart = destBase;
                    referee->battingPlayers[index].currentSafetyBase = destBase;
                } else {
                    referee->battingPlayers[index].baseAtPitchStart = baseId;

                    // Determine safety status for snapshot
                    int hasSafety = 0;
                    if (base >= 0 && base < 4) {
                        if (get_base_controller((MatchSession*)game, (BaseID)base) == index) {
                            hasSafety = 1;
                        }
                    }
                    // Special case: Batter at home is considered to have "safety" in terms of not being irti yet
                    if (game->playerInfo[index].bTPI.state == PLAYER_STATE_AT_BAT) {
                        hasSafety = 1;
                    }

                    // Initialize current safety tracking
                    if (hasSafety) {
                        referee->battingPlayers[index].currentSafetyBase = baseId;
                    } else {
                        referee->battingPlayers[index].currentSafetyBase = BASE_NONE;
                    }
                }

                // Clear temporary rule states
                referee->battingPlayers[index].status = PLAYER_STATUS_ACTIVE;
            } else {
                // Clear baseAtPitchStart for inactive players
                referee->battingPlayers[index].baseAtPitchStart = BASE_NONE;
            }
        }
    }
}

static void update_foul_play_logic(
    const StateInfo* stateInfo, RefereeState* referee, HalfInningState* halfInningState, const GameEvents* events,
    BetweenPitchState* betweenPitchState
)
{
    const MatchSession* game = stateInfo->match;

    // Transition 3: RESETTING -> NONE (Cleanup after physical reset)
    if (referee->foulState == FOUL_STATE_RESETTING) {
        referee->foulState = FOUL_STATE_NONE;
        // Reset other flags as we are now "between pitches" effectively
        betweenPitchState->catchHasBeenMade = 0;
        betweenPitchState->hasBallHitGround = 0;
        betweenPitchState->resolutionProcessed = 0;
        return;
    }

    // Transition 2: DETECTED -> RESETTING (Grace period expired)
    if (referee->foulState == FOUL_STATE_DETECTED) {
        referee->foulTimer++;
        if (referee->foulTimer > (int)(2.0f * (1 / (UPDATE_INTERVAL * 1.0f / 1000)))) {
            // Trigger Reset
            referee->foulState = FOUL_STATE_RESETTING;
            referee->foulTimer = -1;

            // RESTORE LEGAL STATE
            for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
                BaseID restore = referee->battingPlayers[i].baseAtPitchStart;
                if (restore != BASE_NONE) {
                    referee->battingPlayers[i].currentSafetyBase = restore;
                } else {
                    referee->battingPlayers[i].currentSafetyBase = BASE_NONE;
                }

                if (restore != BASE_NONE) {
                    referee->battingPlayers[i].status = PLAYER_STATUS_ACTIVE;
                }

                referee->battingPlayers[i].hasPendingRun = 0;
                referee->battingPlayers[i].hasPendingRunOfHonor = 0;
                referee->battingPlayers[i].baseAtLastEvent = BASE_NONE;
            }

            // Handle Strikes and Outs logic
            if (referee->strikesAtPitchStart == 2) {
                halfInningState->strikes = 3;
                halfInningState->outs += 1;
                halfInningState->event = EVENT_OUT;

                for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
                    if (game->playerInfo[i].bTPI.state == PLAYER_STATE_AT_BAT) {
                        referee->battingPlayers[i].status = PLAYER_STATUS_OUT;
                        referee->battingPlayers[i].currentSafetyBase = BASE_NONE;
                        referee->battingPlayers[i].baseAtPitchStart = BASE_NONE;
                        break;
                    }
                }
            } else {
                halfInningState->strikes = referee->strikesAtPitchStart + 1;
            }
        }
        return;
    }

    // Transition 1: NONE -> DETECTED (Detection)
    // Out of Bounds Logic: Check ONLY on first bounce
    if (events->ballHitGround && betweenPitchState->hasBallHitGround == 0 && referee->foulState == FOUL_STATE_NONE) {
        if (game->pRAI.batHit == 1 && betweenPitchState->catchHasBeenMade == 0) {
            if (checkIfBallIsOutOfBounds((BallInfo*)&game->ballInfo, stateInfo->fieldPositions)) {
                // Transition to DETECTED
                referee->foulState = FOUL_STATE_DETECTED;
                referee->foulTimer = 0;

                // Trigger global event once
                if (halfInningState->event == EVENT_NONE) {
                    halfInningState->event = EVENT_OUT_OF_BOUNDS;
                }
            }
        }
    }
}

static void update_wounding_logic(
    const StateInfo* stateInfo, RefereeState* referee, HalfInningState* halfInningState, const GameEvents* events,
    const BetweenPitchState* betweenPitchState
)
{
    const MatchSession* game = stateInfo->match;

    // A. Start Wounding Evaluation (on catch event)
    // Check: fly ball caught (events->catchMade), ball was hit, hasn't hit ground yet, no prior catch
    if (events->catchMade && game->pRAI.batHit == 1 && betweenPitchState->hasBallHitGround == 0 &&
        betweenPitchState->catchHasBeenMade == 0) {

        // Start evaluation period
        referee->woundingEvaluationActive = 1;
        referee->woundingEvaluationTimer = 0;

        // Mark vulnerable players at this moment
        for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
            if (game->playerInfo[i].bTPI.baseId != BASE_NONE) {
                PlayerUnitState phys_state = game->playerInfo[i].bTPI.state;
                BaseID phys_base = game->playerInfo[i].bTPI.baseId;
                BaseID ref_baseAtStart = referee->battingPlayers[i].baseAtPitchStart;

                bool is_safe = player_is_safe_from_fly(phys_state, phys_base, ref_baseAtStart);

                // Check if player is vulnerable (not safe from fly ball)
                if (!is_safe) {
                    // Mark this player as vulnerable (WOUND_MARKED)
                    referee->battingPlayers[i].status = PLAYER_STATUS_WOUND_MARKED;
                }
            }
        }
    }

    // B. Monitor Wounding Evaluation Period
    if (referee->woundingEvaluationActive) {
        referee->woundingEvaluationTimer++;

        // Calculate threshold (extend if ball was thrown)
        int threshold;
        if (game->pII.hasBallIndex == -1) {
            threshold = (int)(2 * WOUNDING_CATCH_THRESHOLD);
        } else {
            threshold = (int)WOUNDING_CATCH_THRESHOLD;
        }

        // Check for UN-WOUNDING: Ball hit ground before timer expired
        if (game->ballInfo.hitsGroundToUnWound == 1) {
            // Clear evaluation - players are safe!
            referee->woundingEvaluationActive = 0;
            referee->woundingEvaluationTimer = -1;
            for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
                if (referee->battingPlayers[i].status == PLAYER_STATUS_WOUND_MARKED ||
                    referee->battingPlayers[i].status == PLAYER_STATUS_WOUND_MARKED_DOUBLE) {
                    referee->battingPlayers[i].status = PLAYER_STATUS_ACTIVE;
                }
            }
        }
        // Timer expired - CONFIRM WOUNDING
        else if (referee->woundingEvaluationTimer > threshold) {

            // Apply wounding to marked players
            for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
                if (game->playerInfo[i].bTPI.baseId != BASE_NONE) {
                    BaseID baseId = game->playerInfo[i].bTPI.baseId;
                    PlayerUnitState state = game->playerInfo[i].bTPI.state;

                    // Normal wound: WOUND_MARKED -> WOUNDED (if at base) or WOUND_PENDING (if running)
                    if (referee->battingPlayers[i].status == PLAYER_STATUS_WOUND_MARKED) {
                        // Trigger global event
                        halfInningState->event = EVENT_WOUNDED;
                        referee->battingPlayers[i].baseAtLastEvent = baseId;
                        referee->battingPlayers[i].currentSafetyBase = BASE_NONE;

                        // Check if player is at a base or running
                        if (state == PLAYER_STATE_ON_BASE || state == PLAYER_STATE_AT_BAT) {
                            // Player at base: Apply WOUNDED immediately
                            referee->battingPlayers[i].status = PLAYER_STATUS_WOUNDED;
                        } else {
                            // Player running: Mark as WOUND_PENDING (finalized on arrival or when ball arrives)
                            referee->battingPlayers[i].status = PLAYER_STATUS_WOUND_PENDING;
                        }
                    }
                    // Tuplahaava wound: WOUND_MARKED_DOUBLE -> WOUND_PENDING_DOUBLE (keep safety for now)
                    else if (referee->battingPlayers[i].status == PLAYER_STATUS_WOUND_MARKED_DOUBLE) {
                        // Trigger global event
                        halfInningState->event = EVENT_WOUNDED;

                        // Snapshot current base for event tracking
                        referee->battingPlayers[i].baseAtLastEvent = baseId;

                        // Tuplahaava: Keep safety, set to PENDING_DOUBLE (resolved later)
                        referee->battingPlayers[i].status = PLAYER_STATUS_WOUND_PENDING_DOUBLE;
                    }
                }
            }

            // End evaluation period
            referee->woundingEvaluationActive = 0;
            referee->woundingEvaluationFinished = 1;
            referee->woundingEvaluationTimer = -1;
        }
    }
}

static void update_safety_status(const StateInfo* stateInfo, RefereeState* referee)
{
    const MatchSession* game = stateInfo->match;

    // Process players in base order (HOME -> FIRST -> SECOND -> THIRD)
    // This ensures deterministic safety resolution when two players are at the same base.
    // Lead runners (higher bases at pitch start) are processed first and lose safety.
    // Rear runners (lower bases at pitch start) are processed last and win safety.

    // Build sorted list of active players by their baseAtPitchStart
    int sortedPlayers[PLAYERS_IN_TEAM + JOKER_COUNT];
    int playerCount = 0;

    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        const PlayerInfo* player = &game->playerInfo[i];
        if (player->bTPI.baseId != BASE_NONE &&
            (player->bTPI.state == PLAYER_STATE_ON_BASE || player->bTPI.state == PLAYER_STATE_AT_BAT)) {
            sortedPlayers[playerCount++] = i;
        }
    }

    // Sort by baseAtPitchStart (HIGHER bases first: THIRD=3, SECOND=2, FIRST=1, HOME=0)
    // So lead runners (higher bases) processed first, rear runners processed last and win conflicts
    for (int i = 0; i < playerCount - 1; i++) {
        for (int j = i + 1; j < playerCount; j++) {
            BaseID baseI = referee->battingPlayers[sortedPlayers[i]].baseAtPitchStart;
            BaseID baseJ = referee->battingPlayers[sortedPlayers[j]].baseAtPitchStart;
            if (baseI < baseJ) { // REVERSED: higher bases first
                int temp = sortedPlayers[i];
                sortedPlayers[i] = sortedPlayers[j];
                sortedPlayers[j] = temp;
            }
        }
    }

    // 2.5 Safety Acquisition &Displacement
    // Process in field order (rear runners processed last, win safety conflicts)
    for (int idx = 0; idx < playerCount; idx++) {
        int i = sortedPlayers[idx];
        const PlayerInfo* player = &game->playerInfo[i];
        BaseID physicalBase = player->bTPI.baseId;

        // Player is physically at a base (we already filtered for ON_BASE/AT_BAT)
        // But Referee doesn't recognize them as the controller yet
        if (referee->battingPlayers[i].currentSafetyBase != physicalBase) {

            // BUG FIX: Don't grant safety if wound status is WOUNDED or WOUND_PENDING
            // These statuses mean safety was already stripped and should not be restored
            int isWounded =
                (referee->battingPlayers[i].status == PLAYER_STATUS_WOUNDED ||
                 referee->battingPlayers[i].status == PLAYER_STATUS_WOUND_PENDING ||
                 referee->battingPlayers[i].status == PLAYER_STATUS_WOUND_PENDING_DOUBLE);

            if (!isWounded) {
                // Grant Safety (unless wounded/pending)
                referee->battingPlayers[i].currentSafetyBase = physicalBase;
            }

            // Check if this arriving player is marked for a wound
            int isWoundPending = (referee->battingPlayers[i].status == PLAYER_STATUS_WOUND_PENDING) ||
                                 (referee->battingPlayers[i].status == PLAYER_STATUS_WOUND_PENDING_DOUBLE) ||
                                 (referee->battingPlayers[i].status == PLAYER_STATUS_WOUNDED) ||
                                 (referee->battingPlayers[i].status == PLAYER_STATUS_WOUND_MARKED) ||
                                 (referee->battingPlayers[i].status == PLAYER_STATUS_WOUND_MARKED_DOUBLE);

            if (isWoundPending) {
                // TUPLAHAAVA LOGIC: Do not displace. Mark existing occupants for Tuplahaava.
                for (int j = 0; j < PLAYERS_IN_TEAM + JOKER_COUNT; j++) {
                    if (i == j) continue;

                    if (referee->battingPlayers[j].currentSafetyBase == physicalBase) {
                        // Found an existing occupant. Mark them for Tuplahaava.

                        // Don't downgrade stricter wound states
                        // WOUND_MARKED is stricter than WOUND_MARKED_DOUBLE
                        // WOUND_PENDING is stricter than WOUND_PENDING_DOUBLE
                        if (referee->battingPlayers[j].status == PLAYER_STATUS_WOUND_MARKED ||
                            referee->battingPlayers[j].status == PLAYER_STATUS_WOUND_PENDING) {
                            // Keep existing stricter status, don't change
                            continue;
                        }

                        // If the timer has already expired (arriving player has pending wound),
                        // we must immediately set the occupant to pending as well.
                        if (referee->battingPlayers[i].status == PLAYER_STATUS_WOUND_PENDING ||
                            referee->battingPlayers[i].status == PLAYER_STATUS_WOUND_PENDING_DOUBLE ||
                            referee->battingPlayers[i].status == PLAYER_STATUS_WOUNDED) {
                            referee->battingPlayers[j].status = PLAYER_STATUS_WOUND_PENDING_DOUBLE;
                        } else {
                            // Arriving player is still WOUND_MARKED (timer hasn't expired)
                            // Mark occupant as WOUND_MARKED_DOUBLE so timer knows to treat them as tuplahaava
                            referee->battingPlayers[j].status = PLAYER_STATUS_WOUND_MARKED_DOUBLE;
                        }

                        // Crucial: They KEEP safety for now.
                    }
                }
                // Ensure arriving player is marked correctly (normal wound if not already double)
                if (referee->battingPlayers[i].status == PLAYER_STATUS_WOUND_MARKED) {
                    // This shouldn't happen, but keep logic simple
                }
            } else {
                // STANDARD LOGIC: Displace old owner (if any)
                for (int j = 0; j < PLAYERS_IN_TEAM + JOKER_COUNT; j++) {
                    if (i == j) continue;

                    if (referee->battingPlayers[j].currentSafetyBase == physicalBase) {
                        referee->battingPlayers[j].currentSafetyBase = BASE_NONE;
                    }
                }
            }
        }
    }
}

static void update_force_outs(
    const StateInfo* stateInfo, RefereeState* referee, HalfInningState* halfInningState, int ballAtBase,
    BetweenPitchState* betweenPitchState
)
{
    const MatchSession* game = stateInfo->match;

    // Check for Force Outs (§33) and Safety Removal (§36)
    if (ballAtBase != -1) {
        for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
            const PlayerInfo* player = &game->playerInfo[i];

            // Only check active players
            if (player->bTPI.baseId == BASE_NONE) continue;

            BaseID ballBaseId = (BaseID)ballAtBase;
            BaseID checkBaseId;

            if (ballBaseId == BASE_HOME) {
                checkBaseId = BASE_THIRD; // Home base forces runner from 3rd
            } else {
                checkBaseId = (BaseID)(ballAtBase - 1);
            }

            // A. Force Out / Burning (§33)
            int has_safety_at_current = (referee->battingPlayers[i].currentSafetyBase == player->bTPI.baseId);
            int is_protected =
                (player->bTPI.state == PLAYER_STATE_ON_BASE || player->bTPI.state == PLAYER_STATE_AT_BAT);
            int is_safe_from_force_out = has_safety_at_current && is_protected;

            if (is_runner_forced_out(
                    player->bTPI.baseId, is_safe_from_force_out, checkBaseId,
                    player->bTPI.state == PLAYER_STATE_ADVANCING_FREELY, referee->foulState != FOUL_STATE_NONE
                )) {

                referee->battingPlayers[i].status = PLAYER_STATUS_OUT;
                halfInningState->outs += 1;
                halfInningState->event = EVENT_OUT; // Global event

                // Remove safety if they had it at this base
                if (has_safety_at_current) {
                    referee->battingPlayers[i].currentSafetyBase = BASE_NONE;
                }
            }

            // B. Safety Removal (§36 Koppilyönti logic)
            BaseID player_safety_base = referee->battingPlayers[i].currentSafetyBase;

            if (player_safety_base != BASE_NONE && player_safety_base == (BaseID)ballAtBase) {
                if (!is_protected) {
                    // Player has safety here but is "irti" - lose safety and must run
                    referee->battingPlayers[i].currentSafetyBase = BASE_NONE;
                }
            }
        }
    }
}

static void update_tuplahaava_logic(
    const StateInfo* stateInfo, RefereeState* referee, HalfInningState* halfInningState, int ballAtBase
)
{
    const MatchSession* game = stateInfo->match;

    // Tuplahaava Exceptions (Explicit Logic from game_analysis.c)
    // These checks must run every frame, not just when ball is at a base
    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        const PlayerInfo* player = &game->playerInfo[i];

        // Only check active players with Tuplahaava wounds
        if (referee->battingPlayers[i].status == PLAYER_STATUS_WOUND_PENDING_DOUBLE) {

            BaseID source = referee->battingPlayers[i].baseAtPitchStart;
            BaseID next = base_get_next(source);
            BaseID currentBase = player->bTPI.baseId;
            // In-between: RUNNING or LEADING (actively between bases)
            int is_in_between =
                (player->bTPI.state == PLAYER_STATE_RUNNING || player->bTPI.state == PLAYER_STATE_LEADING);

            if (is_in_between && ballAtBase != -1) {
                // Exception 2: Ball at NEXT base -> OUT
                if (base_to_int_index(next) == ballAtBase) {
                    referee->battingPlayers[i].status = PLAYER_STATUS_OUT;
                    halfInningState->outs += 1;
                    halfInningState->event = EVENT_OUT;
                    referee->battingPlayers[i].currentSafetyBase = BASE_NONE; // Clear source safety
                }
                // Exception 1: Ball at SOURCE base -> Lose Safety, become WOUNDED
                else if (base_to_int_index(source) == ballAtBase) {
                    referee->battingPlayers[i].currentSafetyBase = BASE_NONE;
                    referee->battingPlayers[i].status = PLAYER_STATUS_WOUNDED;
                }
            } else if (player->bTPI.state == PLAYER_STATE_ON_BASE || player->bTPI.state == PLAYER_STATE_AT_BAT) {
                // Player is ON_BASE or AT_BAT - check if at source or next base
                if (currentBase == source || currentBase == next) {
                    // Tuplahaava player at source/next base should be wounded
                    referee->battingPlayers[i].currentSafetyBase = BASE_NONE;
                    referee->battingPlayers[i].status = PLAYER_STATUS_WOUNDED;
                }
            }
        }

        // Handle regular WOUND_PENDING (non-tuplahaava) players
        if (referee->battingPlayers[i].status == PLAYER_STATUS_WOUND_PENDING) {

            BaseID source = referee->battingPlayers[i].baseAtPitchStart;
            BaseID next = base_get_next(source);
            BaseID currentBase = player->bTPI.baseId;
            // In-between: RUNNING or LEADING (actively between bases)
            // Note: ADVANCING_FREELY is treated like ON_BASE (safe at next base)
            int is_in_between =
                (player->bTPI.state == PLAYER_STATE_RUNNING || player->bTPI.state == PLAYER_STATE_LEADING);

            if (is_in_between && ballAtBase != -1) {
                // Ball at NEXT base -> OUT
                if (base_to_int_index(next) == ballAtBase) {
                    referee->battingPlayers[i].status = PLAYER_STATUS_OUT;
                    halfInningState->outs += 1;
                    halfInningState->event = EVENT_OUT;
                    referee->battingPlayers[i].currentSafetyBase = BASE_NONE;
                }
            } else if (player->bTPI.state == PLAYER_STATE_ON_BASE || player->bTPI.state == PLAYER_STATE_AT_BAT) {
                // Player arrived at base (ON_BASE or AT_BAT) -> finalize as WOUNDED
                if (currentBase == source || currentBase == next) {
                    referee->battingPlayers[i].currentSafetyBase = BASE_NONE;
                    referee->battingPlayers[i].status = PLAYER_STATUS_WOUNDED;
                }
            }
        }
    }
}

static void update_runs(
    const StateInfo* stateInfo, RefereeState* referee, HalfInningState* halfInningState,
    BetweenPitchState* betweenPitchState, PlayerCounters* playerCounters, Scoreboard* scoreboard
)
{
    const MatchSession* game = stateInfo->match;

    // 4. Check for Runs (§41/42)
    // We trigger this check if ANY player arrived at a base this frame.
    if (game->gameEvents.playerArrivedAtBase) {

        int isBallInAir = (betweenPitchState->catchHasBeenMade == 0 && betweenPitchState->hasBallHitGround == 0);
        int isCatchPending = (betweenPitchState->catchHasBeenMade == 1 && referee->woundingEvaluationFinished == 0);

        // Case A: Pending Run (Ball in Air OR Catch Evaluation Active)
        if (isBallInAir || isCatchPending) {
            for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
                if (game->playerInfo[i].bTPI.baseId != BASE_NONE) {
                    // Check for potential run
                    int regularRun = is_regular_run(
                        game->playerInfo[i].bTPI.baseId, referee->battingPlayers[i].baseAtPitchStart,
                        game->playerInfo[i].bTPI.state == PLAYER_STATE_WOUNDED
                    );

                    int runOfHonor = is_run_of_honor(
                        game->playerInfo[i].bTPI.baseId, referee->battingPlayers[i].baseAtPitchStart,
                        game->playerInfo[i].bTPI.state == PLAYER_STATE_WOUNDED,
                        referee->battingPlayers[i].runOfHonorScored
                    );

                    if (regularRun) {
                        referee->battingPlayers[i].hasPendingRun = 1;
                    }
                    if (runOfHonor) {
                        referee->battingPlayers[i].hasPendingRunOfHonor = 1;
                    }
                }
            }
        }
        // Case B: Ball Grounded or Catch Confirmed (Immediate Run)
        else if ((game->betweenPitchState.catchHasBeenMade == 1 || game->betweenPitchState.hasBallHitGround == 1) &&
                 referee->woundingEvaluationActive == 0 && referee->endOfInningState == END_INNING_STATE_NONE &&
                 referee->foulState == FOUL_STATE_NONE) {

            // Check all players
            for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
                if (game->playerInfo[i].bTPI.baseId != BASE_NONE) {
                    int regularRun = is_regular_run(
                        game->playerInfo[i].bTPI.baseId, referee->battingPlayers[i].baseAtPitchStart,
                        game->playerInfo[i].bTPI.state == PLAYER_STATE_WOUNDED
                    );

                    int runOfHonor = is_run_of_honor(
                        game->playerInfo[i].bTPI.baseId, referee->battingPlayers[i].baseAtPitchStart,
                        game->playerInfo[i].bTPI.state == PLAYER_STATE_WOUNDED,
                        referee->battingPlayers[i].runOfHonorScored
                    );

                    if (regularRun) {
                        halfInningState->event = EVENT_RUN_SCORED;
                        int battingTeamIndex = (scoreboard->inning + scoreboard->playsFirst + scoreboard->period) % 2;
                        scoreboard->teams[battingTeamIndex].runs += 1;
                        halfInningState->runsInTheInning += 1;

                        referee->battingPlayers[i].hasScored = 1;

                        if (halfInningState->runsInTheInning % 2 == 0) {
                            playerCounters->nonJokerPlayersLeft = PLAYERS_IN_TEAM;
                        }
                    } else if (runOfHonor) {
                        halfInningState->event = EVENT_RUN_SCORED;
                        int battingTeamIndex = (scoreboard->inning + scoreboard->playsFirst + scoreboard->period) % 2;
                        scoreboard->teams[battingTeamIndex].runs += 1;
                        halfInningState->runsInTheInning += 1;

                        referee->battingPlayers[i].runOfHonorScored = 1;
                        // Do NOT set hasScored=1, as that removes the player. Run of Honor players stay at 3rd.

                        // §42 Overtaking Logic (Check if Kunniajuoksu overtakes someone)
                        if (game->playerInfo[i].bTPI.baseId == BASE_THIRD) {
                            int someone_else_has_third_safety = 0;
                            for (int j = 0; j < PLAYERS_IN_TEAM + JOKER_COUNT; j++) {
                                if (j != i && referee->battingPlayers[j].currentSafetyBase == BASE_THIRD) {
                                    someone_else_has_third_safety = 1;
                                    break;
                                }
                            }

                            if (someone_else_has_third_safety) {
                                referee->battingPlayers[i].status = PLAYER_STATUS_OUT;
                                halfInningState->outs += 1;
                            }
                        }
                    }
                }
            }

            // Period End Check
            int battingTeamIndex = (scoreboard->inning + scoreboard->playsFirst + scoreboard->period) % 2;
            int catchingTeamIndex = (battingTeamIndex + 1) % 2;
            int currentRuns = scoreboard->teams[battingTeamIndex].runs;
            int opponentRuns = scoreboard->teams[catchingTeamIndex].runs;

            if (scoreboard->period < 4) {
                if ((scoreboard->inning + 1) % scoreboard->halfInningsInPeriod == 0 ||
                    scoreboard->inning + 1 == scoreboard->halfInningsInPeriod * 2 + 2) {
                    if (currentRuns > opponentRuns) {
                        halfInningState->endPeriod = 1;
                    }
                    if (scoreboard->inning + 1 == scoreboard->halfInningsInPeriod * 2 &&
                        scoreboard->teams[battingTeamIndex].period0Runs >
                            scoreboard->teams[catchingTeamIndex].period0Runs &&
                        opponentRuns == currentRuns) {
                        halfInningState->endPeriod = 1;
                    }
                }
            } else {
                if ((scoreboard->inning + 1) % 2 == 0) {
                    if (currentRuns > opponentRuns) {
                        halfInningState->endPeriod = 1;
                    }
                }
            }
        }
    }
}

static void update_strikes(RefereeState* referee, HalfInningState* halfInningState, const GameEvents* events)
{
    // 5. Strike Management
    // The referee is the sole authority on counting strikes based on physical events.
    if (events->ballHitByBat || events->ballMissedByBat) {
        halfInningState->strikes += 1;
    }
}

static void update_pitch_resolution(
    const StateInfo* stateInfo, HalfInningState* halfInningState, BetweenPitchState* betweenPitchState,
    FlowControl* flowControl, const GameEvents* events
)
{
    // Check if a pitch has physically concluded (hit ground) while still logically active
    if (events->ballHitGround && stateInfo->match->pRAI.pitchState != PITCH_STAGE_NONE) {

        PitchResult result =
            determine_pitch_result(stateInfo->match->ballInfo.location.x, PLATE_WIDTH, stateInfo->match->pRAI.batMiss);

        if (result == PITCH_RESULT_STRIKE) {
            halfInningState->strikes += 1;
            halfInningState->event = EVENT_STRIKE;
        } else if (result == PITCH_RESULT_BALL) {
            halfInningState->balls += 1;
            halfInningState->event = EVENT_BALL;

            // Reset free walk calculation flags so they are re-evaluated
            flowControl->freeWalkCalculationMade = 0;
            flowControl->freeWalkIndex = -1;
            flowControl->freeWalkBase = BASE_NONE;
        }

        // Signal to reconcile/cleanup that we have adjudicated this pitch
        betweenPitchState->resolutionProcessed = 1;
    }
}

static void update_free_walk_resolution(
    const StateInfo* stateInfo, RefereeState* referee, HalfInningState* halfInningState, PlayerCounters* playerCounters,
    Scoreboard* scoreboard, const FlowControl* flowControl, const GameEvents* events
)
{
    // 6. Free Walk Resolution
    if (events->freeWalkAccepted && flowControl->freeWalkIndex != -1) {
        int i = flowControl->freeWalkIndex;
        BaseID sourceBase = flowControl->freeWalkBase;
        int battingTeamIndex = (scoreboard->inning + scoreboard->playsFirst + scoreboard->period) % 2;
        int catchingTeamIndex = (battingTeamIndex + 1) % 2;

        if (scoreboard->period >= 4) {
            // Homerun Contest / Super Inning Logic
            referee->battingPlayers[i].baseAtPitchStart = BASE_HOME_SCORED;
            referee->battingPlayers[i].currentSafetyBase = BASE_HOME_SCORED;

            // Add run
            scoreboard->teams[battingTeamIndex].runs += 1;
            halfInningState->runsInTheInning += 1;

            if (halfInningState->balls >= 3) {
                scoreboard->teams[battingTeamIndex].runs += 1;
                halfInningState->runsInTheInning += 1;
                halfInningState->event = EVENT_TWO_RUNS_SCORED;
            } else {
                halfInningState->event = EVENT_RUN_SCORED;
            }

            // Period End Check
            if ((scoreboard->inning + 1) % 2 == 0) {
                if (scoreboard->teams[battingTeamIndex].runs > scoreboard->teams[catchingTeamIndex].runs) {
                    halfInningState->endPeriod = 1;
                }
            }

        } else {
            // Normal Game Logic
            if (sourceBase != BASE_THIRD) {
                // Advance to next base
                BaseID targetBase = base_get_next(sourceBase);
                referee->battingPlayers[i].baseAtPitchStart = targetBase;
                referee->battingPlayers[i].currentSafetyBase = targetBase;
            } else {
                // Score from 3rd base
                referee->battingPlayers[i].baseAtPitchStart = BASE_HOME_SCORED;
                referee->battingPlayers[i].currentSafetyBase = BASE_HOME_SCORED;

                // Add run
                scoreboard->teams[battingTeamIndex].runs += 1;
                halfInningState->runsInTheInning += 1;

                if (halfInningState->runsInTheInning % 2 == 0) {
                    playerCounters->nonJokerPlayersLeft = PLAYERS_IN_TEAM;
                    playerCounters->noMorePlayers = 0;
                }
                halfInningState->event = EVENT_RUN_SCORED;

                // Period End Check
                if ((scoreboard->inning + 1) % scoreboard->halfInningsInPeriod == 0 ||
                    scoreboard->inning + 1 == scoreboard->halfInningsInPeriod * 2 + 2) {
                    if (scoreboard->teams[battingTeamIndex].runs > scoreboard->teams[catchingTeamIndex].runs) {
                        halfInningState->endPeriod = 1;
                    }
                    if (scoreboard->inning + 1 == scoreboard->halfInningsInPeriod * 2 &&
                        scoreboard->teams[battingTeamIndex].period0Runs >
                            scoreboard->teams[catchingTeamIndex].period0Runs &&
                        scoreboard->teams[catchingTeamIndex].runs == scoreboard->teams[battingTeamIndex].runs) {
                        halfInningState->endPeriod = 1;
                    }
                }
            }
        }
    }
}

static void resolve_pending_runs(
    const StateInfo* stateInfo, RefereeState* referee, HalfInningState* halfInningState,
    BetweenPitchState* betweenPitchState, PlayerCounters* playerCounters, Scoreboard* scoreboard
)
{
    // Trigger 1: Ball Hit Ground (Final Verdict)
    if (betweenPitchState->hasBallHitGround) {
        if (referee->foulState != FOUL_STATE_NONE) {
            // FOUL: Void all pending runs
            for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
                referee->battingPlayers[i].hasPendingRun = 0;
                referee->battingPlayers[i].hasPendingRunOfHonor = 0;
            }
        } else {
            // SAFE HIT: Cash In all pending runs
            for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
                if (referee->battingPlayers[i].hasPendingRun) {
                    // Award Regular Run
                    halfInningState->event = EVENT_RUN_SCORED;
                    int battingTeamIndex = (scoreboard->inning + scoreboard->playsFirst + scoreboard->period) % 2;
                    scoreboard->teams[battingTeamIndex].runs += 1;
                    halfInningState->runsInTheInning += 1;

                    referee->battingPlayers[i].hasScored = 1;
                    referee->battingPlayers[i].hasPendingRun = 0;

                    if (halfInningState->runsInTheInning % 2 == 0) {
                        playerCounters->nonJokerPlayersLeft = PLAYERS_IN_TEAM;
                    }
                }
                if (referee->battingPlayers[i].hasPendingRunOfHonor) {
                    // Award Run of Honor
                    halfInningState->event = EVENT_RUN_SCORED;
                    int battingTeamIndex = (scoreboard->inning + scoreboard->playsFirst + scoreboard->period) % 2;
                    scoreboard->teams[battingTeamIndex].runs += 1;
                    halfInningState->runsInTheInning += 1;

                    referee->battingPlayers[i].runOfHonorScored = 1;
                    referee->battingPlayers[i].hasPendingRunOfHonor = 0;

                    // Overtaking logic already handled at arrival time if needed,
                    // but usually overtaking happens physically.
                    // Referee check in update_runs handles logical overtaking for HR.
                }
            }
        }
    }
    // Trigger 2: Catch Confirmed (Wounding Evaluation Finished)
    else if (referee->woundingEvaluationFinished) {
        // CATCH:
        // 1. Regular Pending Runs are VALID IF NOT WOUNDED (Runner beat the catch)
        // 2. Pending HRs are VOID (Batter is burnt/wounded by the catch)

        for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
            if (referee->battingPlayers[i].hasPendingRun) {
                // Only award run if player was NOT wounded by the catch
                if (referee->battingPlayers[i].status < PLAYER_STATUS_WOUND_MARKED) {
                    // Award Regular Run
                    halfInningState->event = EVENT_RUN_SCORED;
                    int battingTeamIndex = (scoreboard->inning + scoreboard->playsFirst + scoreboard->period) % 2;
                    scoreboard->teams[battingTeamIndex].runs += 1;
                    halfInningState->runsInTheInning += 1;

                    referee->battingPlayers[i].hasScored = 1;
                } else {
                    // Player was wounded despite arriving (e.g. didn't escape in time logically?)
                    // Run voided.
                }
                referee->battingPlayers[i].hasPendingRun = 0;

                if (halfInningState->runsInTheInning % 2 == 0) {
                    playerCounters->nonJokerPlayersLeft = PLAYERS_IN_TEAM;
                }
            }

            if (referee->battingPlayers[i].hasPendingRunOfHonor) {
                // Void Run of Honor (Batter burnt)
                referee->battingPlayers[i].hasPendingRunOfHonor = 0;
            }
        }
    }
}

static void update_game_state_flags(
    StateInfo* stateInfo, RefereeState* referee, HalfInningState* halfInningState, const GameEvents* events,
    BetweenPitchState* betweenPitchState
)
{
    // 6. Game State Flags

    // Persistent Catch State Management
    // When a catch is made, we lock it in.
    if (events->catchMade) {
        betweenPitchState->catchHasBeenMade = 1;
    }
}

void update_referee(
    const StateInfo* stateInfo, RefereeState* refereeState, HalfInningState* halfInningState,
    BetweenPitchState* betweenPitchState, PlayerCounters* playerCounters, Scoreboard* scoreboard
)
{
    const MatchSession* game = stateInfo->match;
    const FlowControl* flowControl = &game->flowControl; // Read-only access to flow data

    // 0. Initialization Events (Milestone 17)
    update_initialization_events(
        stateInfo, refereeState, &stateInfo->match->gameEvents, betweenPitchState, halfInningState
    );

    // 1. Where is the ball?
    int ballAtBase = get_ball_at_base_index(stateInfo);

    // 2. Track if ball has been at 3rd base since pitch started (for run of honor logic)
    if (ballAtBase == 3) {
        refereeState->ballInThirdBaseSincePitch = 1;
    }

    // 2.5 Foul Play & Wounding Logic (Milestone 17)
    update_foul_play_logic(stateInfo, refereeState, halfInningState, &stateInfo->match->gameEvents, betweenPitchState);

    // If foul play reset is triggered, the physical world is out of sync with legal state.
    // We must abort further processing this frame to prevent the Referee from
    // "correcting" the legal state based on the stale physical positions.
    // The physical reset will happen in GameConsolidation later this frame.
    if (refereeState->foulState == FOUL_STATE_RESETTING) {
        return;
    }

    // Mark that ball hit ground (after foul play check, so it can detect first bounce)
    if (stateInfo->match->gameEvents.ballHitGround) {
        betweenPitchState->hasBallHitGround = 1;
    }

    update_wounding_logic(stateInfo, refereeState, halfInningState, &stateInfo->match->gameEvents, betweenPitchState);

    // 3. Safety Pipeline
    update_safety_status(stateInfo, refereeState);
    update_force_outs(stateInfo, refereeState, halfInningState, ballAtBase, betweenPitchState);
    update_tuplahaava_logic(stateInfo, refereeState, halfInningState, ballAtBase);
    update_runs(stateInfo, refereeState, halfInningState, betweenPitchState, playerCounters, scoreboard);

    // 3.5 Resolve Pending Runs (Milestone 17)
    resolve_pending_runs(stateInfo, refereeState, halfInningState, betweenPitchState, playerCounters, scoreboard);

    // 4. Strikes
    update_strikes(refereeState, halfInningState, &stateInfo->match->gameEvents);
    update_pitch_resolution(
        stateInfo, halfInningState, betweenPitchState, (FlowControl*)&game->flowControl, &stateInfo->match->gameEvents
    );
    update_free_walk_resolution(
        stateInfo, refereeState, halfInningState, playerCounters, scoreboard, flowControl, &stateInfo->match->gameEvents
    );

    // 5. Game State Flags
    update_game_state_flags(
        (StateInfo*)stateInfo, refereeState, halfInningState, &stateInfo->match->gameEvents, betweenPitchState
    );

    // 6. Homerun Contest: Check if current pair is complete
    if (scoreboard->period >= 4) {
        // State Machine for Pair Lifecycle
        // 0 = NONE: Check for end condition
        // 1 = DETECTED: Timer running
        // 2 = RESETTING: Signal to Consolidation (1 frame)

        HomeRunPairState currentState = refereeState->nextPairTransitionState;

        if (currentState == HR_PAIR_STATE_NONE) {
            // Only check pair-ending conditions after at least one pitch has been released
            // This prevents premature "NEXT PAIR" at the start when baseAtPitchStart is still BASE_NONE
            if (((MatchSession*)game)->homeRunContestState.homerunPairHasPitch) {

                // Identify batter and runner using baseAtPitchStart (reliable after first pitch)
                int batterIndex = -1;
                int runnerIndex = -1;

                for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
                    if (refereeState->battingPlayers[i].baseAtPitchStart == BASE_HOME) {
                        batterIndex = i;
                    }
                    if (refereeState->battingPlayers[i].baseAtPitchStart == BASE_THIRD) {
                        runnerIndex = i;
                    }
                }

                // Get current state information
                int ballHome = ((MatchSession*)game)->gameFlowState.ballHome;

                // Check batter status
                BaseID batterBaseId = BASE_NONE;
                PlayerUnitState batterState = PLAYER_STATE_IDLE;
                if (batterIndex != -1) {
                    batterBaseId = game->playerInfo[batterIndex].bTPI.baseId;
                    batterState = game->playerInfo[batterIndex].bTPI.state;
                }

                // Check runner status
                BaseID runnerBaseId = BASE_NONE;
                if (runnerIndex != -1) {
                    runnerBaseId = game->playerInfo[runnerIndex].bTPI.baseId;
                }

                // Check if batter has safety at home
                int batterHasSafetyAtHome = 0;
                if (batterIndex != -1 && refereeState->battingPlayers[batterIndex].currentSafetyBase == BASE_HOME) {
                    batterHasSafetyAtHome = 1;
                }

                // Check if batter is advancing from second to third (only realistic chance to reach third)
                int batterAdvancingToThird = 0;
                if (batterIndex != -1 && batterBaseId == BASE_SECOND && batterState != PLAYER_STATE_ON_BASE) {
                    batterAdvancingToThird = 1;
                }

                int shouldEndPair = 0;

                // CONDITION 1: Runner at third base (still in play)
                // Continue if: batter has safety at home OR batter advancing 2nd→3rd
                // Quit if: ball home AND batter not in those safe states
                if (runnerBaseId == BASE_THIRD && ballHome) {
                    if (!batterHasSafetyAtHome && !batterAdvancingToThird) {
                        shouldEndPair = 1;
                    }
                }

                // CONDITION 2: Runner NOT at third (scored/out/gone)
                // Quit if: ball home AND batter not advancing 2nd→3rd
                if (runnerBaseId != BASE_THIRD && ballHome) {
                    if (!batterAdvancingToThird) {
                        shouldEndPair = 1;
                    }
                }

                // CONDITION 3: Safety net - no players with safety anywhere
                // Quit if: ball home AND no one has safety at any base
                if (ballHome) {
                    int anyoneHasSafety = 0;
                    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
                        if (refereeState->battingPlayers[i].currentSafetyBase != BASE_NONE) {
                            anyoneHasSafety = 1;
                            break;
                        }
                    }
                    if (!anyoneHasSafety) {
                        shouldEndPair = 1;
                    }
                }

                if (shouldEndPair) {
                    // Transition to DETECTED
                    refereeState->nextPairTransitionState = HR_PAIR_STATE_DETECTED;
                    refereeState->nextPairTimer = 0;
                    // Signal UI (for display only, not used for logic)
                    if (refereeState->endOfInningState == END_INNING_STATE_NONE) {
                        halfInningState->event = EVENT_NEXT_PAIR;
                    }
                }
            }
        } else if (currentState == HR_PAIR_STATE_DETECTED) {
            refereeState->nextPairTimer++;
            if (refereeState->nextPairTimer > 200) {
                // Transition to RESETTING and clear referee state
                refereeState->nextPairTransitionState = HR_PAIR_STATE_RESETTING;
                refereeState->nextPairTimer = -1;

                // Reset homerun pair pitch tracking for the next pair
                ((MatchSession*)game)->homeRunContestState.homerunPairHasPitch = 0;

                // Clear referee state NOW (before consolidation resets physical world)
                halfInningState->strikes = 0;
                halfInningState->balls = 0;
                betweenPitchState->catchHasBeenMade = 0;
                betweenPitchState->hasBallHitGround = 0;
                refereeState->foulState = FOUL_STATE_NONE;
                betweenPitchState->resolutionProcessed = 0;

                for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
                    refereeState->battingPlayers[i].baseAtPitchStart = BASE_NONE;
                    refereeState->battingPlayers[i].currentSafetyBase = BASE_NONE;
                    refereeState->battingPlayers[i].status = PLAYER_STATUS_ACTIVE;
                    refereeState->battingPlayers[i].hasScored = 0;
                    refereeState->battingPlayers[i].runOfHonorScored = 0;
                    refereeState->battingPlayers[i].hasPendingRun = 0;
                    refereeState->battingPlayers[i].hasPendingRunOfHonor = 0;
                }
            }
        } else if (currentState == HR_PAIR_STATE_RESETTING) {
            // Consolidation saw RESETTING and reset the physical world.
            // Transition back to ACTIVE and scan for new batter/runner pair.
            refereeState->nextPairTransitionState = HR_PAIR_STATE_NONE;

            // Scan physical world and initialize safety for new pair
            for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
                if (game->playerInfo[i].bTPI.state == PLAYER_STATE_AT_BAT) {
                    refereeState->battingPlayers[i].baseAtPitchStart = BASE_HOME;
                    refereeState->battingPlayers[i].currentSafetyBase = BASE_HOME;
                } else if (game->playerInfo[i].bTPI.state == PLAYER_STATE_ON_BASE &&
                           game->playerInfo[i].bTPI.baseId == BASE_THIRD) {
                    refereeState->battingPlayers[i].baseAtPitchStart = BASE_THIRD;
                    refereeState->battingPlayers[i].currentSafetyBase = BASE_THIRD;
                }
            }
        }
    }

    // 7. End of Inning State Machine
    // State Machine for Inning Lifecycle
    // 0 = NONE: Check for end condition
    // 1 = DETECTED: Timer running
    // 2 = RESETTING: Signal to Consolidation (1 frame)

    EndOfInningTransitionState endState = refereeState->endOfInningState;

    if (endState == END_INNING_STATE_NONE) {
        // Check if inning should end:
        // - Three outs (ONLY in normal/super inning modes, NOT in homerun contest) OR
        // - No more players to bat AND ball is home OR
        // - Period should end OR
        // - Homerun contest: all pairs complete
        int shouldEndInning = 0;

        if (scoreboard->period >= 4) {
            // Homerun Contest mode: inning ends only when all pairs complete or period ends
            shouldEndInning =
                (halfInningState->endPeriod == 1) ||
                (((MatchSession*)game)->homeRunContestState.runnerBatterPairCounter >= scoreboard->pairCount);
        } else {
            // Normal/Super Inning mode: inning ends on 3 outs or no more players
            shouldEndInning =
                (halfInningState->outs >= 3) ||
                (playerCounters->noMorePlayers == 1 && ((MatchSession*)game)->gameFlowState.ballHome == 1) ||
                (halfInningState->endPeriod == 1);
        }

        if (shouldEndInning) {
            // Transition to DETECTED
            refereeState->endOfInningState = END_INNING_STATE_DETECTED;
            refereeState->endInningTimer = 0;
            // Prevent further batter decisions
            ((FlowControl*)flowControl)->waitingForBatterDecision = 0;
            halfInningState->event = EVENT_INNING_ENDING;
        }
    } else if (endState == END_INNING_STATE_DETECTED) {
        refereeState->endInningTimer++;
        if (refereeState->endInningTimer > 200) {
            // Transition to RESETTING and clear referee state
            refereeState->endOfInningState = END_INNING_STATE_RESETTING;
            refereeState->endInningTimer = -1;

            // Reset homerun pair pitch tracking for the next inning
            if (scoreboard->period >= 4) {
                ((MatchSession*)game)->homeRunContestState.homerunPairHasPitch = 0;
            }

            // Clear referee state NOW (before consolidation resets physical world)
            halfInningState->strikes = 0;
            halfInningState->balls = 0;
            betweenPitchState->catchHasBeenMade = 0;
            betweenPitchState->hasBallHitGround = 0;
            refereeState->foulState = FOUL_STATE_NONE;
            betweenPitchState->resolutionProcessed = 0;

            for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
                refereeState->battingPlayers[i].baseAtPitchStart = BASE_NONE;
                refereeState->battingPlayers[i].currentSafetyBase = BASE_NONE;
                refereeState->battingPlayers[i].status = PLAYER_STATUS_ACTIVE;
                refereeState->battingPlayers[i].hasScored = 0;
                refereeState->battingPlayers[i].runOfHonorScored = 0;
                refereeState->battingPlayers[i].hasPendingRun = 0;
                refereeState->battingPlayers[i].hasPendingRunOfHonor = 0;
            }
        }
    } else if (endState == END_INNING_STATE_RESETTING) {
        // Consolidation saw RESETTING and reset the physical world.
        // Transition back to NONE.
        refereeState->endOfInningState = END_INNING_STATE_NONE;
    }
}
int is_wounding_evaluation_active(const RefereeState* ref)
{
    return ref->woundingEvaluationActive;
}

int get_wounding_evaluation_timer(const RefereeState* ref)
{
    return ref->woundingEvaluationTimer;
}

void initialize_referee(const StateInfo* stateInfo)
{
    const MatchSession* game = stateInfo->match;
    RefereeState* referee = &((MatchSession*)game)->referee;

    // Full reset first
    initializeRefereeState(referee);

    // Scan physical world and initialize safety based on player positions
    if (game->scoreboard.period >= 4) {
        // Homerun Contest (period >= 4): scan for batter at HOME, runner at THIRD
        // Reset homerun pair pitch tracking
        ((MatchSession*)game)->homeRunContestState.homerunPairHasPitch = 0;

        // Initialize Legal State for Batter/Runner based on Physical World
        // baseAtPitchStart will be set on pitchReleased
        for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
            if (game->playerInfo[i].bTPI.state == PLAYER_STATE_AT_BAT) {
                referee->battingPlayers[i].currentSafetyBase = BASE_HOME;
            } else if (game->playerInfo[i].bTPI.state == PLAYER_STATE_ON_BASE &&
                       game->playerInfo[i].bTPI.baseId == BASE_THIRD) {
                referee->battingPlayers[i].currentSafetyBase = BASE_THIRD;
            }
        }
    } else {
        // Normal games: scan for ANY players that are already positioned
        // This handles both: (1) real game initialization, (2) test fixtures
        // baseAtPitchStart will be set on pitchReleased
        for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
            PlayerUnitState state = game->playerInfo[i].bTPI.state;
            BaseID base = game->playerInfo[i].bTPI.baseId;

            if (state == PLAYER_STATE_AT_BAT && base == BASE_HOME) {
                referee->battingPlayers[i].currentSafetyBase = BASE_HOME;
            } else if (state == PLAYER_STATE_ON_BASE && base != BASE_NONE) {
                referee->battingPlayers[i].currentSafetyBase = base;
            } else if (state == PLAYER_STATE_RUNNING && base != BASE_NONE) {
                // Runner between bases - has left their starting base
                referee->battingPlayers[i].currentSafetyBase = BASE_NONE;
            }
        }
    }
}

int is_player_marked_for_wound(const RefereeState* ref, int playerIndex)
{
    if (playerIndex < 0 || playerIndex >= PLAYERS_IN_TEAM + JOKER_COUNT) {
        return 0;
    }
    return (
        ref->battingPlayers[playerIndex].status == PLAYER_STATUS_WOUND_MARKED ||
        ref->battingPlayers[playerIndex].status == PLAYER_STATUS_WOUND_MARKED_DOUBLE
    );
}

void initializeRefereeState(RefereeState* referee)
{
    int i;
    for (i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        referee->battingPlayers[i].baseAtPitchStart = BASE_NONE;
        referee->battingPlayers[i].currentSafetyBase = BASE_NONE;
        referee->battingPlayers[i].status = PLAYER_STATUS_ACTIVE;
        referee->battingPlayers[i].hasScored = 0;
        referee->battingPlayers[i].runOfHonorScored = 0;
        referee->battingPlayers[i].baseAtLastEvent = BASE_NONE;
        referee->battingPlayers[i].hadSafetyAtLastEvent = 0;
        referee->battingPlayers[i].hasPendingRun = 0;
        referee->battingPlayers[i].hasPendingRunOfHonor = 0;
    }
    referee->strikesAtPitchStart = 0;
    referee->woundingEvaluationActive = 0;
    referee->woundingEvaluationFinished = 0;
    referee->woundingEvaluationTimer = -1;
    referee->ballInThirdBaseSincePitch = 0;

    referee->foulState = FOUL_STATE_NONE;
    referee->foulTimer = -1;
    referee->nextPairTransitionState = HR_PAIR_STATE_NONE;
    referee->nextPairTimer = -1;
    referee->endOfInningState = END_INNING_STATE_NONE;
    referee->endInningTimer = -1;
}
