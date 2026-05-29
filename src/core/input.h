#ifndef INPUT_H
#define INPUT_H

#include "globals.h"

int init_input(StateInfo* stateInfo);
void update_input(StateInfo* stateInfo, GLFWwindow* window);
void clear_released_keys(KeyStates* keyStates);

#endif /* INPUT_H */
