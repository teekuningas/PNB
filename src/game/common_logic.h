#ifndef COMMON_LOGIC_H
#define COMMON_LOGIC_H

#include "globals.h"

int isVectorSmallEnoughSphere(Vector3D* vector, float limit);
int isVectorSmallEnoughCircleXZV(Vector3D* vector, float limit);
int isVectorSmallEnoughCircleXZ(float dx, float dz, float limit);
void setVectorXYZ(Vector3D* vector, float x, float y, float z);
void setVectorV(Vector3D* vector1, Vector3D* vector2);
void setVectorXZ(Vector3D* vector, float x, float z);
void addToVectorXZ(Vector3D* vector, float x, float z);
void addToVectorV(Vector3D* vector1, Vector3D* vector2);
void stopMovement(PlayerInfo* playerInfo, int index);
void smoothOutMovement(MatchSession* match); // Still needs ActionFlags
void stopTargetLookingPlayer(PlayerInfo* playerInfo, PlayerRuntimeState* playerRuntime, int index);
void setOrientation(PlayerInfo* playerInfo, BallInfo* ballInfo, int i);
void run_to_target(PlayerInfo* playerInfo, int index, Vector3D* target);
void move_to_target(PlayerInfo* playerInfo, int index, Vector3D* target);
void move_player_out(
    PlayerInfo* playerInfo, PlayerRuntimeState* playerRuntime, const FieldPositions* field_positions, int index
);
void moveRankedToCatch(MatchSession* match);
void run_to_next_base(
    MatchSession* match, const FieldPositions* field_positions, int index, BaseID base
); // Needs GameControl, PRAI
void run_to_previous_base(
    MatchSession* match, const FieldPositions* field_positions, int index, BaseID base
); // Needs PRAI
void lead(PlayerInfo* playerInfo, PlayerRuntimeState* playerRuntime, const FieldPositions* field_positions, int index);
void changePlayer(MatchSession* match);
void prepare_batter(MatchSession* match);
void calculate_free_walk(MatchSession* match);
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
