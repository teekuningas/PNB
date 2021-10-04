#include "globals.h"
#include "sound.h"
#include "sound_internal.h"

int initSound()
{
	stateInfo.playSoundEffect = 0;
	working = 1;
	#if defined(__wii__)
	// Initialise the audio subsystem
	ASND_Init(NULL);
	MP3Player_Init();
	#else

    /*
        Create a System object and initialize.
    */
    result = FMOD_System_Create(&soundSystem);
    ERRCHECK(result);

    result = FMOD_System_GetVersion(soundSystem, &version);
    ERRCHECK(result);

    if (version < FMOD_VERSION)
    {
        printf("Error!  You are using an old version of FMOD %08x.  This program requires %08x\n", version, FMOD_VERSION);
        return 0;
    }

    result = FMOD_System_Init(soundSystem, 32, FMOD_INIT_NORMAL, NULL);
    ERRCHECK(result);

    result = FMOD_System_CreateSound(soundSystem, "../../sounds/swing.mp3", FMOD_HARDWARE, 0, &swing);
    ERRCHECK(result);    

    result = FMOD_System_CreateSound(soundSystem, "../../sounds/catch.mp3", FMOD_HARDWARE, 0, &catchBall);
    ERRCHECK(result);   

    result = FMOD_System_CreateSound(soundSystem, "../../sounds/menu.mp3", FMOD_HARDWARE, 0, &menu);
    ERRCHECK(result);  

	#endif

	repeat = 0;
	hasPlayed = 0;
	
	return 0;
}
void updateSound()
{
	if(stateInfo.playSoundEffect != 0)
	{
		if(working == 1)
		{
			switch(stateInfo.playSoundEffect)
			{
				case SOUND_MENU:
					#if defined(__wii__)
					MP3Player_PlayBuffer(menu_mp3, menu_mp3_size, NULL);
					#else

					result = FMOD_System_PlaySound(soundSystem, 0, menu, 0, &channel);
					ERRCHECK(result);
					#endif
					hasPlayed = 0;
					repeat = 1;
					break;
				case SOUND_SWING:
					#if defined(__wii__)
					MP3Player_PlayBuffer(swing_mp3, swing_mp3_size, NULL);
					#else
					result = FMOD_System_PlaySound(soundSystem, 0, swing, 0, &channel);
					ERRCHECK(result);
					#endif
					repeat = 0;
					break;
				case SOUND_CATCH:
					#if defined(__wii__)
					MP3Player_PlayBuffer(catch_mp3, catch_mp3_size, NULL);
					#else
					result = FMOD_System_PlaySound(soundSystem, 0, catchBall, 0, &channel);
					ERRCHECK(result);
					#endif
					repeat = 0;
					break;

			}
		}

		stateInfo.playSoundEffect = 0;
	}
	else
	{
		if(working == 1)
		{
			if(repeat == 1)
			{
				#if defined(__wii__)
				int playing = MP3Player_IsPlaying();
				#else
				FMOD_BOOL playing;
				FMOD_Channel_IsPlaying(channel, &playing);
				#endif
				if(playing != 0) hasPlayed = 1;
				if(playing == 0 && hasPlayed == 1)
				{
					#if defined(__wii__)
					MP3Player_PlayBuffer(menu_mp3, menu_mp3_size, NULL);
					#else
					result = FMOD_System_PlaySound(soundSystem, 0, menu, 0, &channel);
					ERRCHECK(result);
					#endif
					hasPlayed = 0;
				}
			}
		}
		
	}


}

int cleanSound()
{
	#if defined(__wii__)
	#else
	int result2;
    result = FMOD_Sound_Release(swing);
    result2 = ERRCHECK(result);
    if(result2 != 0)
	{
		return 1;
	}
    result = FMOD_Sound_Release(catchBall);
    result2 = ERRCHECK(result);
    if(result2 != 0)
	{
		return 1;
	}
	result = FMOD_System_Close(soundSystem);
    result2 = ERRCHECK(result);
    if(result2 != 0)
	{
		return 1;
	}
    result = FMOD_System_Release(soundSystem);
    result2 = ERRCHECK(result);
    if(result2 != 0)
	{
		return 1;
	}	
	#endif
	return 0;
}