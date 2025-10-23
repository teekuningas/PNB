#define MA_IMPLEMENTATION

#include "globals.h"
#include "sound.h"
#include "miniaudio.h"

static int working;

static ma_sound swing;
static ma_sound catchBall;
static ma_sound menu;
static ma_result result;
static ma_engine engine;

int initSound(StateInfo* stateInfo)
{

	result = ma_engine_init(NULL, &engine);
	if (result != MA_SUCCESS) {
		return -2;
	}

	result = ma_engine_start(&engine);
	if (result != MA_SUCCESS) {
		ma_engine_uninit(&engine);
		return -3;
	}

	result = ma_sound_init_from_file(&engine, "data/sounds/swing.wav", 0, NULL, NULL, &swing);
	if (result != MA_SUCCESS) {
		ma_engine_uninit(&engine);
		return -4;
	}

	result = ma_sound_init_from_file(&engine, "data/sounds/catch.wav", 0, NULL, NULL, &catchBall);
	if (result != MA_SUCCESS) {
		ma_engine_uninit(&engine);
		return -4;
	}

	result = ma_sound_init_from_file(&engine, "data/sounds/menu.wav", 0, NULL, NULL, &menu);
	if (result != MA_SUCCESS) {
		ma_engine_uninit(&engine);
		return -4;
	}

	ma_sound_set_looping(&menu, MA_TRUE);

	stateInfo->playSoundEffect = 0;
	working = 1;

	return 0;
}
void updateSound(StateInfo* stateInfo)
{
	if(stateInfo->playSoundEffect != 0) {
		if(working == 1) {
			switch(stateInfo->playSoundEffect) {
			case SOUND_MENU:
				ma_sound_seek_to_pcm_frame(&menu, 0);
				result = ma_sound_start(&menu);
				break;
			case SOUND_SWING:
				result = ma_sound_start(&swing);
				break;
			case SOUND_CATCH:
				result = ma_sound_start(&catchBall);
				break;
			}
		}
		stateInfo->playSoundEffect = 0;
	}

	if (stateInfo->stopSoundEffect != 0) {
		if (working == 1) {
			switch (stateInfo->stopSoundEffect) {
			case SOUND_MENU:
				ma_sound_stop(&menu);
				break;
			}
		}
		stateInfo->stopSoundEffect = 0;
	}
}

int cleanSound(StateInfo* stateInfo)
{
	ma_engine_uninit(&engine);
	return 0;
}
