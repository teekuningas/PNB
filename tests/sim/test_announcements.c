#include "test_helpers.h"
#include "sim_harness.h"
#include "sim_observers.h"
#include "game_screen.h"
#include <string.h>

// What the referee announces — OUT, STRIKE, RUN — is an output of the tick, and nothing that decides the
// game may depend on whether anybody displayed it.
//
// That was not true while the announcement lived in the legal world. The draw code cleared it there
// after showing it, and the referee read it back: the foul-play announcement spoke only if nothing else
// was standing. A rendered game therefore announced fouls that a headless one swallowed, and the two
// held different bytes in their HalfInningState for the same seed — the World depending on whether a
// renderer had run, which is the one thing the tick equation says it may never depend on.
//
// So the same seed is played twice: once with a screen consuming every announcement the way the real
// one does (hud_take_announcement, once per tick), and once with nobody consuming anything. The game
// must be the same game, and the announcements the same announcements, frame for frame.

#define ANNOUNCE_MAX 4096
#define PERIOD_MAX_FRAMES 120000L

typedef struct {
    int consume; // 1 = behave like the screen; 0 = nobody is watching
    HudState hud;
    int count;
    long frame[ANNOUNCE_MAX];
    GameEventType event[ANNOUNCE_MAX];
} AnnouncementLog;

static void announcement_hook(const SimGame* g, void* ctx)
{
    AnnouncementLog* log = (AnnouncementLog*)ctx;
    GameEventType* announcement = &g->state->announcement;
    if (*announcement != EVENT_NONE && log->count < ANNOUNCE_MAX) {
        log->frame[log->count] = g->frame;
        log->event[log->count] = *announcement;
        log->count++;
    }
    if (log->consume) hud_take_announcement(&log->hud, announcement);
}

static int three_half_innings_done(const SimGame* g)
{
    return (g->state->rules->scoreboard.inning - g->start_inning) >= 3 || g->state->screen != SCREEN_GAME;
}

static int play(unsigned int seed, AnnouncementLog* log, unsigned long long* hash, long* frames)
{
    GameSetup setup;
    sim_make_normal_setup(&setup, 0, 1, CONTROL_AI, CONTROL_AI);
    SimGame* g = sim_create(&setup, seed);
    if (!g) return 0;
    ChecksumObserver cs;
    checksum_observer_init(&cs);
    sim_attach(g, checksum_observer_hook, &cs);
    sim_attach(g, announcement_hook, log);
    sim_run_until(g, three_half_innings_done, PERIOD_MAX_FRAMES);
    int ok = !g->failed;
    *hash = cs.hash;
    *frames = g->frame;
    sim_destroy(g);
    return ok;
}

int test_announcements_do_not_depend_on_the_screen(void)
{
    static AnnouncementLog watched, unwatched;
    memset(&watched, 0, sizeof(watched));
    memset(&unwatched, 0, sizeof(unwatched));
    watched.consume = 1;
    watched.hud.infoEventTimer = -1;

    unsigned long long watchedHash, unwatchedHash;
    long watchedFrames, unwatchedFrames;
    ASSERT(play(0xA11CE5EDu, &watched, &watchedHash, &watchedFrames), "the watched run must stay healthy");
    ASSERT(play(0xA11CE5EDu, &unwatched, &unwatchedHash, &unwatchedFrames), "the unwatched run must stay healthy");

    int outs = 0, fouls = 0;
    for (int i = 0; i < watched.count; i++) {
        if (watched.event[i] == EVENT_OUT) outs++;
        if (watched.event[i] == EVENT_OUT_OF_BOUNDS) fouls++;
    }
    printf("    announcements=%d (outs=%d out-of-bounds=%d) frames=%ld\n", watched.count, outs, fouls, watchedFrames);

    // The claim is about announcements, so there have to be some — and among them the foul's, which is the
    // one that asks whether anything has been announced before it speaks, and so the one a consumer used
    // to be able to change. (A STRIKE is announced only when nobody swings, which the AI almost never does.)
    ASSERT(outs > 0 && fouls > 0, "the run must announce outs and an out-of-bounds to mean anything");

    ASSERT_EQ(watchedFrames, unwatchedFrames, "a screen consuming announcements must not change the game's length");
    ASSERT(watchedHash == unwatchedHash, "a screen consuming announcements must not change the game");
    ASSERT_EQ(watched.count, unwatched.count, "nobody watching must not change what the referee announces");
    for (int i = 0; i < watched.count; i++) {
        ASSERT_EQ(watched.frame[i], unwatched.frame[i], "each announcement must come on the same frame");
        ASSERT_EQ((int)watched.event[i], (int)unwatched.event[i], "each announcement must say the same thing");
    }

    // And the screen's side, held where it is cheap to hold: an announcement starts the info line and is
    // consumed, so the next tick starts with nothing standing.
    HudState hud = {.infoEvent = EVENT_NONE, .infoEventTimer = -1};
    GameEventType announcement = EVENT_STRIKE;
    hud_take_announcement(&hud, &announcement);
    ASSERT_EQ(EVENT_STRIKE, (int)hud.infoEvent, "the info line shows what was announced");
    ASSERT_EQ(0, hud.infoEventTimer, "and starts its clock");
    ASSERT_EQ(EVENT_NONE, (int)announcement, "and the announcement is consumed");
    hud_take_announcement(&hud, &announcement);
    ASSERT_EQ(EVENT_STRIKE, (int)hud.infoEvent, "an empty tick leaves the line as it was");
    return TEST_PASSED;
}
