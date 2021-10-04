#ifndef INPUT_INTERNAL_H
#define INPUT_INTERNAL_H

extern StateInfo stateInfo;
static KeyStates keyStates;
static int buttonsJustReleased[3][KEY_COUNT];

#if defined(__wii__)
static u16 buttonsHeld[2];
#endif

#endif /* INPUT_INTERNAL_H */