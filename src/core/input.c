/*
    all of the platform specific input code is to be inserted here. we save states to a structure and read it from other
   places when needed.
*/

#include "globals.h"
#include "input.h"

static int buttonsJustReleased[3][KEY_COUNT];

static void keyCheck(StateInfo* stateInfo, GLFWwindow* window, int glfwKey, int keyCode, int index);

int init_input(StateInfo* stateInfo)
{
    int i, j;
    for (j = 0; j < 3; j++) {
        for (i = 0; i < KEY_COUNT; i++) {
            stateInfo->keyStates->released[j][i] = 0;
            stateInfo->keyStates->pressed[j][i] = 0;
            stateInfo->keyStates->down[j][i] = 0;
            buttonsJustReleased[j][i] = 0;
        }
    }
    return 0;
}
// One key, three readings (see KeyStates). `down` is a level: 1 for as long as the key is held.
// `pressed` and `released` are one-frame EDGES on either side of that level — pressed on the frame
// the key goes down, released on the frame after it comes up.
//
// There used to be two of these, one taking a char and one taking an int, with byte-identical bodies:
// both arguments were passed straight to the same glfwGetKey call, so the duplication was a type and
// not a concept.
static void keyCheck(StateInfo* stateInfo, GLFWwindow* window, int glfwKey, int keyCode, int index)
{
    KeyStates* keys = stateInfo->keyStates;

    if (glfwGetKey(window, glfwKey) == GLFW_PRESS) {
        // The press edge is the transition, so it is derived from `down` NOT yet being set — the
        // mirror of how the release edge is derived from it still being set below.
        keys->pressed[index][keyCode] = (keys->down[index][keyCode] == 0) ? 1 : 0;
        keys->down[index][keyCode] = 1;
    } else {
        keys->pressed[index][keyCode] = 0;
        if (keys->down[index][keyCode] == 1) {
            keys->released[index][keyCode] = 1;
            keys->down[index][keyCode] = 0;
            buttonsJustReleased[index][keyCode] = 1;
        } else if (buttonsJustReleased[index][keyCode] == 1) {
            buttonsJustReleased[index][keyCode] = 0;
            keys->released[index][keyCode] = 0;
        }
    }
}

void update_input(StateInfo* stateInfo, GLFWwindow* window)
{
    // here we just check them all and name the keys to associate with keycodes used in keyStates.
    keyCheck(stateInfo, window, GLFW_KEY_RIGHT_CONTROL, KEY_PLUS, 0);
    keyCheck(stateInfo, window, GLFW_KEY_RIGHT_ALT, KEY_MINUS, 0);
    keyCheck(stateInfo, window, GLFW_KEY_RIGHT_SHIFT, KEY_1, 0);
    keyCheck(stateInfo, window, GLFW_KEY_ENTER, KEY_2, 0);
    keyCheck(stateInfo, window, GLFW_KEY_LEFT, KEY_LEFT, 0);
    keyCheck(stateInfo, window, GLFW_KEY_RIGHT, KEY_RIGHT, 0);
    keyCheck(stateInfo, window, GLFW_KEY_UP, KEY_UP, 0);
    keyCheck(stateInfo, window, GLFW_KEY_DOWN, KEY_DOWN, 0);
    keyCheck(stateInfo, window, GLFW_KEY_ESCAPE, KEY_HOME, 0);
    keyCheck(stateInfo, window, GLFW_KEY_LEFT_ALT, KEY_PLUS, 1);
    keyCheck(stateInfo, window, GLFW_KEY_LEFT_CONTROL, KEY_MINUS, 1);
    keyCheck(stateInfo, window, 'A', KEY_2, 1);
    keyCheck(stateInfo, window, 'Z', KEY_1, 1);
    keyCheck(stateInfo, window, 'F', KEY_LEFT, 1);
    keyCheck(stateInfo, window, 'H', KEY_RIGHT, 1);
    keyCheck(stateInfo, window, 'T', KEY_UP, 1);
    keyCheck(stateInfo, window, 'G', KEY_DOWN, 1);
    keyCheck(stateInfo, window, 'Q', KEY_HOME, 1);
}

// Drop both edges without touching `down`. A screen that consumes a key so the next screen does not
// also act on it has to drop the press as well as the release, or the edge it swallowed on one side
// of the transition arrives on the other.
void clear_released_keys(KeyStates* keyStates)
{
    int i, j;
    for (j = 0; j < 3; j++) {
        for (i = 0; i < KEY_COUNT; i++) {
            keyStates->released[j][i] = 0;
            keyStates->pressed[j][i] = 0;
        }
    }
}

int any_human_released(const KeyStates* keyStates, int key)
{
    for (int pad = 0; pad < HUMAN_PAD_COUNT; pad++) {
        if (keyStates->released[pad][key]) {
            return 1;
        }
    }
    return 0;
}
