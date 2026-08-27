#include "game_consolidation.h"
#include "common_logic.h"
#include "game_manipulation.h"
#include "game_setup.h" // For setRunnerAndBatter and initialization helpers
#include "base_logic.h"
#include "base_control.h"
#include "rules_pure/player_utils.h"
#include "rules_pure/rules_strikes.h"
#include "execute_actions.h"
#include "game_reset.h"
#include "referee.h"

// ===============================================================================================
// FORWARD DECLARATIONS
// ===============================================================================================

static void enforce_legal_state(
    MatchSession* match, const FieldPositions* field_positions, const RefereeState* referee,
    const BetweenPitchState* bps
);
static int handle_foul_play_reset(MatchSession* match, const FieldPositions* field_positions, GameRulesState* rules);
static void
update_game_flow(MatchSession* match, const FieldPositions* field_positions, GameRulesState* rules, MenuInfo* menuInfo);

static void check_next_batter_decision(MatchSession* match, const GameRulesState* rules);
static void apply_batter_becomes_runner(
    MatchSession* match, const FieldPositions* field_positions, const HalfInningState* his, const RefereeState* referee
);
static void handle_free_walk_offers(MatchSession* match, const HalfInningState* his, const RefereeState* referee);
static int check_end_of_inning(
    MatchSession* match, const FieldPositions* field_positions, const TeamData* team_data,
    GameConclusion* game_conclusion, GameRulesState* rules, MenuInfo* menuInfo, ConsolidationOutput* output
);
static int check_next_pair(MatchSession* match, const FieldPositions* field_positions, GameRulesState* rules);
static void populate_game_conclusion(GameConclusion* game_conclusion, const Scoreboard* scoreboard, int winner);

// ===============================================================================================
// PUBLIC API
// ===============================================================================================

void consolidation_init(GameFlowState* gameFlowState)
{
    gameFlowState->closeToGround = 0;
    gameFlowState->homeRunCameraCounter = -1;
}

void consolidation_update(
    MatchSession* match, const FieldPositions* field_positions, const TeamData* team_data,
    GameConclusion* game_conclusion, GameRulesState* rules, MenuInfo* menuInfo, ConsolidationOutput* output
)
{
    const RefereeState* referee = &rules->referee;
    const BetweenPitchState* bps = &rules->betweenPitchState;

    // Initialize output
    output->request_screen_change = 0;
    output->target_screen = SCREEN_GAME;

    // 0. Check for Physical Resets FIRST (these reset the world)
    // If any reset happens, abort all further processing for this frame

    // Check for end-of-inning reset
    if (check_end_of_inning(match, field_positions, team_data, game_conclusion, rules, menuInfo, output)) {
        return;
    }

    // Check for next pair reset (homerun contest)
    if (check_next_pair(match, field_positions, rules)) {
        return;
    }

    // Check for foul play (out of bounds) reset
    if (handle_foul_play_reset(match, field_positions, rules)) {
        return;
    }

    // 1. Game Flow Analysis (The Game Master)
    // Decides if we need to pause for input, etc.
    update_game_flow(match, field_positions, rules, menuInfo);

    // 2. State Enforcement (The Enforcer)
    // Ensures physical entities obey legal outcomes (Outs, Scores, Safety).
    enforce_legal_state(match, field_positions, referee, bps);
}

// ===============================================================================================
// INTERNAL: PHYSICAL ENFORCEMENT (formerly reconcileLegalAndPhysicalState)
// ===============================================================================================

static void enforce_legal_state(
    MatchSession* match, const FieldPositions* field_positions, const RefereeState* referee,
    const BetweenPitchState* bps
)
{
    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        // 1. React to OUT
        if (referee->battingPlayers[i].status == PLAYER_STATUS_OUT) {
            if (match->playerInfo[i].bTPI.state != PLAYER_STATE_OUT) {
                match->playerInfo[i].bTPI.state = PLAYER_STATE_OUT;
                match->playerInfo[i].bTPI.baseId = BASE_NONE;
                move_player_out(match->playerInfo, match->playerRuntime, field_positions, i);
            }
        }

        // 2. React to SCORE
        if (referee->battingPlayers[i].hasScored && match->playerInfo[i].bTPI.state != PLAYER_STATE_SCORED) {
            match->playerInfo[i].bTPI.state = PLAYER_STATE_SCORED;
            match->playerInfo[i].bTPI.baseId = BASE_NONE;
            move_player_out(match->playerInfo, match->playerRuntime, field_positions, i);
        }

        // 3. React to WOUNDED
        if (referee->battingPlayers[i].status == PLAYER_STATUS_WOUNDED) {
            if (match->playerInfo[i].bTPI.state != PLAYER_STATE_WOUNDED) {
                match->playerInfo[i].bTPI.state = PLAYER_STATE_WOUNDED;
                match->playerInfo[i].bTPI.baseId = BASE_NONE;
                move_player_out(match->playerInfo, match->playerRuntime, field_positions, i);
            }
        }

        // 4. React to displacement (Panic Run)
        if (match->playerInfo[i].bTPI.state == PLAYER_STATE_ON_BASE ||
            match->playerInfo[i].bTPI.state == PLAYER_STATE_LEADING) {
            BaseID physBase = match->playerInfo[i].bTPI.baseId;
            if (referee->battingPlayers[i].currentSafetyBase != physBase) {
                int isWounded = (referee->battingPlayers[i].status == PLAYER_STATUS_WOUNDED);

                if (!isWounded) {
                    // Player is physically at base but legally has no safety there.
                    // They must run forward.
                    run_to_next_base(match, field_positions, i, physBase);
                }
            }
        }
    }

    // 5. React to Pitch Resolution
    if (bps->pitchResult != PITCH_RESULT_NONE && match->pRAI.pitch_state != PITCH_STAGE_NONE) {
        match->pRAI.pitch_state = PITCH_STAGE_NONE;
        // On ball: reset free walk calculation so it is re-evaluated
        if (bps->pitchResult == PITCH_RESULT_BALL) {
            match->flowControl.freeWalkCalculationMade = 0;
            match->flowControl.freeWalkIndex = -1;
            match->flowControl.freeWalkBase = BASE_NONE;
        }
    }
}

// ===============================================================================================
// INTERNAL: FOUL PLAY RESET
// ===============================================================================================

static int handle_foul_play_reset(MatchSession* match, const FieldPositions* field_positions, GameRulesState* rules)
{
    // Check if Referee has triggered the reset state
    if (rules->referee.foulState == FOUL_STATE_RESETTING) {
        reset_for_foul_play(match, field_positions, rules);

        // Note: Referee will transition state to NONE in referee_finalize
        return 1; // Signal that we reset
    }
    return 0; // No reset
}

// Formerly applyFoulPlayReset in game_setup.c
// Now handled by game_reset.c: reset_for_foul_play

// ===============================================================================================
// INTERNAL: GAME FLOW (formerly game_analysis)
// ===============================================================================================

static void
update_game_flow(MatchSession* match, const FieldPositions* field_positions, GameRulesState* rules, MenuInfo* menuInfo)
{
    // when player from third base starts running, we change camera view. when the situation is over we
    // wait 50 update frames, before moving to normal camera
    if (match->gameFlowState.homeRunCameraCounter >= 0) {
        match->gameFlowState.homeRunCameraCounter++;
        if (match->gameFlowState.homeRunCameraCounter > 50) {
            match->cameraState.homeRunCameraFlag = 0;
            match->gameFlowState.homeRunCameraCounter = -1;
        }
    }

    check_next_batter_decision(match, rules);

    // §18(1) holds in every mode: a batter who has received his three correct pitches is permanently a
    // runner, whatever else is being played. The homerun contest used to be exempt from this along with
    // the free walks, and that exemption was bug #8 — nothing closed the batting turn, so the lukkari
    // pitched a fourth time to a batter who had none left.
    apply_batter_becomes_runner(match, field_positions, &rules->halfInningState, &rules->referee);

    // §26's free walks are normal play only. The contest answers wrong pitches its own way (§8: two wrong
    // gives the runner the free-walk right, three wrong gives the team two runs) and neither is built yet,
    // so offering the normal-play walk here would be the wrong rule rather than a missing one.
    if (rules->scoreboard.period < 4) {
        handle_free_walk_offers(match, &rules->halfInningState, &rules->referee);
    }
}

static void check_next_batter_decision(MatchSession* match, const GameRulesState* rules)
{
    const RefereeState* referee = &rules->referee;
    const BetweenPitchState* bps = &rules->betweenPitchState;
    const Scoreboard* scoreboard = &rules->scoreboard;

    // Cancel pending batter request if inning is ending
    if (referee->endOfInningState != END_INNING_STATE_NONE) {
        match->flowControl.waitingForBatterDecision = 0;
        return;
    }

    // so this function's idea is to make progress in selecting a new batter if old one's gone.
    // so this will be called only once when possible.
    const HalfInningState* his = &rules->halfInningState;

    if (scoreboard->period >= 4) {

    } else if (get_active_batter_index(match) == -1 && match->flowControl.waitingForBatterDecision == 0 &&
               referee->endOfInningState == END_INNING_STATE_NONE) {
        // §12: the batting order is a cycle, so there is always a next regular batter — until the
        // referee pronounces the turn spent, after which only an unused joker can extend it. When
        // neither is available there is nothing to offer: the referee ends the half-inning as soon
        // as the ball is in a home fielder's hands, which is the rule's own third conjunct.
        if (his->lastBatter.turnExhausted == 0 || his->jokersLeft > 0) {
            // have to check that there is only three players in the field too and that it is not a out of bounds
            // situation.
            if (count_active_batting_players(match->playerInfo) < BASE_COUNT && referee->foulState == FOUL_STATE_NONE) {
                // also we cannot know yet if it will be out of position situation so we have to wait that the ball will
                // land in some way.
                if (bps->hasBallHitGround == 1 || bps->catchHasBeenMade == 1) {
                    // if that happens we can now start.
                    int battingTeamIndex = get_batting_team_index(scoreboard);
                    // this will give work to action_invocations.c and execute_actions.c
                    match->flowControl.waitingForBatterDecision = 1;
                    if (his->lastBatter.turnExhausted == 0) {
                        match->pII.batterSelectionIndex =
                            scoreboard->teams[battingTeamIndex]
                                .batterOrder[scoreboard->teams[battingTeamIndex].batterOrderIndex];
                    } else {
                        int i;
                        for (i = 0; i < JOKER_COUNT; i++) {
                            if (match->playerInfo[match->pII.jokerIndices[i]].bTPI.joker == JOKER_AVAILABLE) {
                                match->pII.batterSelectionIndex = match->pII.jokerIndices[i];
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

// §18(1): the batter who has spent his three correct pitches has permanently become a runner. He is no
// longer in the batting turn and no longer safe at home, so he sets off for first — where the outside
// team may burn him at once, exactly as the rule's own note says.
static void apply_batter_becomes_runner(
    MatchSession* match, const FieldPositions* field_positions, const HalfInningState* his, const RefereeState* referee
)
{
    if (batter_has_become_runner_permanently(his->strikes)) {
        // We restore automatic force running to resolve control ambiguity.
        // The batter is now "forced" to run by the rules.
        int index = get_base_controller(match, referee, BASE_HOME);

        // Only force run if player is still there and NOT already running.
        // This prevents re-triggering every frame while preserving the 3 strikes state
        // until the next batter resets it.
        if (index != -1 && match->playerInfo[index].bTPI.state != PLAYER_STATE_RUNNING) {
            run_to_next_base(match, field_positions, index, BASE_HOME);
        }
    }
}

static void handle_free_walk_offers(MatchSession* match, const HalfInningState* his, const RefereeState* referee)
{
    // we calculate the player and the base he has right to go freely only once, and that is when
    // the ball happens. if player moves to next base and user after that decides to make the free walk
    // that wont have any effect.
    if (match->flowControl.freeWalkCalculationMade == 0) {
        if (count_active_batting_players(match->playerInfo) == 1) {
            // if only one player on the field, thats the batter, and then free walks can be made after one pitch.
            if (his->balls >= 1) {
                calculate_free_walk(match, referee);
                match->flowControl.waitingForFreeWalkDecision = 1;
            }
        } else {
            // otherwise there is some non-batter leadrunner and he can have free walks after two balls.
            if (his->balls >= 2) {
                calculate_free_walk(match, referee);
                match->flowControl.waitingForFreeWalkDecision = 1;
            }
        }
        match->flowControl.freeWalkCalculationMade = 1;
    } else {
        // so that if player just ran without taking his free walk, and got wounded or out, then stop asking
        if (match->flowControl.waitingForFreeWalkDecision == 1) {
            if (match->playerInfo[match->flowControl.freeWalkIndex].bTPI.state == PLAYER_STATE_WOUNDED ||
                match->playerInfo[match->flowControl.freeWalkIndex].bTPI.state == PLAYER_STATE_OUT) {
                match->flowControl.waitingForFreeWalkDecision = 0;
            }
        }
    }
}

static int check_end_of_inning(
    MatchSession* match, const FieldPositions* field_positions, const TeamData* team_data,
    GameConclusion* game_conclusion, GameRulesState* rules, MenuInfo* menuInfo, ConsolidationOutput* output
)
{
    const RefereeState* referee = &rules->referee;
    const Scoreboard* scoreboard = &rules->scoreboard;

    // Scoreboard advancement and period logic handled by referee at DETECTED→RESETTING.
    // We only react when Referee signals RESETTING: perform physical reset + menu routing.

    if (referee->endOfInningState != END_INNING_STATE_RESETTING) {
        return 0;
    }

    PeriodTransitionType transition = referee->periodTransition;

    if (transition == PERIOD_TRANSITION_NONE) {
        // Normal next-inning within same period — just reset physical world
        reset_for_new_half_inning(match, field_positions, team_data, rules);
    } else {
        // Period transition — route to appropriate menu
        switch (transition) {
        case PERIOD_TRANSITION_INTER_PERIOD:
            menuInfo->mode = MENU_ENTRY_INTER_PERIOD;
            break;
        case PERIOD_TRANSITION_SUPER_INNING:
            menuInfo->mode = MENU_ENTRY_SUPER_INNING;
            break;
        case PERIOD_TRANSITION_HOMERUN_CONTEST:
            menuInfo->mode = MENU_ENTRY_HOMERUN_CONTEST;
            break;
        case PERIOD_TRANSITION_GAME_OVER:
            populate_game_conclusion(game_conclusion, scoreboard, referee->periodTransitionWinner);
            menuInfo->mode = MENU_ENTRY_GAME_OVER;
            break;
        default:
            break;
        }
        output->request_screen_change = 1;
        output->target_screen = SCREEN_MAIN_MENU;
    }

    return 1; // Signal that we handled end-of-inning
}

static int check_next_pair(MatchSession* match, const FieldPositions* field_positions, GameRulesState* rules)
{
    const Scoreboard* scoreboard = &rules->scoreboard;

    if (scoreboard->period >= 4) {

        // Milestone 17.5: Timer and logic moved to Referee (State Machine).
        // We only react when Referee signals RESETTING (State 2).

        HomeRunPairState currentState = rules->referee.nextPairTransitionState;

        if (currentState == HR_PAIR_STATE_RESETTING) {

            // if equality holds, ending of inning will load the settings.
            if (rules->homeRunContestState.runnerBatterPairCounter != scoreboard->pairCount) {
                // Physical Reset for Next Pair
                reset_for_next_pair(match, field_positions, scoreboard, &rules->homeRunContestState);
            }
            return 1; // Signal that we reset
        }
    }
    return 0; // No reset
}

static void populate_game_conclusion(GameConclusion* game_conclusion, const Scoreboard* scoreboard, int winner)
{
    game_conclusion->winner = winner;
    game_conclusion->isCupGame = scoreboard->isCupGame;
    game_conclusion->period0Runs[0] = scoreboard->teams[0].period0Runs;
    game_conclusion->period0Runs[1] = scoreboard->teams[1].period0Runs;
    game_conclusion->period1Runs[0] = scoreboard->teams[0].period1Runs;
    game_conclusion->period1Runs[1] = scoreboard->teams[1].period1Runs;
    game_conclusion->period2Runs[0] = scoreboard->teams[0].period2Runs;
    game_conclusion->period2Runs[1] = scoreboard->teams[1].period2Runs;
    game_conclusion->period3Runs[0] = scoreboard->teams[0].period3Runs;
    game_conclusion->period3Runs[1] = scoreboard->teams[1].period3Runs;
}
