#include "test_helpers.h"
#include "sim_harness.h"
#include "game_screen.h"
#include "game_frame.h"
#include <string.h>

// The screen writes nothing into the World. The same seed is played twice, frame for frame: once through
// the screen's own update — the camera following the ball, the HUD taking announcements, the pause key
// read — and once ticked bare, the way the headless tiers and a future peer tick it. The World must be the
// same bytes after every tick, the physical half and the legal half alike.
//
// It holds a second claim on the way. The two games load their rosters separately, so every name and
// every TeamData lives at a different address in each; a pointer anywhere in the World would make the
// bytes differ. That is the mechanism behind "MatchSession holds no pointers" — the blittability a
// snapshot, a rollback and a peer all depend on — where there used to be only a sentence, and the sentence
// was wrong: each batting player carried a pointer to his name.

#define SCREEN_TEST_FRAMES 6000

int test_the_screen_writes_nothing_into_the_world(void)
{
    GameSetup setup;
    sim_make_normal_setup(&setup, 0, 1, CONTROL_AI, CONTROL_AI);
    SimGame* screened = sim_create(&setup, 0x5C2EE1u);
    SimGame* bare = sim_create(&setup, 0x5C2EE1u);
    ASSERT_NOT_NULL(screened, "sim_create returned NULL");
    ASSERT_NOT_NULL(bare, "sim_create returned NULL");
    ASSERT(
        screened->state->teamData != bare->state->teamData, "the two rosters must live apart for this to see pointers"
    );

    HudState hud = {.infoEvent = EVENT_NONE, .infoEventTimer = -1};
    CameraView camera = {0};
    for (int frame = 0; frame < SCREEN_TEST_FRAMES; frame++) {
        update_game_screen(screened->state, &hud, &camera, &screened->menu);
        update_game_frame(bare->state, &bare->menu);
        if (memcmp(screened->state->match, bare->state->match, sizeof(MatchSession)) != 0) {
            printf("    MatchSession differs after frame %d\n", frame);
            ASSERT(0, "the screen must not change the physical world");
        }
        if (memcmp(screened->state->rules, bare->state->rules, sizeof(GameRulesState)) != 0) {
            printf("    GameRulesState differs after frame %d\n", frame);
            ASSERT(0, "the screen must not change the legal world");
        }
    }
    ASSERT(camera.target.x != 0.0f || camera.target.z != -1.0f, "and the screen really did run: its camera moved");

    sim_destroy(screened);
    sim_destroy(bare);
    return TEST_PASSED;
}
