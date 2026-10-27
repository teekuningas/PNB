#ifndef GAME_SCREEN_H
#define GAME_SCREEN_H

#include "globals.h"
#include "menu_types.h"
#include "resource_manager.h"

// The HUD's own memory: where its markers were drawn last, for the render interpolator, and the info
// line's current message and how long it has been up. Client-local render memory, owned by whoever runs
// the screen — never in the World, never on the wire, never reached by the engine. The screen's entry
// points take it explicitly so that "the draw code writes this, and only this" is in their signatures.
typedef struct _HudState {
    float lastMeterX; // the catching marker's previous position
    // The batting marker's previous position. NEGATIVE means it was not drawn last frame — the batter's
    // bar legitimately has no cursor between his two beats — and a marker that has just appeared has
    // nothing to interpolate from, so it snaps instead of streaking across the bar from wherever the last
    // one died.
    float lastSwingMeterX;
    GameEventType infoEvent; // the message on the info line
    int infoEventTimer; // frames it has been up; -1 when the line is empty
} HudState;

// The screen's camera: where it looks from and where it looks at, this tick and last, so the draw can
// interpolate between them. Client-local render memory like HudState — the screen moves it after each tick,
// nothing in the engine reads it, and it is in no snapshot. (It lived in the World, where the screen wrote
// it between ticks, so a rendered World and a headless one differed for the same seed.)
typedef struct _CameraView {
    Vector3D eye;
    Vector3D lastEye;
    Vector3D target;
    Vector3D lastTarget;
} CameraView;

int init_game_screen(StateInfo* stateInfo, HudState* hud, CameraView* camera, ResourceManager* rm);
void update_game_screen(StateInfo* stateInfo, HudState* hud, CameraView* camera, MenuInfo* menuInfo);
void draw_game_screen(
    const StateInfo* stateInfo, HudState* hud, const CameraView* camera, double alpha, ResourceManager* rm,
    const RenderState* rs
);
// The screen's side of one tick that ran: the info line's clock advances, then the referee's announcement
// is shown and consumed.
void hud_after_tick(HudState* hud, GameEventType* announcement);
int clean_game_screen(StateInfo* stateInfo);

#endif /* GAME_SCREEN_H */
