#ifndef DISPLAY_H
#define DISPLAY_H

// The screen's own constants: the virtual resolution everything is laid out in, the perspective, the light.
// Client-side only — nothing in the engine may need them, and nothing headless reads them.

#define PERSPECTIVE_ASPECT_RATIO (16.0f / 9.0f)
#define VIRTUAL_WIDTH 1920
#define VIRTUAL_HEIGHT 1080

#define LIGHT_SOURCE_POSITION_X 30.0f
#define LIGHT_SOURCE_POSITION_Y 50.0f
#define LIGHT_SOURCE_POSITION_Z -50.0f

#define SHADOW_HEIGHT 0.01f

#endif /* DISPLAY_H */
