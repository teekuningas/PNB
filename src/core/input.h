#ifndef INPUT_H
#define INPUT_H

#include "globals.h"

int initInput(StateInfo* stateInfo);
void updateInput(StateInfo* stateInfo, GLFWwindow* window);
void clearReleasedKeys(KeyStates* keyStates);

#endif /* INPUT_H */
