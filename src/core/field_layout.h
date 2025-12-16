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

#endif // FIELD_LAYOUT_H