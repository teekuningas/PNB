#include "help_menu.h"
#include "menu_types.h"
#include "globals.h"
#include "render.h"
#include "font.h"

#define HELP_LEFT -0.75f

void initHelpMenu(HelpMenuState *state)
{
	state->page = 0;
}

MenuStage updateHelpMenu(HelpMenuState *state, KeyStates *keyStates)
{
	if (keyStates->released[0][KEY_1]) {
		return MENU_STAGE_FRONT;
	}

	if (keyStates->released[0][KEY_2]) {
		state->page++;
		if (state->page > 4) {
			return MENU_STAGE_FRONT;
		}
	}

	return MENU_STAGE_HELP;
}

void drawHelpMenu(HelpMenuState *state)
{
	drawFontBackground();
	if (state->page == 0) {
		printText("Controls", 8, HELP_LEFT, -0.45f, 4);
		printText("Pad 1", 5, HELP_LEFT, -0.3f, 3);
		printText("Direction keys - Arrow keys", 27, HELP_LEFT, -0.2f, 2);
		printText("Action key - Return", 19, HELP_LEFT, -0.15f, 2);
		printText("Second action key - Right shift", 31, HELP_LEFT, -0.10f, 2);
		printText("Left strafe - Right alt", 23, HELP_LEFT, -0.05f, 2);
		printText("Right strafe - Right ctrl", 25, HELP_LEFT, 0.0f, 2);
		printText("Pad 2", 5, HELP_LEFT, 0.1f, 3);
		printText("Direction keys - G F H T", 24, HELP_LEFT, 0.2f, 2);
		printText("Action key - A", 14, HELP_LEFT, 0.25f, 2);
		printText("Second action key - Z", 21, HELP_LEFT, 0.3f, 2);
		printText("Left strafe - Left ctrl", 23, HELP_LEFT, 0.35f, 2);
		printText("Right strafe - Left alt", 23, HELP_LEFT, 0.4f, 2);
	} else if (state->page == 1) {
		char* str;
		printText("Batting", 7, HELP_LEFT, -0.45f, 3);
		str = "Batting is done by the action key. Movement of the batter starts";
		printText(str, strlen(str), HELP_LEFT, -0.35f, 2);
		str = "automatically when pitch is in air. If you dont press the key";
		printText(str, strlen(str), HELP_LEFT, -0.3f, 2);
		str = "before the marker reaches the end of the meter, batter will not bat.";
		printText(str, strlen(str), HELP_LEFT, -0.25f, 2);
		str = "If you want to bat, select power by pressing the key down when marker";
		printText(str, strlen(str), HELP_LEFT, -0.2f, 2);
		str = "reaches wanted height and then releasing it after marker has come down";
		printText(str, strlen(str), HELP_LEFT, -0.15f, 2);
		str = "to the yellow area. ";
		printText(str, strlen(str), HELP_LEFT, -0.10f, 2);
		str = "Balls vertical takeoff direction depends on the actual spot of the";
		printText(str, strlen(str), HELP_LEFT, 0.0f, 2);
		str = "marker in the yellow area. You can adjust the horizontal angle of ";
		printText(str, strlen(str), HELP_LEFT, 0.05f, 2);
		str = "balls takeoff direction by strafing with batter. If there is no";
		printText(str, strlen(str), HELP_LEFT, 0.1f, 2);
		str = "batter you can browse through available batters by pressing the";
		printText(str, strlen(str), HELP_LEFT, 0.15f, 2);
		str = "second action key and select one by pressing the action key";
		printText(str, strlen(str), HELP_LEFT, 0.2f, 2);
	} else if (state->page == 2) {
		char* str;
		printText("Running", 7, HELP_LEFT, -0.45f, 3);
		str = "Running is controlled by the direction keys. Every direction key";
		printText(str, strlen(str), HELP_LEFT, -0.35f, 2);
		str = "represents a base. Down is the home, left is the first base, right";
		printText(str, strlen(str), HELP_LEFT, -0.3f, 2);
		str = "is the second base and up is the third base. If you just suddenly";
		printText(str, strlen(str), HELP_LEFT, -0.25f, 2);
		str = "feel the urge to run, you can double click one of these keys to";
		printText(str, strlen(str), HELP_LEFT, -0.2f, 2);
		str = "make runner at that base to run. Normally you probably should ";
		printText(str, strlen(str), HELP_LEFT, -0.15f, 2);
		str = "synchronize running with pitching and batting though. This is done";
		printText(str, strlen(str), HELP_LEFT, -0.10f, 2);
		str = "by pressing the key once. That will make baserunners start running";
		printText(str, strlen(str), HELP_LEFT, -0.05f, 2);
		str = "immediately after pitch leaves pitchers hand. Batter will start";
		printText(str, strlen(str), HELP_LEFT, 0.0f, 2);
		str = "running after he has finished his swing or bunt. If you ended up";
		printText(str, strlen(str), HELP_LEFT, 0.05f, 2);
		str = "running but arent sure why and would like to return to the previous";
		printText(str, strlen(str), HELP_LEFT, 0.1f, 2);
		str = "base, you can press the corresponding direction key once and the";
		printText(str, strlen(str), HELP_LEFT, 0.15f, 2);
		str = "runner will return if he still is safe on that base.";
		printText(str, strlen(str), HELP_LEFT, 0.2f, 2);
		str = "If pitcher pitches enough invalid pitches you will be prompted if ";
		printText(str, strlen(str), HELP_LEFT, 0.3f, 2);
		str = "you want to take a free walk or not. Action key accepts and second ";
		printText(str, strlen(str), HELP_LEFT, 0.35f, 2);
		str = "action key rejects.";
		printText(str, strlen(str), HELP_LEFT, 0.4f, 2);
	} else if (state->page == 3) {
		char* str;
		printText("Pitching", 8, HELP_LEFT, -0.45f, 3);
		str = "Pitching is also done with the action key. To pitch you first hold";
		printText(str, strlen(str), HELP_LEFT, -0.35f, 2);
		str = "the action key down and the marker will rise. You select the power";
		printText(str, strlen(str), HELP_LEFT, -0.3f, 2);
		str = "by releasing action key and after that you must click the action";
		printText(str, strlen(str), HELP_LEFT, -0.25f, 2);
		str = "key when marker reaches the yellow area again. The final position";
		printText(str, strlen(str), HELP_LEFT, -0.2f, 2);
		str = "of marker in yellow area will affect the takeoff direction of ball.";
		printText(str, strlen(str), HELP_LEFT, -0.15f, 2);
	} else if (state->page == 4) {
		char* str;
		printText("Fielding", 8, HELP_LEFT, -0.45f, 3);
		str = "You move with a fielder by pressing direction keys. If you dont";
		printText(str, strlen(str), HELP_LEFT, -0.35f, 2);
		str = "have the ball, the action key will change the player. If you have";
		printText(str, strlen(str), HELP_LEFT, -0.3f, 2);
		str = "the ball, pressing action key alone will make player drop the ball.";
		printText(str, strlen(str), HELP_LEFT, -0.25f, 2);
		str = "If you hold one direction key when holding down the action key, ";
		printText(str, strlen(str), HELP_LEFT, -0.2f, 2);
		str = "the player will load some throwing power. Releasing the key will";
		printText(str, strlen(str), HELP_LEFT, -0.15f, 2);
		str = "make player to throw the ball towards a base. The base corresponds";
		printText(str, strlen(str), HELP_LEFT, -0.10f, 2);
		str = "to direction key that was pressed. That is, down will be the home";
		printText(str, strlen(str), HELP_LEFT, -0.05f, 2);
		str = "base, left will be the first base, right will be the second base";
		printText(str, strlen(str), HELP_LEFT, 0.0f, 2);
		str = "and up will be the third base.";
		printText(str, strlen(str), HELP_LEFT, 0.05f, 2);
	}
}
