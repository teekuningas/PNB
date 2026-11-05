#ifndef PLAYER_MOTION_H
#define PLAYER_MOTION_H

#include "field.h"
#include "world.h"

// A body's movement primitives: stop it, face it at the ball, walk it to a point, walk it off the field.
// Every mover in the engine — the actualizers, the fielders' behaviours, enforcement — moves a body
// through these.
void stop_movement(PlayerInfo* playerInfo, int index);
void stop_target_looking_player(PlayerInfo* playerInfo, int index);
void set_orientation(PlayerInfo* playerInfo, const BallInfo* ballInfo, int i);
void move_to_target(PlayerInfo* playerInfo, int index, const Vector3D* target);
void move_player_out(
    PlayerInfo* playerInfo, PlayerRuntimeState* playerRuntime, const FieldPositions* field_positions, int index
);

#endif /* PLAYER_MOTION_H */
