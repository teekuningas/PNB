#ifndef SOUND_INTERNAL_H
#define SOUND_INTERNAL_H

#define MA_IMPLEMENTATION
#include "miniaudio.h"

extern StateInfo stateInfo;

static int repeat;
static int hasPlayed;
static int working;

static ma_sound swing;
static ma_sound catchBall;
static ma_sound menu;
static ma_result result;
static ma_engine engine;

#endif /* INPUT_INTERNAL_H */
