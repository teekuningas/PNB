#include <string.h>
#include "referee.h"
#include "rules_outs.h"
#include "rules_runs.h"
#include "rules_strikes.h"
#include "base_logic.h"
#include "rules_pure/player_utils.h"
#include "scoring_helpers.h"
#include "geometry.h"
#include "vector_math.h"
#include "base_control.h"
#include "common_logic.h"

// Referee-specific timing thresholds (in frames)
// BASE_RADIUS, HOME_RADIUS, HOME_LINE_Z come from globals.h
#define WOUNDING_CATCH_THRESHOLD (1.0f * (1 / (UPDATE_INTERVAL * 1.0f / 1000)))
#define OUT_OF_BOUNDS_THRESHOLD (2.0f * (1 / (UPDATE_INTERVAL * 1.0f / 1000)))

static void clear_between_pitch_state(BetweenPitchState* bps)
{
    bps->catchHasBeenMade = 0;
    bps->hasBallHitGround = 0;
    bps->pitchResult = PITCH_RESULT_NONE;
    bps->batOutcome = BAT_OUTCOME_NONE;
}

// ============================================================================
// Referee Update Pipeline (Milestone 15)
// ============================================================================

static void update_initialization_events(
    const StateInfo* stateInfo, RefereeState* referee, const GameEvents* events, BetweenPitchState* betweenPitchState,
    HalfInningState* halfInningState, Scoreboard* scoreboard
)
{
    const MatchSession* game = stateInfo->match;

    // Batter Entered: Initialize safety for the new batter and reset count
    if (events->batterEntered) {
        int battingTeamIndex = get_batting_team_index(scoreboard);
        for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
            if (game->playerInfo[i].bTPI.state == PLAYER_STATE_AT_BAT) {
                referee->battingPlayers[i].currentSafetyBase = BASE_HOME;
                // Advance batting order only for regular (non-joker) batters.
                // Note: joker status is JOKER_USED by this point (batting_system sets it
                // before firing the event), so we check for JOKER_REGULAR specifically.
                if (game->playerInfo[i].bTPI.joker == JOKER_REGULAR) {
                    scoreboard->teams[battingTeamIndex].batterOrderIndex =
                        (scoreboard->teams[battingTeamIndex].batterOrderIndex + 1) % PLAYERS_IN_TEAM;
                }
            }
        }
        // Reset strikes and balls for the new batter
        halfInningState->strikes = 0;
        halfInningState->balls = 0;
    }

    // Pitch Released: Snapshot state for the new pitch
    if (events->pitchReleased) {
        // 1. Reset Sticky Flags
        clear_between_pitch_state(betweenPitchState);
        referee->woundingEvaluationFinished = 0;
        referee->woundingEvaluationActive = 0;
        referee->woundingEvaluationTimer = -1;
        referee->foulTimer = -1;
        referee->foulState = FOUL_STATE_NONE;
        referee->ballInThirdBaseSincePitch = 0;

        // 2. Mark that a pitch has been released (for homerun contest pair tracking)
        if (stateInfo->match->scoreboard.period >= 4) {
            referee->homerunPairHasPitch = 1;
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
                        if (get_base_controller(game, (BaseID)base) == index) {
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

    // RESETTING is handled by referee_finalize (post-consolidation stage).
    // Skip all foul logic this frame — consolidation is doing the physical reset.
    if (referee->foulState == FOUL_STATE_RESETTING) {
        return;
    }

    // Transition 2: DETECTED -> RESETTING (Grace period expired)
    if (referee->foulState == FOUL_STATE_DETECTED) {
        referee->foulTimer++;
        if (referee->foulTimer > (int)OUT_OF_BOUNDS_THRESHOLD) {
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
        }
        return;
    }

    // Transition 1: NONE -> DETECTED (Detection)
    // Out of Bounds Logic: Check ONLY on first bounce
    if (events->ballHitGround && betweenPitchState->hasBallHitGround == 0 && referee->foulState == FOUL_STATE_NONE) {
        if (betweenPitchState->batOutcome == BAT_OUTCOME_HIT && betweenPitchState->catchHasBeenMade == 0) {
            if (checkIfBallIsOutOfBounds(&game->ballInfo, stateInfo->fieldPositions)) {

                // 1. Immediately apply the consequence of the foul (Strike/Out)
                if (referee->strikesAtPitchStart == 2) {
                    halfInningState->outs += 1;

                    // Mark batter as OUT immediately
                    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
                        if (referee->battingPlayers[i].baseAtPitchStart == BASE_HOME) {
                            referee->battingPlayers[i].status = PLAYER_STATUS_OUT;
                            referee->battingPlayers[i].currentSafetyBase = BASE_NONE;
                            referee->battingPlayers[i].baseAtPitchStart = BASE_NONE;
                            break;
                        }
                    }
                }

                // 2. Always transition to DETECTED.
                // If this foul caused the inning to end, the Decide phase later in
                // update_referee will instantly upgrade this and abort the foul timer.
                referee->foulState = FOUL_STATE_DETECTED;
                referee->foulTimer = 0;

                // Trigger global OUT_OF_BOUNDS event (or OUT if we struck out)
                if (halfInningState->event == EVENT_NONE) {
                    if (referee->strikesAtPitchStart == 2) {
                        halfInningState->event = EVENT_OUT;
                    } else {
                        halfInningState->event = EVENT_OUT_OF_BOUNDS;
                    }
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
    if (events->catchMade && betweenPitchState->batOutcome == BAT_OUTCOME_HIT &&
        betweenPitchState->hasBallHitGround == 0 && betweenPitchState->catchHasBeenMade == 0) {

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
        // Don't mark new pending runs if end-of-inning has been decided
        if ((isBallInAir || isCatchPending) && referee->endOfInningState == END_INNING_STATE_NONE) {
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
                        int battingTeamIndex = get_batting_team_index(scoreboard);
                        scoreboard->teams[battingTeamIndex].runs += 1;
                        halfInningState->runsInTheInning += 1;

                        referee->battingPlayers[i].hasScored = 1;

                        if (halfInningState->runsInTheInning % 2 == 0) {
                            playerCounters->nonJokerPlayersLeft = PLAYERS_IN_TEAM;
                        }
                    } else if (runOfHonor) {
                        halfInningState->event = EVENT_RUN_SCORED;
                        int battingTeamIndex = get_batting_team_index(scoreboard);
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
            int battingTeamIndex = get_batting_team_index(scoreboard);
            int catchingTeamIndex = (battingTeamIndex + 1) % 2;
            if (should_period_end(
                    scoreboard, scoreboard->teams[battingTeamIndex].runs, scoreboard->teams[catchingTeamIndex].runs,
                    scoreboard->teams[battingTeamIndex].period0Runs, scoreboard->teams[catchingTeamIndex].period0Runs
                )) {
                halfInningState->endPeriod = 1;
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
    const GameEvents* events
)
{
    // Check if a pitch has physically concluded (hit ground) while still logically active
    if (events->ballHitGround && stateInfo->match->pRAI.pitchState != PITCH_STAGE_NONE) {

        if (betweenPitchState->batOutcome == BAT_OUTCOME_NONE) {
            // No swing: resolve by ball position (strike zone check)
            PitchResult result = determine_pitch_result(stateInfo->match->ballInfo.location.x, PLATE_WIDTH);

            if (result == PITCH_RESULT_STRIKE) {
                halfInningState->strikes += 1;
                halfInningState->event = EVENT_STRIKE;
            } else if (result == PITCH_RESULT_BALL) {
                halfInningState->balls += 1;
                halfInningState->event = EVENT_BALL;
            }

            betweenPitchState->pitchResult = result;
        } else {
            // Swing happened: strike already counted by update_strikes, just signal resolution
            betweenPitchState->pitchResult = PITCH_RESULT_STRIKE;
        }
    }
}

static void update_free_walk_resolution(
    const StateInfo* stateInfo, RefereeState* referee, HalfInningState* halfInningState, PlayerCounters* playerCounters,
    Scoreboard* scoreboard, const FlowControl* flowControl, const GameEvents* events
)
{
    // No free walk processing once end-of-inning has been decided
    if (referee->endOfInningState != END_INNING_STATE_NONE) return;

    // 6. Free Walk Resolution
    if (events->freeWalkAccepted && flowControl->freeWalkIndex != -1) {
        int i = flowControl->freeWalkIndex;
        BaseID sourceBase = flowControl->freeWalkBase;
        int battingTeamIndex = get_batting_team_index(scoreboard);
        int catchingTeamIndex = (battingTeamIndex + 1) % 2;

        if (scoreboard->period >= 4) {
            // Homerun Contest / Super Inning Logic
            referee->battingPlayers[i].baseAtPitchStart = BASE_HOME_SCORED;
            referee->battingPlayers[i].currentSafetyBase = BASE_HOME_SCORED;
            referee->battingPlayers[i].hasScored = 1;

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
            if (should_period_end(
                    scoreboard, scoreboard->teams[battingTeamIndex].runs, scoreboard->teams[catchingTeamIndex].runs,
                    scoreboard->teams[battingTeamIndex].period0Runs, scoreboard->teams[catchingTeamIndex].period0Runs
                )) {
                halfInningState->endPeriod = 1;
            }

        } else {
            // Normal Game Logic
            if (sourceBase != BASE_THIRD) {
                // Advance to next base
                BaseID targetBase = base_get_next(sourceBase);
                referee->battingPlayers[i].baseAtPitchStart = targetBase;
                referee->battingPlayers[i].currentSafetyBase = targetBase;
            } else {
                // Score from 3rd base — no physical run, player is removed by enforceLegalState
                referee->battingPlayers[i].baseAtPitchStart = BASE_HOME_SCORED;
                referee->battingPlayers[i].currentSafetyBase = BASE_HOME_SCORED;
                referee->battingPlayers[i].hasScored = 1;

                // Add run
                scoreboard->teams[battingTeamIndex].runs += 1;
                halfInningState->runsInTheInning += 1;

                if (halfInningState->runsInTheInning % 2 == 0) {
                    playerCounters->nonJokerPlayersLeft = PLAYERS_IN_TEAM;
                    playerCounters->noMorePlayers = 0;
                }
                halfInningState->event = EVENT_RUN_SCORED;

                // Period End Check
                if (should_period_end(
                        scoreboard, scoreboard->teams[battingTeamIndex].runs, scoreboard->teams[catchingTeamIndex].runs,
                        scoreboard->teams[battingTeamIndex].period0Runs,
                        scoreboard->teams[catchingTeamIndex].period0Runs
                    )) {
                    halfInningState->endPeriod = 1;
                }
            }
        }

        // Reset balls so free walk is not re-offered on the next BALL pitch
        halfInningState->balls = 0;
    }
}

static void resolve_pending_runs(
    const StateInfo* stateInfo, RefereeState* referee, HalfInningState* halfInningState,
    BetweenPitchState* betweenPitchState, PlayerCounters* playerCounters, Scoreboard* scoreboard
)
{
    // No run resolution once end-of-inning has been decided
    if (referee->endOfInningState != END_INNING_STATE_NONE) return;

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
                    int battingTeamIndex = get_batting_team_index(scoreboard);
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
                    int battingTeamIndex = get_batting_team_index(scoreboard);
                    scoreboard->teams[battingTeamIndex].runs += 1;
                    halfInningState->runsInTheInning += 1;

                    referee->battingPlayers[i].runOfHonorScored = 1;
                    referee->battingPlayers[i].hasPendingRunOfHonor = 0;
                }
            }

            // Period End Check (Bug #1 fix: was missing from resolve_pending_runs)
            int battingTeamIndex = get_batting_team_index(scoreboard);
            int catchingTeamIndex = (battingTeamIndex + 1) % 2;
            if (should_period_end(
                    scoreboard, scoreboard->teams[battingTeamIndex].runs, scoreboard->teams[catchingTeamIndex].runs,
                    scoreboard->teams[battingTeamIndex].period0Runs, scoreboard->teams[catchingTeamIndex].period0Runs
                )) {
                halfInningState->endPeriod = 1;
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
                    int battingTeamIndex = get_batting_team_index(scoreboard);
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

        // Period End Check (Bug #1 fix: was missing from resolve_pending_runs)
        int battingTeamIndex = get_batting_team_index(scoreboard);
        int catchingTeamIndex = (battingTeamIndex + 1) % 2;
        if (should_period_end(
                scoreboard, scoreboard->teams[battingTeamIndex].runs, scoreboard->teams[catchingTeamIndex].runs,
                scoreboard->teams[battingTeamIndex].period0Runs, scoreboard->teams[catchingTeamIndex].period0Runs
            )) {
            halfInningState->endPeriod = 1;
        }
    }
}

static void update_game_state_flags(
    const StateInfo* stateInfo, RefereeState* referee, HalfInningState* halfInningState, const GameEvents* events,
    BetweenPitchState* betweenPitchState
)
{
    // 6. Game State Flags

    // Persistent Catch State Management
    // When a catch is made, we lock it in.
    if (events->catchMade) {
        betweenPitchState->catchHasBeenMade = 1;
    }

    // Batting Outcome Promotion
    // Transient events → sticky flag, same pattern as catchMade → catchHasBeenMade
    if (events->ballHitByBat) {
        betweenPitchState->batOutcome = BAT_OUTCOME_HIT;
    }
    if (events->ballMissedByBat) {
        betweenPitchState->batOutcome = BAT_OUTCOME_MISSED;
    }
}

// Per-pair clear for homerun contest next-pair transition.
// Called by the referee itself at DETECTED→RESETTING for next-pair.
// Clears per-pitch and per-pair state. Preserves outs, runsInTheInning (inning-level state).
static void clear_referee_for_pair_end(
    RefereeState* refereeState, HalfInningState* halfInningState, BetweenPitchState* betweenPitchState
)
{
    halfInningState->strikes = 0;
    halfInningState->balls = 0;
    clear_between_pitch_state(betweenPitchState);
    refereeState->foulState = FOUL_STATE_NONE;
    refereeState->foulTimer = -1;
    refereeState->homerunPairHasPitch = 0;

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

// Full clear of ALL referee-owned state for a new inning.
// Called by the referee itself at DETECTED→RESETTING for end-of-inning.
// Clears everything EXCEPT endOfInningState/endInningTimer (the active handshake signal).
static void clear_referee_for_inning_end(
    RefereeState* refereeState, HalfInningState* halfInningState, BetweenPitchState* betweenPitchState
)
{
    // HalfInningState (all referee-owned fields)
    halfInningState->outs = 0;
    halfInningState->strikes = 0;
    halfInningState->balls = 0;
    halfInningState->runsInTheInning = 0;
    halfInningState->event = EVENT_NONE;
    halfInningState->endPeriod = 0;

    // BetweenPitchState
    clear_between_pitch_state(betweenPitchState);

    // Player tracking
    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        refereeState->battingPlayers[i].baseAtPitchStart = BASE_NONE;
        refereeState->battingPlayers[i].currentSafetyBase = BASE_NONE;
        refereeState->battingPlayers[i].status = PLAYER_STATUS_ACTIVE;
        refereeState->battingPlayers[i].hasScored = 0;
        refereeState->battingPlayers[i].runOfHonorScored = 0;
        refereeState->battingPlayers[i].baseAtLastEvent = BASE_NONE;
        refereeState->battingPlayers[i].hadSafetyAtLastEvent = 0;
        refereeState->battingPlayers[i].hasPendingRun = 0;
        refereeState->battingPlayers[i].hasPendingRunOfHonor = 0;
    }

    // Referee internal state
    refereeState->strikesAtPitchStart = 0;
    refereeState->woundingEvaluationActive = 0;
    refereeState->woundingEvaluationFinished = 0;
    refereeState->woundingEvaluationTimer = -1;
    refereeState->ballInThirdBaseSincePitch = 0;
    refereeState->homerunPairHasPitch = 0;

    // Sub-state machines (foul + next-pair are done if inning is ending)
    refereeState->foulState = FOUL_STATE_NONE;
    refereeState->foulTimer = -1;
    refereeState->nextPairTransitionState = HR_PAIR_STATE_NONE;
    refereeState->nextPairTimer = -1;
    // NOTE: endOfInningState and endInningTimer are NOT cleared here.
    // They remain RESETTING/-1 for the two-frame handshake with consolidation.
}

void referee_reset_for_new_inning(RefereeState* ref, HalfInningState* his, BetweenPitchState* bps)
{
    initializeRefereeState(ref); // existing: clears all player tracking + state machines

    his->outs = 0;
    his->balls = 0;
    his->strikes = 0;
    his->runsInTheInning = 0;
    his->event = EVENT_NONE;
    his->endPeriod = 0;

    clear_between_pitch_state(bps);
}

// Advance the scoreboard at end-of-inning and determine what period transition occurs.
// Called by referee at DETECTED→RESETTING time. Consolidation reads periodTransition
// to know what menu/screen action to take.
static void advance_scoreboard_for_inning_end(RefereeState* refereeState, Scoreboard* scoreboard)
{
    int battingTeamIndex = get_batting_team_index(scoreboard);
    int catchingTeamIndex = (battingTeamIndex + 1) % 2;

    scoreboard->inning++;

    // First period ending
    if (scoreboard->inning == scoreboard->halfInningsInPeriod ||
        (scoreboard->inning == scoreboard->halfInningsInPeriod - 1 &&
         scoreboard->teams[catchingTeamIndex].runs > scoreboard->teams[battingTeamIndex].runs)) {
        scoreboard->period = 1;
        for (int i = 0; i < 2; i++) {
            scoreboard->teams[i].period0Runs = scoreboard->teams[i].runs;
            scoreboard->teams[i].runs = 0;
        }
        if (scoreboard->inning == scoreboard->halfInningsInPeriod - 1) {
            scoreboard->inning++; // skip the last half-inning
        }
        refereeState->periodTransition = PERIOD_TRANSITION_INTER_PERIOD;
    }
    // Second period ending
    else if (scoreboard->inning == scoreboard->halfInningsInPeriod * 2 ||
             (scoreboard->inning == scoreboard->halfInningsInPeriod * 2 - 1 &&
              (scoreboard->teams[catchingTeamIndex].runs > scoreboard->teams[battingTeamIndex].runs ||
               (scoreboard->teams[catchingTeamIndex].period0Runs > scoreboard->teams[battingTeamIndex].period0Runs &&
                scoreboard->teams[catchingTeamIndex].runs == scoreboard->teams[battingTeamIndex].runs)))) {
        for (int i = 0; i < 2; i++) {
            scoreboard->teams[i].period1Runs = scoreboard->teams[i].runs;
        }

        int t0p0 = scoreboard->teams[0].period0Runs;
        int t0p1 = scoreboard->teams[0].period1Runs;
        int t1p0 = scoreboard->teams[1].period0Runs;
        int t1p1 = scoreboard->teams[1].period1Runs;

        if (t0p0 >= t1p0 && t0p1 >= t1p1 && (t0p0 != t1p0 || t0p1 != t1p1)) {
            refereeState->periodTransition = PERIOD_TRANSITION_GAME_OVER;
            refereeState->periodTransitionWinner = 0;
        } else if (t0p0 <= t1p0 && t0p1 <= t1p1 && (t0p0 != t1p0 || t0p1 != t1p1)) {
            refereeState->periodTransition = PERIOD_TRANSITION_GAME_OVER;
            refereeState->periodTransitionWinner = 1;
        } else {
            scoreboard->period = 2;
            refereeState->periodTransition = PERIOD_TRANSITION_SUPER_INNING;
        }

        if (scoreboard->inning == scoreboard->halfInningsInPeriod * 2 - 1) {
            scoreboard->inning++; // skip the last half-inning
        }
        for (int i = 0; i < 2; i++) {
            scoreboard->teams[i].runs = 0;
        }
    }
    // Super inning ending
    else if (scoreboard->inning == scoreboard->halfInningsInPeriod * 2 + 2) {
        for (int i = 0; i < 2; i++) {
            scoreboard->teams[i].period2Runs = scoreboard->teams[i].runs;
        }

        if (scoreboard->teams[0].runs > scoreboard->teams[1].runs) {
            refereeState->periodTransition = PERIOD_TRANSITION_GAME_OVER;
            refereeState->periodTransitionWinner = 0;
        } else if (scoreboard->teams[0].runs < scoreboard->teams[1].runs) {
            refereeState->periodTransition = PERIOD_TRANSITION_GAME_OVER;
            refereeState->periodTransitionWinner = 1;
        } else {
            // Super Inning → Homerun Contest
            scoreboard->period = 4;
            refereeState->periodTransition = PERIOD_TRANSITION_HOMERUN_CONTEST;
        }

        for (int i = 0; i < 2; i++) {
            scoreboard->teams[i].runs = 0;
        }
    }
    // Homerun contest round ending
    else if (scoreboard->period >= 4 && (scoreboard->inning) % 2 == 0) {
        for (int i = 0; i < 2; i++) {
            scoreboard->teams[i].period3Runs += scoreboard->teams[i].runs;
        }

        if (scoreboard->teams[0].period3Runs > scoreboard->teams[1].period3Runs) {
            refereeState->periodTransition = PERIOD_TRANSITION_GAME_OVER;
            refereeState->periodTransitionWinner = 0;
        } else if (scoreboard->teams[0].period3Runs < scoreboard->teams[1].period3Runs) {
            refereeState->periodTransition = PERIOD_TRANSITION_GAME_OVER;
            refereeState->periodTransitionWinner = 1;
        } else {
            // Next homerun contest round (period += 2 for battingTeamIndex calculation)
            scoreboard->period += 2;
            refereeState->periodTransition = PERIOD_TRANSITION_HOMERUN_CONTEST;
        }

        for (int i = 0; i < 2; i++) {
            scoreboard->teams[i].runs = 0;
        }
    }
    // Normal next-inning (no period transition)
    else {
        refereeState->periodTransition = PERIOD_TRANSITION_NONE;
    }
}

void update_referee(
    const StateInfo* stateInfo, RefereeState* refereeState, HalfInningState* halfInningState,
    BetweenPitchState* betweenPitchState, PlayerCounters* playerCounters, Scoreboard* scoreboard,
    HomeRunContestState* homeRunContestState
)
{
    const MatchSession* game = stateInfo->match;
    const FlowControl* flowControl = &game->flowControl; // Read-only access to flow data

    // 0. Initialization Events (Milestone 17)
    update_initialization_events(
        stateInfo, refereeState, &stateInfo->match->gameEvents, betweenPitchState, halfInningState, scoreboard
    );

    // 1. Where is the ball?
    int ballAtBase = get_ball_at_base_index(stateInfo->match, stateInfo->fieldPositions);

    // 2. Track if ball has been at 3rd base since pitch started (for run of honor logic)
    if (ballAtBase == 3) {
        refereeState->ballInThirdBaseSincePitch = 1;
    }

    // 2.5 Foul Play & Wounding Logic (Milestone 17)
    update_foul_play_logic(stateInfo, refereeState, halfInningState, &stateInfo->match->gameEvents, betweenPitchState);

    // If foul play reset is triggered, the physical world is out of sync with legal state.
    // We skip gameplay logic this frame, but we still run Flow Control (Gather-Decide-Act)
    if (refereeState->foulState != FOUL_STATE_RESETTING) {
        // Mark that ball hit ground (after foul play check, so it can detect first bounce)
        if (stateInfo->match->gameEvents.ballHitGround) {
            betweenPitchState->hasBallHitGround = 1;
        }

        update_wounding_logic(
            stateInfo, refereeState, halfInningState, &stateInfo->match->gameEvents, betweenPitchState
        );

        // 3. Safety Pipeline
        update_safety_status(stateInfo, refereeState);
        update_force_outs(stateInfo, refereeState, halfInningState, ballAtBase, betweenPitchState);
        update_tuplahaava_logic(stateInfo, refereeState, halfInningState, ballAtBase);
        update_runs(stateInfo, refereeState, halfInningState, betweenPitchState, playerCounters, scoreboard);

        // 3.5 Resolve Pending Runs (Milestone 17)
        resolve_pending_runs(stateInfo, refereeState, halfInningState, betweenPitchState, playerCounters, scoreboard);

        // 4. Strikes
        update_strikes(refereeState, halfInningState, &stateInfo->match->gameEvents);
        update_pitch_resolution(stateInfo, halfInningState, betweenPitchState, &stateInfo->match->gameEvents);
        update_free_walk_resolution(
            stateInfo, refereeState, halfInningState, playerCounters, scoreboard, flowControl,
            &stateInfo->match->gameEvents
        );

        // 5. Game State Flags
        update_game_state_flags(
            stateInfo, refereeState, halfInningState, &stateInfo->match->gameEvents, betweenPitchState
        );
    }

    // ========================================================================
    // PHASE A: GATHER (Read-only evaluation of flow rules)
    // ========================================================================
    int wants_pair_end = 0;

    if (scoreboard->period >= 4) {
        if (refereeState->nextPairTransitionState == HR_PAIR_STATE_NONE) {
            // Only check pair-ending conditions after at least one pitch has been released
            if (refereeState->homerunPairHasPitch) {
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

                int ballHome = game->gameFlowState.ballHome;
                BaseID batterBaseId = BASE_NONE;
                PlayerUnitState batterState = PLAYER_STATE_IDLE;

                if (batterIndex != -1) {
                    batterBaseId = game->playerInfo[batterIndex].bTPI.baseId;
                    batterState = game->playerInfo[batterIndex].bTPI.state;
                }

                BaseID runnerBaseId = BASE_NONE;
                if (runnerIndex != -1) {
                    runnerBaseId = game->playerInfo[runnerIndex].bTPI.baseId;
                }

                int batterHasSafetyAtHome = 0;
                if (batterIndex != -1 && refereeState->battingPlayers[batterIndex].currentSafetyBase == BASE_HOME) {
                    batterHasSafetyAtHome = 1;
                }

                int batterAdvancingToThird = 0;
                if (batterIndex != -1 && batterBaseId == BASE_SECOND && batterState != PLAYER_STATE_ON_BASE) {
                    batterAdvancingToThird = 1;
                }

                if (runnerBaseId == BASE_THIRD && ballHome) {
                    if (!batterHasSafetyAtHome && !batterAdvancingToThird) wants_pair_end = 1;
                }

                if (runnerBaseId != BASE_THIRD && ballHome) {
                    if (!batterAdvancingToThird) wants_pair_end = 1;
                }

                if (ballHome) {
                    int anyoneHasSafety = 0;
                    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
                        if (refereeState->battingPlayers[i].currentSafetyBase != BASE_NONE) {
                            anyoneHasSafety = 1;
                            break;
                        }
                    }
                    if (!anyoneHasSafety) wants_pair_end = 1;
                }
            }
        }
    }

    int wants_inning_end = 0;
    if (refereeState->endOfInningState == END_INNING_STATE_NONE) {
        if (scoreboard->period >= 4) {
            wants_inning_end = (halfInningState->endPeriod == 1) ||
                               (homeRunContestState->runnerBatterPairCounter >= scoreboard->pairCount);
        } else {
            wants_inning_end = (halfInningState->outs >= 3) ||
                               (playerCounters->noMorePlayers == 1 && game->gameFlowState.ballHome == 1) ||
                               (halfInningState->endPeriod == 1);
        }
    }

    // ========================================================================
    // PHASE B: DECIDE (Priority & Upgrades)
    // ========================================================================

    // Upgrade 1: Uncatchable HR Pair OR Last Pair -> Inning End
    if (wants_pair_end) {
        int nextPairCounter = homeRunContestState->runnerBatterPairCounter + 1;
        int pairsLeft = scoreboard->pairCount - nextPairCounter;

        int battingTeamIndex = get_batting_team_index(scoreboard);
        int catchingTeamIndex = (battingTeamIndex + 1) % 2;
        int battingRuns = scoreboard->teams[battingTeamIndex].runs;
        int catchingRuns = scoreboard->teams[catchingTeamIndex].runs;

        // If this is the last pair, or uncatchable, the inning ends directly
        if (nextPairCounter >= scoreboard->pairCount ||
            ((scoreboard->inning + 1) % 2 == 0 && pairsLeft * 2 + battingRuns < catchingRuns)) {
            wants_inning_end = 1;
            wants_pair_end = 0; // Subsumed
            // Increment the counter since we bypassed the pair state machine
            homeRunContestState->runnerBatterPairCounter = nextPairCounter;
        }
    }

    // Mutual Exclusivity: Inning End suppresses Pair Ends and active Foul timers
    if (wants_inning_end || refereeState->endOfInningState != END_INNING_STATE_NONE) {
        wants_pair_end = 0;

        if (refereeState->nextPairTransitionState != HR_PAIR_STATE_NONE) {
            refereeState->nextPairTransitionState = HR_PAIR_STATE_NONE;
            refereeState->nextPairTimer = -1;
        }

        if (refereeState->foulState != FOUL_STATE_NONE) {
            refereeState->foulState = FOUL_STATE_NONE;
            refereeState->foulTimer = -1;
        }
    }

    // ========================================================================
    // PHASE C: ACT (State Machine Updates)
    // ========================================================================

    // 6. Homerun Contest Pair State Machine
    if (scoreboard->period >= 4) {
        HomeRunPairState currentState = refereeState->nextPairTransitionState;

        if (currentState == HR_PAIR_STATE_NONE) {
            if (wants_pair_end) {
                refereeState->nextPairTransitionState = HR_PAIR_STATE_DETECTED;
                refereeState->nextPairTimer = 0;

                // Milestone 18: Referee applies rule logic immediately at DETECTED
                homeRunContestState->runnerBatterPairCounter++;

                if (refereeState->endOfInningState == END_INNING_STATE_NONE) {
                    halfInningState->event = EVENT_NEXT_PAIR;
                }
            }
        } else if (currentState == HR_PAIR_STATE_DETECTED) {
            refereeState->nextPairTimer++;
            if (refereeState->nextPairTimer > 200) {
                refereeState->nextPairTransitionState = HR_PAIR_STATE_RESETTING;
                refereeState->nextPairTimer = -1;

                // Per-pair legal state cleanup
                clear_referee_for_pair_end(refereeState, halfInningState, betweenPitchState);
            }
        } else if (currentState == HR_PAIR_STATE_RESETTING) {
            // Handled by referee_finalize (post-consolidation stage).
            // Nothing to do here — consolidation is performing the physical reset this frame.
        }
    }

    // 7. End of Inning State Machine
    EndOfInningTransitionState endState = refereeState->endOfInningState;

    if (endState == END_INNING_STATE_NONE) {
        if (wants_inning_end) {
            refereeState->endOfInningState = END_INNING_STATE_DETECTED;
            refereeState->endInningTimer = 0;
            halfInningState->event = EVENT_INNING_ENDING;
        }
    } else if (endState == END_INNING_STATE_DETECTED) {
        refereeState->endInningTimer++;
        if (refereeState->endInningTimer > 200) {
            refereeState->endOfInningState = END_INNING_STATE_RESETTING;
            refereeState->endInningTimer = -1;
            clear_referee_for_inning_end(refereeState, halfInningState, betweenPitchState);
            advance_scoreboard_for_inning_end(refereeState, scoreboard);
        }
    } else if (endState == END_INNING_STATE_RESETTING) {
        // Handled by referee_finalize (post-consolidation stage).
        // Nothing to do here — consolidation is performing the physical reset this frame.
    }
}

// ============================================================================
// Referee Finalize (Post-Consolidation Stage)
//
// Runs AFTER consolidation has performed any physical resets.
// Handles RESETTING→NONE transitions: clears state machine flags and scans the
// newly-reset physical world to establish legal tracking for the next cycle.
//
// This eliminates the old one-frame delay where RESETTING→NONE was processed at
// the TOP of update_referee in the NEXT frame.
// ============================================================================

void referee_finalize(const StateInfo* stateInfo, RefereeState* refereeState, BetweenPitchState* betweenPitchState)
{
    const MatchSession* game = stateInfo->match;

    // Foul play: RESETTING → NONE
    if (refereeState->foulState == FOUL_STATE_RESETTING) {
        refereeState->foulState = FOUL_STATE_NONE;
        clear_between_pitch_state(betweenPitchState);
    }

    // HR pair: RESETTING → NONE + scan physical world for new pair
    if (refereeState->nextPairTransitionState == HR_PAIR_STATE_RESETTING) {
        refereeState->nextPairTransitionState = HR_PAIR_STATE_NONE;

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

    // End of inning: RESETTING → NONE + scan physical world for new players
    if (refereeState->endOfInningState == END_INNING_STATE_RESETTING) {
        refereeState->endOfInningState = END_INNING_STATE_NONE;
        refereeState->periodTransition = PERIOD_TRANSITION_NONE;

        for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
            if (game->playerInfo[i].bTPI.state == PLAYER_STATE_AT_BAT) {
                refereeState->battingPlayers[i].currentSafetyBase = BASE_HOME;
            } else if (game->playerInfo[i].bTPI.state == PLAYER_STATE_ON_BASE &&
                       game->playerInfo[i].bTPI.baseId != BASE_NONE) {
                refereeState->battingPlayers[i].currentSafetyBase = game->playerInfo[i].bTPI.baseId;
            }
        }
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

void initialize_referee(const StateInfo* stateInfo, RefereeState* referee)
{
    const MatchSession* game = stateInfo->match;

    // Full reset first
    initializeRefereeState(referee);

    // Scan physical world and initialize safety based on player positions
    if (game->scoreboard.period >= 4) {
        // Homerun Contest (period >= 4): scan for batter at HOME, runner at THIRD
        // Reset homerun pair pitch tracking
        referee->homerunPairHasPitch = 0;

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
    referee->periodTransition = PERIOD_TRANSITION_NONE;
    referee->periodTransitionWinner = -1;
    referee->homerunPairHasPitch = 0;
}
