#ifndef SOUND_INTERNAL_H
#define SOUND_INTERNAL_H



#if defined(__wii__)
#include "swing_mp3.h"
#include "catch_mp3.h"
#include "menu_mp3.h" 
#else
#include <fmod.h>
#include <fmod_errors.h>
#endif

extern StateInfo stateInfo;

static int	repeat;
static int hasPlayed;
static int working;
#if defined(__wii__)
#else
static FMOD_SYSTEM      *soundSystem;
static FMOD_SOUND       *swing, *catchBall, *menu;
static FMOD_CHANNEL     *channel = 0;
static FMOD_RESULT       result;
static int               key;
static unsigned int      version;

int ERRCHECK(FMOD_RESULT result)
{
    if (result != FMOD_OK)
    {
        printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
		working = 0;
		return 1;
    }
	return 0;
}

#endif

#endif /* INPUT_INTERNAL_H */