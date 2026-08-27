#ifndef FIELD_LAYOUT_H
#define FIELD_LAYOUT_H

#include "globals.h"

/**
 * @brief Initializes the field positions with standard Pesäpallo field dimensions.
 *
 * This function populates the FieldPositions structure with the static coordinate
 * data defining the bases, pitch plate, and other key points on the field.
 * It does NOT depend on the global StateInfo, only the pure FieldPositions struct.
 *
 * @param positions Pointer to the FieldPositions structure to populate.
 */
void field_init_positions(FieldPositions* positions);

/**
 * @brief Where a heading runs out of field.
 *
 * The point at which a ray leaving `from` along (dirX, dirZ) reaches the edge of the area a fielder
 * may stand in — the fences, inset by FENCE_OFFSET, which is the same boundary the movement update
 * clamps velocity against.
 *
 * This is what turns a held arrow key into a destination. A human steering a fielder is expressing a
 * heading, but the engine is told WHERE, never which way: a heading means something different when
 * applied twice and nothing at all a tick later, while a point means the same thing however often it
 * arrives. Sending the far end of the heading preserves the gesture exactly (the walk starts along
 * the identical vector) and bounds the damage if the "stop" that should follow it is ever lost — the
 * fielder walks to the fence and stops there, instead of running forever.
 *
 * A degenerate heading, or one that only leads further outside the field, yields `from` itself.
 */
Vector3D field_boundary_point_along(const Vector3D* from, float dirX, float dirZ);

#endif // FIELD_LAYOUT_H