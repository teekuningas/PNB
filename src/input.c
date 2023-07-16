#include "globals.h"
#include "input.h"
#include "input_internal.h"
/*
	all of the platform specific input code is to be inserted here. we save states to a structure and read it from other places when needed.
*/

int initInput()
{
	int i, j;
	stateInfo.keyStates = &keyStates;
	for(j = 0; j < 3; j++)
	{
		for(i = 0; i < KEY_COUNT; i++)
		{
			keyStates.released[j][i] = 0;
			keyStates.down[j][i] = 0;
			buttonsJustReleased[j][i] = 0;
		}
	}
	#if defined(__wii__)
	WPAD_Init();
	#endif
	return 0;
}
// so in these key checking function the idea is the following. we will have a keyStates array that is used to read the
// states in other parts of code.
// if button is held now, keyStates.down will be 1.
// if button is not held down but it was held down in last frame, we set released to 1 and down to 0.
// after this in the next frame we will se released to 0 too. so down is 1 for all the time key is pressed
// and released is 1 only for one frame just after the key was released.
#if defined(__wii__)
static __inline void keyCheckWii(int special, int keyCode, int index)
{
	if(buttonsHeld[index] & special)
	{
		keyStates.down[index][keyCode] = 1;
	}
	else
	{

		if(keyStates.down[index][keyCode] == 1)
		{
			keyStates.released[index][keyCode] = 1;
			keyStates.down[index][keyCode] = 0;
			buttonsJustReleased[index][keyCode] = 1;
		}
		else if(buttonsJustReleased[index][keyCode] == 1)
		{
			buttonsJustReleased[index][keyCode] = 0;
			keyStates.released[index][keyCode] = 0;
		}
	}
}
#else
static __inline void keyCheckL(const char letter, int keyCode, int index)
{
	if(glfwGetKey(letter) == GLFW_PRESS)
	{
		keyStates.down[index][keyCode] = 1;
	}
	else
	{
		if(keyStates.down[index][keyCode] == 1)
		{
			keyStates.released[index][keyCode] = 1;
			keyStates.down[index][keyCode] = 0;
			buttonsJustReleased[index][keyCode] = 1;
		}
		else if(buttonsJustReleased[index][keyCode] == 1)
		{
			buttonsJustReleased[index][keyCode] = 0;
			keyStates.released[index][keyCode] = 0;
		}
	}
}

static __inline void keyCheckS(int special, int keyCode, int index)
{
	if(glfwGetKey(special) == GLFW_PRESS)
	{
		keyStates.down[index][keyCode] = 1;
	}
	else
	{
		if(keyStates.down[index][keyCode] == 1)
		{
			keyStates.released[index][keyCode] = 1;
			keyStates.down[index][keyCode] = 0;
			buttonsJustReleased[index][keyCode] = 1;
		}
		else if(buttonsJustReleased[index][keyCode] == 1)
		{
			buttonsJustReleased[index][keyCode] = 0;
			keyStates.released[index][keyCode] = 0;
		}
	}
}
#endif

static __inline void checkKeyImitations()
{
	int i;
	for(i = 0; i < KEY_COUNT; i++)
	{
		if(stateInfo.keyStates->imitateKeyPress[i] == 1)
		{
			keyStates.down[2][i] = 1;
		}
		else
		{
			if(keyStates.down[2][i] == 1)
			{
				keyStates.released[2][i] = 1;
				keyStates.down[2][i] = 0;
				buttonsJustReleased[2][i] = 1;
			}
			else if(buttonsJustReleased[2][i] == 1)
			{
				buttonsJustReleased[2][i] = 0;
				keyStates.released[2][i] = 0;
			}
		}
	}
}

void updateInput()
{
	// here we just check them all and name the keys to associate with keycodes used in keyStates.
	#if defined(__wii__)
	WPAD_ScanPads();
	buttonsHeld[0] = WPAD_ButtonsHeld(0);
	buttonsHeld[1] = WPAD_ButtonsHeld(1);

	keyCheckWii(WPAD_BUTTON_PLUS, KEY_PLUS, 0);
	keyCheckWii(WPAD_BUTTON_MINUS, KEY_MINUS, 0);
	keyCheckWii(WPAD_BUTTON_1, KEY_1, 0);
	keyCheckWii(WPAD_BUTTON_2, KEY_2, 0);
	keyCheckWii(WPAD_BUTTON_LEFT, KEY_LEFT, 0);
	keyCheckWii(WPAD_BUTTON_RIGHT, KEY_RIGHT, 0);
	keyCheckWii(WPAD_BUTTON_UP, KEY_UP, 0);
	keyCheckWii(WPAD_BUTTON_DOWN, KEY_DOWN, 0);
	keyCheckWii(WPAD_BUTTON_HOME, KEY_HOME, 0);
	keyCheckWii(WPAD_BUTTON_PLUS, KEY_PLUS, 1);
	keyCheckWii(WPAD_BUTTON_MINUS, KEY_MINUS, 1);
	keyCheckWii(WPAD_BUTTON_1, KEY_1, 1);
	keyCheckWii(WPAD_BUTTON_2, KEY_2, 1);
	keyCheckWii(WPAD_BUTTON_LEFT, KEY_LEFT, 1);
	keyCheckWii(WPAD_BUTTON_RIGHT, KEY_RIGHT, 1);
	keyCheckWii(WPAD_BUTTON_UP, KEY_UP, 1);
	keyCheckWii(WPAD_BUTTON_DOWN, KEY_DOWN, 1);
	keyCheckWii(WPAD_BUTTON_HOME, KEY_HOME, 1);
	#else
	keyCheckS(GLFW_KEY_RCTRL, KEY_PLUS, 0);
	keyCheckS(GLFW_KEY_RALT, KEY_MINUS, 0);
	keyCheckS(GLFW_KEY_RSHIFT, KEY_1, 0);
	keyCheckS(GLFW_KEY_ENTER, KEY_2, 0);
	keyCheckS(GLFW_KEY_LEFT, KEY_LEFT, 0);
	keyCheckS(GLFW_KEY_RIGHT, KEY_RIGHT, 0);
	keyCheckS(GLFW_KEY_UP, KEY_UP, 0);
	keyCheckS(GLFW_KEY_DOWN, KEY_DOWN, 0);
	keyCheckS(GLFW_KEY_ESC, KEY_HOME, 0);
	keyCheckS(GLFW_KEY_LALT, KEY_PLUS, 1);
	keyCheckS(GLFW_KEY_LCTRL, KEY_MINUS, 1);
	keyCheckL('A', KEY_2, 1);
	keyCheckL('Z', KEY_1, 1);
	keyCheckL('F', KEY_LEFT, 1);
	keyCheckL('H', KEY_RIGHT, 1);
	keyCheckL('T', KEY_UP, 1);
	keyCheckL('G', KEY_DOWN, 1);
	keyCheckL('Q', KEY_HOME, 1);
	#endif

	checkKeyImitations();
}
