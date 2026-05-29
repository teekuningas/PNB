#ifndef COMMON_LOGIC_H
#define COMMON_LOGIC_H

#include "globals.h"

int is_vector_small_enough_sphere(Vector3D* vector, float limit);
int is_vector_small_enough_circle_xzv(Vector3D* vector, float limit);
int is_vector_small_enough_circle_xz(float dx, float dz, float limit);
void set_vector_xyz(Vector3D* vector, float x, float y, float z);
void set_vector_v(Vector3D* vector1, Vector3D* vector2);
void set_vector_xz(Vector3D* vector, float x, float z);
void add_to_vector_xz(Vector3D* vector, float x, float z);
void add_to_vector_v(Vector3D* vector1, Vector3D* vector2);
void stop_movement(PlayerInfo* playerInfo, int index);
void smooth_out_movement(MatchSession* match); // Still needs ActionFlags
void stop_target_looking_player(PlayerInfo* playerInfo, PlayerRuntimeState* playerRuntime, int index);
void set_orientation(PlayerInfo* playerInfo, BallInfo* ballInfo, int i);
void run_to_target(PlayerInfo* playerInfo, int index, Vector3D* target);
void move_to_target(PlayerInfo* playerInfo, int index, Vector3D* target);
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
void initialize_spatial_player_information(
    MatchSession* match, const FieldPositions* field_positions, unsigned int* rng_seed
);
void initialize_inning_permanent_player_information(
    MatchSession* match, const Scoreboard* scoreboard, const TeamData* team_data
);
void initialize_non_critical_player_information(MatchSession* match);
void initialize_ball_info(MatchSession* match);
void initialize_action_info(MatchSession* match);
void reset_flow_state(MatchSession* match);
void initialize_critical_game_info(MatchSession* match, const Scoreboard* scoreboard);
void initialize_index_information(MatchSession* match);
void initialize_prai_information(MatchSession* match);
void setup_homerun_physical_state(
    MatchSession* match, const Scoreboard* scoreboard, const FieldPositions* field_positions
);
void clear_frame_events(GameEvents* events);

#endif /* COMMON_LOGIC_H */
