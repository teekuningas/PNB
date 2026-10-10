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

int init_game_screen(StateInfo* stateInfo, HudState* hud, ResourceManager* rm);
void update_game_screen(StateInfo* stateInfo, HudState* hud, MenuInfo* menuInfo);
void draw_game_screen(
    const StateInfo* stateInfo, HudState* hud, double alpha, ResourceManager* rm, const RenderState* rs
);
// The screen's side of the tick's announcement: show it, and consume it.
void hud_take_announcement(HudState* hud, GameEventType* announcement);
int clean_game_screen(StateInfo* stateInfo);

#endif /* GAME_SCREEN_H */
