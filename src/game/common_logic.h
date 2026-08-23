#ifndef COMMON_LOGIC_H
#define COMMON_LOGIC_H

#include "globals.h"

void stop_movement(PlayerInfo* playerInfo, int index);
void smooth_out_movement(MatchSession* match);
void stop_target_looking_player(PlayerInfo* playerInfo, int index);
void set_orientation(PlayerInfo* playerInfo, const BallInfo* ballInfo, int i);
void move_to_target(PlayerInfo* playerInfo, int index, const Vector3D* target);
void move_player_out(
    PlayerInfo* playerInfo, PlayerRuntimeState* playerRuntime, const FieldPositions* field_positions, int index
);
void move_ranked_to_catch(MatchSession* match);
void run_to_next_base(
    MatchSession* match, const FieldPositions* field_positions, int index, BaseID base
); // Needs GameControl, PRAI
void run_to_previous_base(
    MatchSession* match, const FieldPositions* field_positions, int index, BaseID base
); // Needs PRAI
void lead_from_base(
    PlayerInfo* playerInfo, PlayerRuntimeState* playerRuntime, const FieldPositions* field_positions, int index
);
void change_player(MatchSession* match);
void prepare_batter(MatchSession* match);
void calculate_free_walk(MatchSession* match, const RefereeState* referee);
void initialize_spatial_player_information(MatchSession* match, const FieldPositions* field_positions);
void initialize_inning_permanent_player_information(
    MatchSession* match, const Scoreboard* scoreboard, const TeamData* team_data
);
void initialize_non_critical_player_information(MatchSession* match);
void initialize_ball_info(MatchSession* match);
void initialize_action_info(MatchSession* match);
void reset_flow_state(MatchSession* match, PlayerCounters* player_counters);
void initialize_critical_game_info(MatchSession* match, PlayerCounters* player_counters, const Scoreboard* scoreboard);
void initialize_index_information(MatchSession* match);
void initialize_prai_information(MatchSession* match);
void setup_homerun_physical_state(
    MatchSession* match, const Scoreboard* scoreboard, const HomeRunContestState* hrcs,
    const FieldPositions* field_positions, int batterResumesInPlace
);
void clear_frame_events(GameEvents* events);

#endif /* COMMON_LOGIC_H */
