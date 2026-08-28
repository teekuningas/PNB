#include "game_reset.h"
#include "common_logic.h"
#include "referee.h"

static void reset_physical_world(MatchSession* match, const FieldPositions* field_positions)
{
    initialize_ball_info(match);
    initialize_action_info(match);
    initialize_index_information(match);
    initialize_prai_information(match);
    initialize_spatial_player_information(match, field_positions);
    initialize_non_critical_player_information(match);
}

// Recipe 1: Full physical + flow + team reset for new half-inning.
// Called by consolidation when referee signals END_INNING_STATE_RESETTING.
// NOTE: Referee has already cleared its own legal state at DETECTED→RESETTING.
// This function does NOT touch referee-owned state (ownership boundary).
void reset_for_new_half_inning(
    MatchSession* match, const FieldPositions* field_positions, const TeamData* team_data, GameRulesState* rules
)
{
    reset_physical_world(match, field_positions);
    reset_flow_state(match);

    // Team setup
    initialize_inning_permanent_player_information(match, &rules->scoreboard, team_data);

    if (rules->scoreboard.period >= 4) {
        if (!(rules->homeRunContestState.runnerBatterPairCounter > 0 &&
              rules->homeRunContestState.runnerBatterPairCounter < rules->scoreboard.pairCount)) {
            rules->homeRunContestState.runnerBatterPairCounter = 0;
        }
        setup_homerun_physical_state(match, &rules->scoreboard, &rules->homeRunContestState, field_positions, 0);
    }
}

// Recipe 2: Foul play — referee already restored legal state from snapshot
void reset_for_foul_play(MatchSession* match, const FieldPositions* field_positions, GameRulesState* rules)
{
    reset_physical_world(match, field_positions);
    reset_flow_state(match);

    if (rules->scoreboard.period >= 4) {
        // Homerun Contest: same batter resumes after a foul/out-of-bounds, so keep them at the plate.
        setup_homerun_physical_state(match, &rules->scoreboard, &rules->homeRunContestState, field_positions, 1);
    } else {
        // restorePlayersToRefereePositions: Restore players to their bases at the start of the pitch
        for (int j = 0; j < PLAYERS_IN_TEAM + JOKER_COUNT; j++) {
            if (rules->referee.battingPlayers[j].baseAtPitchStart != BASE_NONE) {
                BaseID restoreBase = rules->referee.battingPlayers[j].baseAtPitchStart;

                // 1. Restore Player State and ID (Physical/Logical State)
                if (restoreBase == BASE_HOME) {
                    match->playerInfo[j].bTPI.state = PLAYER_STATE_AT_BAT;
                } else {
                    match->playerInfo[j].bTPI.state = PLAYER_STATE_ON_BASE;
                }
                match->playerInfo[j].bTPI.baseId = restoreBase;

                // 5. Handle the Batter - Physical State Only
                if (restoreBase == BASE_HOME) {
                    // Prepare batter for next pitch (animation etc)
                    prepare_batter(match);
                }

                // 6. Restore Physical Locations for field runners
                if (match->playerInfo[j].bTPI.baseId == BASE_FIRST) {
                    match->playerInfo[j].tPI.location.x = field_positions->firstBaseRun.x;
                    match->playerInfo[j].tPI.location.z = field_positions->firstBaseRun.z;
                } else if (match->playerInfo[j].bTPI.baseId == BASE_SECOND) {
                    match->playerInfo[j].tPI.location.x = field_positions->secondBaseRun.x;
                    match->playerInfo[j].tPI.location.z = field_positions->secondBaseRun.z;
                } else if (match->playerInfo[j].bTPI.baseId == BASE_THIRD) {
                    match->playerInfo[j].tPI.location.x = field_positions->thirdBaseRun.x;
                    match->playerInfo[j].tPI.location.z = field_positions->thirdBaseRun.z;
                }
            } else {
                // Restore OUT/SCORED/WOUNDED states to avoid re-triggering animations
                // This is physical state sync
                if (rules->referee.battingPlayers[j].status == PLAYER_STATUS_OUT) {
                    match->playerInfo[j].bTPI.state = PLAYER_STATE_OUT;
                    match->playerInfo[j].bTPI.baseId = BASE_NONE;
                } else if (rules->referee.battingPlayers[j].hasScored) {
                    match->playerInfo[j].bTPI.state = PLAYER_STATE_SCORED;
                    match->playerInfo[j].bTPI.baseId = BASE_NONE;
                } else if (rules->referee.battingPlayers[j].status == PLAYER_STATUS_WOUNDED) {
                    match->playerInfo[j].bTPI.state = PLAYER_STATE_WOUNDED;
                    match->playerInfo[j].bTPI.baseId = BASE_NONE;
                }
            }
        }
    }
}

// Recipe 3: Next HR pair — referee already cleared per-pair state
void reset_for_next_pair(
    MatchSession* match, const FieldPositions* field_positions, const Scoreboard* scoreboard,
    const HomeRunContestState* hrcs
)
{
    reset_physical_world(match, field_positions);
    reset_flow_state(match);
    setup_homerun_physical_state(match, scoreboard, hrcs, field_positions, 0);
}
