/*
	all of the platform specific input code is to be inserted here. we save states to a structure and read it from other places when needed.
*/

#include "globals.h"
#include "input.h"

static int buttonsJustReleased[3][KEY_COUNT];

static void keyCheckL(StateInfo* stateInfo, GLFWwindow* window, const char letter, int keyCode, int index);
static void keyCheckS(StateInfo* stateInfo, GLFWwindow* window, int special, int keyCode, int index);
static void checkKeyImitations(StateInfo* stateInfo);

int initInput(StateInfo* stateInfo)
{
	int i, j;
	for(j = 0; j < 3; j++) {
		for(i = 0; i < KEY_COUNT; i++) {
			stateInfo->keyStates->released[j][i] = 0;
			stateInfo->keyStates->down[j][i] = 0;
			buttonsJustReleased[j][i] = 0;
		}
	}
	return 0;
}
// so in these key checking function the idea is the following. we will have a keyStates array that is used to read the
// states in other parts of code.
// if button is held now, keyStates.down will be 1.
// if button is not held down but it was held down in last frame, we set released to 1 and down to 0.
// after this in the next frame we will se released to 0 too. so down is 1 for all the time key is pressed
// and released is 1 only for one frame just after the key was released.
static void keyCheckL(StateInfo* stateInfo, GLFWwindow* window, const char letter, int keyCode, int index)
{
	if(glfwGetKey(window, letter) == GLFW_PRESS) {
		stateInfo->keyStates->down[index][keyCode] = 1;
	} else {
		if(stateInfo->keyStates->down[index][keyCode] == 1) {
			stateInfo->keyStates->released[index][keyCode] = 1;
			stateInfo->keyStates->down[index][keyCode] = 0;
			buttonsJustReleased[index][keyCode] = 1;
		} else if(buttonsJustReleased[index][keyCode] == 1) {
			buttonsJustReleased[index][keyCode] = 0;
			stateInfo->keyStates->released[index][keyCode] = 0;
		}
	}
}

static void keyCheckS(StateInfo* stateInfo, GLFWwindow* window, int special, int keyCode, int index)
{
	if(glfwGetKey(window, special) == GLFW_PRESS) {
		stateInfo->keyStates->down[index][keyCode] = 1;
	} else {
		if(stateInfo->keyStates->down[index][keyCode] == 1) {
			stateInfo->keyStates->released[index][keyCode] = 1;
			stateInfo->keyStates->down[index][keyCode] = 0;
			buttonsJustReleased[index][keyCode] = 1;
		} else if(buttonsJustReleased[index][keyCode] == 1) {
			buttonsJustReleased[index][keyCode] = 0;
			stateInfo->keyStates->released[index][keyCode] = 0;
		}
	}
}

static void checkKeyImitations(StateInfo* stateInfo)
{
	int i;
	for(i = 0; i < KEY_COUNT; i++) {
		if(stateInfo->keyStates->imitateKeyPress[i] == 1) {
			stateInfo->keyStates->down[2][i] = 1;
		} else {
			if(stateInfo->keyStates->down[2][i] == 1) {
				stateInfo->keyStates->released[2][i] = 1;
				stateInfo->keyStates->down[2][i] = 0;
				buttonsJustReleased[2][i] = 1;
			} else if(buttonsJustReleased[2][i] == 1) {
				buttonsJustReleased[2][i] = 0;
				stateInfo->keyStates->released[2][i] = 0;
			}
		}
	}
}

void updateInput(StateInfo* stateInfo, GLFWwindow* window)
{
	// here we just check them all and name the keys to associate with keycodes used in keyStates.
	keyCheckS(stateInfo, window, GLFW_KEY_RIGHT_CONTROL, KEY_PLUS, 0);
	keyCheckS(stateInfo, window, GLFW_KEY_RIGHT_ALT, KEY_MINUS, 0);
	keyCheckS(stateInfo, window, GLFW_KEY_RIGHT_SHIFT, KEY_1, 0);
	keyCheckS(stateInfo, window, GLFW_KEY_ENTER, KEY_2, 0);
	keyCheckS(stateInfo, window, GLFW_KEY_LEFT, KEY_LEFT, 0);
	keyCheckS(stateInfo, window, GLFW_KEY_RIGHT, KEY_RIGHT, 0);
	keyCheckS(stateInfo, window, GLFW_KEY_UP, KEY_UP, 0);
	keyCheckS(stateInfo, window, GLFW_KEY_DOWN, KEY_DOWN, 0);
	keyCheckS(stateInfo, window, GLFW_KEY_ESCAPE, KEY_HOME, 0);
	keyCheckS(stateInfo, window, GLFW_KEY_LEFT_ALT, KEY_PLUS, 1);
	keyCheckS(stateInfo, window, GLFW_KEY_LEFT_CONTROL, KEY_MINUS, 1);
	keyCheckL(stateInfo, window, 'A', KEY_2, 1);
	keyCheckL(stateInfo, window, 'Z', KEY_1, 1);
	keyCheckL(stateInfo, window, 'F', KEY_LEFT, 1);
	keyCheckL(stateInfo, window, 'H', KEY_RIGHT, 1);
	keyCheckL(stateInfo, window, 'T', KEY_UP, 1);
	keyCheckL(stateInfo, window, 'G', KEY_DOWN, 1);
	keyCheckL(stateInfo, window, 'Q', KEY_HOME, 1);

	checkKeyImitations(stateInfo);
}

void clearReleasedKeys(KeyStates* keyStates)
{
	int i, j;
	for (j = 0; j < 3; j++) {
		for (i = 0; i < KEY_COUNT; i++) {
			keyStates->released[j][i] = 0;
		}
	}
}
