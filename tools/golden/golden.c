// Golden traces — the instrument for a refactor that must not change behaviour.
//
// Each run plays a real game headless and writes one line per frame: the frame number and twelve section
// hashes over a wide projection of the World and the client widgets (players, ball, possession, flags,
// flow, the pitch, the throw, the swing, the whole legal world, the human widgets and the bars they draw,
// the AI's memory, the tick's outputs). Two builds that play the same game write the same file; when they
// do not, firstdiff.py names the first frame and the sections that moved, and a run given a frame range
// prints those sections in full.
//
// The runs: six AI-vs-AI seeds, and twelve seeds each of a fuzzed human catching, batting, and on both
// sides — a key-stream generator that knows enough of the game (a ready pitcher, a batter up, a loose ball)
// to reach pitches, fakes, throws and swings rather than wander. It is deterministic: same build, same file.
//
// projection.h is the version-specific half: how this tree's structs answer the semantic questions. A
// commit that reshapes a struct edits it so that it prints the same thing from the new shape, and the
// traces of the commit before and after can then be compared directly. See record.sh.
#include "scripted_harness.h"
#include "sim_harness.h"
#include "globals.h"
#include "action_invocations.h"
#include "rules_pure/player_utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NSEC 12
static const char* SEC_NAMES[NSEC] = {"players", "ball", "pII", "pRAI", "flow", "pitch",
                                      "throw", "swing", "rules", "client", "ai", "out"};
static char buf[NSEC][1 << 16];
static int len[NSEC];

static void put(int s, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    len[s] += vsnprintf(buf[s] + len[s], sizeof(buf[s]) - len[s], fmt, ap);
    va_end(ap);
}

static unsigned long long fnv(const void* p, size_t n, unsigned long long h)
{
    const unsigned char* c = p;
    for (size_t i = 0; i < n; i++) {
        h ^= c[i];
        h *= 1099511628211ULL;
    }
    return h;
}

#define V(s, v) put(s, "%a,%a,%a;", (double)(v).x, (double)(v).y, (double)(v).z)

#include "projection.h"

typedef struct {
    long pitches, fakes, throws, windups, hits, frames, walkOffers, walkOffersMidWindup;
    int lastCca, lastHit, lastWalk;
} Coverage;

static FILE* out;
static long dumpFrom = -1, dumpTo = -1;
static Coverage cov;

static void observe(const SimGame* g, void* ctx)
{
    (void)ctx;
    for (int s = 0; s < NSEC; s++) len[s] = 0, buf[s][0] = 0;
    project(g->state);
    fprintf(out, "%ld", g->frame);
    for (int s = 0; s < NSEC; s++) fprintf(out, " %016llx", fnv(buf[s], len[s], 1469598103934665603ULL));
    fprintf(out, "\n");
    if (g->frame >= dumpFrom && g->frame <= dumpTo) {
        for (int s = 0; s < NSEC; s++) fprintf(stderr, "[%ld %s] %s\n", g->frame, SEC_NAMES[s], buf[s]);
    }
    const MatchSession* m = g->state->match;
    int cca = catching_action_code(m);
    int airborne = m->pRAI.pitch_state == PITCH_STAGE_AIRBORNE;
    int hit = g->state->rules->betweenPitchState.batOutcome == BAT_OUTCOME_HIT;
    if (cca == 2 && cov.lastCca != 2) cov.windups++;
    if (cca == 1 && cov.lastCca != 1) cov.throws++;
    if (cov.lastCca == 2 && cca != 2) {
        if (airborne) cov.pitches++;
        else cov.fakes++;
    }
    if (hit && !cov.lastHit) cov.hits++;
    int walk = m->flowControl.waitingForFreeWalkDecision;
    if (walk && !cov.lastWalk) {
        cov.walkOffers++;
        if (cca == 2) cov.walkOffersMidWindup++;
    }
    cov.lastWalk = walk;
    cov.lastHit = hit;
    cov.lastCca = cca;
    cov.frames++;
}

// ---- the fuzzed human ---------------------------------------------------------------------------
typedef struct {
    unsigned int rng;
} Fuzzer;

static unsigned int xr(Fuzzer* f)
{
    f->rng ^= f->rng << 13;
    f->rng ^= f->rng >> 17;
    f->rng ^= f->rng << 5;
    return f->rng;
}
static int rnd(Fuzzer* f, int n) { return (int)(xr(f) % (unsigned)n); }

#define MAXQ 400
typedef struct {
    int mask[MAXQ];
    int n, at;
} Queue;

static void q_hold(Queue* q, int mask, int frames)
{
    for (int i = 0; i < frames && q->n < MAXQ; i++) q->mask[q->n++] = mask;
}

static const int ARROWS[4] = {KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT};

// What situation is this pad's team in? 1 = it controls a pitcher ready at home, 2 = its batter is up.
static int situation(const StateInfo* st, int pad)
{
    const MatchSession* m = st->match;
    const Scoreboard* sb = &st->rules->scoreboard;
    int batting = get_batting_team_index(sb);
    int catching = (batting + 1) % 2;
    int p = m->pII.catcherOnBaseIndex[0];
    if ((int)sb->teams[catching].control == pad && p != -1 && m->pII.hasBallIndex == p && m->pII.controlIndex == p &&
        m->playerInfo[p].cTPI.isNearHomeLocation)
        return 1;
    if ((int)sb->teams[batting].control == pad && m->pRAI.batter_ready) return 2;
    if ((int)sb->teams[catching].control == pad && m->pII.controlIndex != -1) {
        if (m->pII.hasBallIndex == m->pII.controlIndex) return 3; // holding it away from the plate
        if (m->pII.hasBallIndex == -1) {
            const Vector3D* at = &m->playerInfo[m->pII.controlIndex].tPI.location;
            const Vector3D* to = &m->catchingState.ballTargetPoint;
            float dx = to->x - at->x, dz = to->z - at->z;
            int arrows = 0;
            if (dx > 0.3f) arrows |= 1 << KEY_RIGHT;
            if (dx < -0.3f) arrows |= 1 << KEY_LEFT;
            if (dz > 0.3f) arrows |= 1 << KEY_DOWN;
            if (dz < -0.3f) arrows |= 1 << KEY_UP;
            return 16 + arrows; // chase, with the arrows toward the ball folded in
        }
    }
    return 0;
}

static void plan_gesture(Fuzzer* f, Queue* q, int sit)
{
    q->n = q->at = 0;
    int a = 1 << ARROWS[rnd(f, 4)];
    int k2 = 1 << KEY_2, k1 = 1 << KEY_1;
    int pick = rnd(f, 14);
    if (sit == 1) pick = (int[]){2, 2, 3, 12, 12, 1, 6, 5, 7, 0}[rnd(f, 10)];
    if (sit == 2) pick = (int[]){2, 3, 3, 1, 10, 11, 0, 9}[rnd(f, 8)];
    if (sit == 3) {
        if (rnd(f, 3) != 0) {
            q_hold(q, k2 | (1 << (rnd(f, 3) ? KEY_DOWN : ARROWS[rnd(f, 4)])), 1 + rnd(f, 50));
            return;
        }
    }
    if (sit >= 16) {
        if (rnd(f, 4) != 0) {
            q_hold(q, sit - 16, 3 + rnd(f, 20));
            return;
        }
    }
    switch (pick) {
    case 0: q_hold(q, 0, 1 + rnd(f, 60)); break; // idle
    case 1: q_hold(q, k2, 1 + rnd(f, 3)); break; // tap
    case 2: // the pitch / swing dance: tap, wait, tap, wait, tap
    case 3:
        q_hold(q, k2, 1 + rnd(f, 2));
        q_hold(q, 0, 1 + rnd(f, 70));
        q_hold(q, k2, 1 + rnd(f, 2));
        q_hold(q, 0, rnd(f, 90));
        q_hold(q, k2, 1 + rnd(f, 2));
        break;
    case 4: q_hold(q, k2, 1 + rnd(f, 80)); break; // long hold
    case 5: q_hold(q, k2 | a, 1 + rnd(f, 70)); break; // throw gesture
    case 6: // Enter first, then an arrow joins
        q_hold(q, k2, 1 + rnd(f, 6));
        q_hold(q, k2 | a, 1 + rnd(f, 60));
        break;
    case 7: // a throw released, then the key pressed again during the windup
        q_hold(q, k2 | a, 1 + rnd(f, 10));
        q_hold(q, 0, rnd(f, 4));
        q_hold(q, k2, 1 + rnd(f, 3));
        q_hold(q, 0, rnd(f, 6));
        q_hold(q, k2, 1 + rnd(f, 3));
        break;
    case 8: q_hold(q, a | (rnd(f, 3) == 0 ? 1 << ARROWS[rnd(f, 4)] : 0), 1 + rnd(f, 90)); break; // move
    case 9: // double tap an arrow (run now)
        q_hold(q, a, 1 + rnd(f, 3));
        q_hold(q, 0, 1 + rnd(f, 8));
        q_hold(q, a, 1 + rnd(f, 3));
        break;
    case 10: q_hold(q, k1, 1 + rnd(f, 3)); break; // withdraw / change batter / refuse walk
    case 11: q_hold(q, 1 << (rnd(f, 2) ? KEY_PLUS : KEY_MINUS), 1 + rnd(f, 40)); break; // aim
    case 12: // pitch power then abandon the aim (valesyöttö)
        q_hold(q, k2, 1);
        q_hold(q, 0, 1 + rnd(f, 60));
        q_hold(q, k2, 1);
        q_hold(q, 0, 150);
        break;
    default: q_hold(q, 0, 1 + rnd(f, 20)); break;
    }
}

static int run_scripted(TeamControlMode c1, TeamControlMode c2, unsigned int seed, unsigned int fuzzSeed, long cap)
{
    ScriptedGame* g = scripted_create(0, 1, c1, c2, seed);
    if (!g) return 1;
    scripted_attach(g, observe, NULL);
    Fuzzer f[2] = {{.rng = fuzzSeed | 1u}, {.rng = (fuzzSeed * 2654435761u) | 1u}};
    Queue q[2];
    memset(q, 0, sizeof(q));
    for (long i = 0; i < cap; i++) {
        for (int pad = 0; pad < 2; pad++) {
            if (q[pad].at >= q[pad].n) plan_gesture(&f[pad], &q[pad], situation(scripted_state(g), pad));
            int mask = q[pad].mask[q[pad].at++];
            for (int k = 0; k < KEY_COUNT; k++) {
                if (k == KEY_HOME) continue;
                if (mask & (1 << k)) scripted_hold(g, pad, k);
                else scripted_release(g, pad, k);
            }
        }
        if (!scripted_tick(g)) break;
    }
    fprintf(out, "# end frame=%ld failed=%d reason=%s\n", scripted_frame(g), scripted_failed(g),
            scripted_failed(g) ? scripted_fail_reason(g) : "-");
    scripted_destroy(g);
    return 0;
}

static int run_sim(unsigned int seed, long cap)
{
    GameSetup setup;
    sim_make_normal_setup(&setup, 0, 1, CONTROL_AI, CONTROL_AI);
    SimGame* g = sim_create(&setup, seed);
    if (!g) return 1;
    sim_attach(g, observe, NULL);
    for (long i = 0; i < cap; i++) {
        if (!sim_tick(g)) break;
    }
    fprintf(out, "# end frame=%ld failed=%d reason=%s\n", g->frame, g->failed, g->failed ? g->fail_reason : "-");
    sim_destroy(g);
    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: golden OUTDIR RUN [dumpFrom dumpTo]   (RUN: sim0-5, catch0-11, bat0-11, h2h0-11)\n");
        return 2;
    }
    const char* dir = argv[1];
    const char* run = argv[2];
    if (argc >= 5) dumpFrom = atol(argv[3]), dumpTo = atol(argv[4]);
    char path[512];
    snprintf(path, sizeof(path), "%s/%s.trace", dir, run);
    out = fopen(path, "w");
    if (!out) return 2;
    memset(&cov, 0, sizeof(cov));

    unsigned int n = 0;
    int rc = 1;
    if (sscanf(run, "sim%u", &n) == 1) {
        static const unsigned int seeds[] = {0x1234ABCDu, 0x0000AAAAu, 0xBBBB0000u, 0x5EED0001u, 0x5EED0002u, 0x5EED0003u};
        rc = run_sim(seeds[n % 6], 60000);
    } else if (sscanf(run, "catch%u", &n) == 1) {
        rc = run_scripted(CONTROL_AI, CONTROL_PLAYER_1, 0xC0000000u + n, 0xF0000000u + n * 7919u, 40000);
    } else if (sscanf(run, "bat%u", &n) == 1) {
        rc = run_scripted(CONTROL_PLAYER_1, CONTROL_AI, 0xB0000000u + n, 0xE0000000u + n * 7919u, 40000);
    } else if (sscanf(run, "h2h%u", &n) == 1) {
        rc = run_scripted(CONTROL_PLAYER_1, CONTROL_PLAYER_2, 0xD0000000u + n, 0xD1000000u + n * 7919u, 40000);
    }
    fprintf(out, "# coverage frames=%ld windups=%ld pitches=%ld fakes=%ld throws=%ld hits=%ld\n", cov.frames,
            cov.windups, cov.pitches, cov.fakes, cov.throws, cov.hits);
    fprintf(out, "# walks offers=%ld midWindup=%ld\n", cov.walkOffers, cov.walkOffersMidWindup);
    fclose(out);
    return rc;
}
