#ifndef INPUT_H
#define INPUT_H

#include "globals.h"

int init_input(StateInfo* stateInfo);
void update_input(StateInfo* stateInfo, GLFWwindow* window);
void clear_released_keys(KeyStates* keyStates);

// True if any human pad released `key` this frame. Input source is independent
// of team control, so watcher-facing screens (e.g. game over) use this to accept
// input regardless of which team(s) the AI controlled. Only real pads are read,
// never the phantom CONTROL_AI slot.
int any_human_released(const KeyStates* keyStates, int key);

#endif /* INPUT_H */
