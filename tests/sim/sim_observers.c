#include "sim_observers.h"

#include <string.h>

int sim_runners_on_base(const SimGame* g)
{
    const MatchSession* m = g->state->match;
    int n = 0;
    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        BaseID b = m->playerInfo[i].bTPI.baseId;
        if (b == BASE_FIRST || b == BASE_SECOND || b == BASE_THIRD) n++;
    }
    return n;
}

/* ---- Invariant observer ------------------------------------------------ */

void invariant_observer_init(InvariantObserver* o, long stall_limit)
{
    memset(o, 0, sizeof(*o));
    o->stall_limit = stall_limit;
}

void invariant_observer_hook(const SimGame* g, void* ctx)
{
    InvariantObserver* o = (InvariantObserver*)ctx;
    const GameRulesState* r = g->state->rules;
    const HalfInningState* h = &r->halfInningState;
    const Scoreboard* sb = &r->scoreboard;

    int outs = h->outs;
    int balls = h->balls;
    int strikes = h->strikes;
    int runs0 = sb->teams[0].runs;
    int runs1 = sb->teams[1].runs;
    int inning = sb->inning;
    int hasBall = g->state->match->pII.hasBallIndex;
    int pitchState = (int)g->state->match->pRAI.pitchState;

    // --- Bounds (be conservative: only flag clearly-illegal values) ---
    // The 3-out rule applies to normal/super innings (period < 4); the homerun
    // contest (period >= 4) accumulates outs differently, so only floor it there.
    if (outs < 0 || (sb->period < 4 && outs > 3)) {
        sim_fail((SimGame*)g, "invariant: outs out of range for the current mode");
        return;
    }
    if (strikes < 0 || strikes > 3) {
        sim_fail((SimGame*)g, "invariant: strikes out of [0,3]");
        return;
    }
    if (balls < 0) {
        sim_fail((SimGame*)g, "invariant: balls negative");
        return;
    }
    if (runs0 < 0 || runs1 < 0) {
        sim_fail((SimGame*)g, "invariant: negative run total");
        return;
    }

    if (!o->initialized) {
        o->initialized = 1;
        o->last_progress_frame = g->frame;
        o->p_outs = outs;
        o->p_balls = balls;
        o->p_strikes = strikes;
        o->p_runs0 = runs0;
        o->p_runs1 = runs1;
        o->p_inning = inning;
        o->p_hasBall = hasBall;
        o->p_pitchState = pitchState;
        return;
    }

    // Count a pitch on the rising edge into AIRBORNE. (gameEvents.pitchReleased can't be
    // used here: update_game_frame clears frame events before observers run.)
    if (pitchState == PITCH_STAGE_AIRBORNE && o->p_pitchState != PITCH_STAGE_AIRBORNE) {
        o->pitches++;
    }

    // --- Monotonicity (runs/inning never go backwards) ---
    if (runs0 < o->p_runs0 || runs1 < o->p_runs1) {
        sim_fail((SimGame*)g, "invariant: run total decreased");
        return;
    }
    if (inning < o->p_inning) {
        sim_fail((SimGame*)g, "invariant: inning counter decreased");
        return;
    }

    // --- Progress / liveness ---
    int count_changed = (outs != o->p_outs) || (balls != o->p_balls) || (strikes != o->p_strikes) ||
                        (runs0 != o->p_runs0) || (runs1 != o->p_runs1) || (inning != o->p_inning);
    if (count_changed) o->count_changes++;

    int progressed = count_changed || (hasBall != o->p_hasBall) || (pitchState != o->p_pitchState);
    if (progressed) {
        o->last_progress_frame = g->frame;
    } else if (o->stall_limit > 0 && (g->frame - o->last_progress_frame) > o->stall_limit) {
        sim_fail((SimGame*)g, "invariant: no progress for stall_limit frames (game stuck)");
        return;
    }

    o->p_outs = outs;
    o->p_balls = balls;
    o->p_strikes = strikes;
    o->p_runs0 = runs0;
    o->p_runs1 = runs1;
    o->p_inning = inning;
    o->p_hasBall = hasBall;
    o->p_pitchState = pitchState;
}

/* ---- Trace observer ---------------------------------------------------- */

void trace_observer_init(TraceObserver* o, FILE* f, long every)
{
    memset(o, 0, sizeof(*o));
    o->f = f;
    o->every = (every > 0) ? every : 1;
}

void trace_observer_hook(const SimGame* g, void* ctx)
{
    TraceObserver* o = (TraceObserver*)ctx;
    if (!o->f) return;

    if (!o->header_written) {
        fprintf(
            o->f, "frame,period,inning,outs,balls,strikes,runs0,runs1,onBase,ballHome,"
                  "pitchState,catchingAction,hasBall,"
                  "waitBatter,waitWalk,batterReady,batterSel,planCalc,aiPitchStage,aiEventLock,batterReadyTimer,"
                  "meterCnt,swing,wrongPitch,batStyle,batOutcome,battingGoingOn\n"
        );
        o->header_written = 1;
    }

    if (o->every > 1 && (g->frame % o->every) != 0 && !g->state->match->gameEvents.pitchReleased) {
        return; // sample, but never drop a pitch-release frame
    }

    const MatchSession* m = g->state->match;
    const GameRulesState* r = g->state->rules;
    fprintf(
        o->f, "%ld,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", g->frame,
        r->scoreboard.period, r->scoreboard.inning, r->halfInningState.outs, r->halfInningState.balls,
        r->halfInningState.strikes, r->scoreboard.teams[0].runs, r->scoreboard.teams[1].runs, sim_runners_on_base(g),
        m->gameFlowState.ballHome, (int)m->pRAI.pitchState, (int)m->pendingActionState.currentCatchingAction,
        m->pII.hasBallIndex, m->flowControl.waitingForBatterDecision, m->flowControl.waitingForFreeWalkDecision,
        m->pRAI.batterReady, m->pII.batterSelectionIndex, m->aiState.planCalculated, m->aiState.pitchStage,
        (int)m->pendingActionState.aiActionEventLock, m->aiState.batterReadyTimer, m->pendingActionState.meterCounter,
        (int)m->aF.bTAF.swing, m->aiState.aiWrongPitch, m->aiState.battingStyle, (int)r->betweenPitchState.batOutcome,
        m->pRAI.battingGoingOn
    );
}

/* ---- Checksum observer ------------------------------------------------- */

void checksum_observer_init(ChecksumObserver* o)
{
    o->hash = 1469598103934665603ULL; // FNV-1a 64-bit offset basis
}

static void fold_bytes(unsigned long long* h, const void* p, size_t n)
{
    const unsigned char* b = (const unsigned char*)p;
    unsigned long long x = *h;
    for (size_t i = 0; i < n; i++) {
        x ^= b[i];
        x *= 1099511628211ULL; // FNV-1a 64-bit prime
    }
    *h = x;
}
#define FOLD(h, v) fold_bytes(&(h), &(v), sizeof(v))

void checksum_observer_hook(const SimGame* g, void* ctx)
{
    // Fingerprint *curated game-meaningful state*, not raw MatchSession bytes: the player
    // structs embed char* name pointers whose heap addresses differ between runs, which
    // would make an honest, deterministic game look nondeterministic. These fields are the
    // actual simulation state.
    ChecksumObserver* o = (ChecksumObserver*)ctx;
    const MatchSession* m = g->state->match;
    const GameRulesState* r = g->state->rules;
    unsigned long long h = o->hash;

    for (int i = 0; i < 2 * PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        const PlayerInfo* p = &m->playerInfo[i];
        FOLD(h, p->tPI.location.x);
        FOLD(h, p->tPI.location.y);
        FOLD(h, p->tPI.location.z);
        FOLD(h, p->bTPI.baseId);
        FOLD(h, p->bTPI.state);
    }
    FOLD(h, m->ballInfo.location.x);
    FOLD(h, m->ballInfo.location.y);
    FOLD(h, m->ballInfo.location.z);
    FOLD(h, m->ballInfo.velocity.x);
    FOLD(h, m->ballInfo.velocity.y);
    FOLD(h, m->ballInfo.velocity.z);
    FOLD(h, m->ballInfo.moving);
    FOLD(h, m->ballInfo.onGround);
    FOLD(h, m->pII.hasBallIndex);
    FOLD(h, m->pII.controlIndex);
    FOLD(h, m->pRAI.pitchState);
    FOLD(h, m->pendingActionState.currentCatchingAction);
    FOLD(h, m->pendingActionState.meterCounter);

    FOLD(h, r->halfInningState.outs);
    FOLD(h, r->halfInningState.balls);
    FOLD(h, r->halfInningState.strikes);
    FOLD(h, r->scoreboard.inning);
    FOLD(h, r->scoreboard.period);
    FOLD(h, r->scoreboard.teams[0].runs);
    FOLD(h, r->scoreboard.teams[1].runs);
    FOLD(h, r->scoreboard.teams[0].batterOrderIndex);
    FOLD(h, r->scoreboard.teams[1].batterOrderIndex);

    o->hash = h;
}
