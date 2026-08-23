#include "sim_observers.h"
#include "actions/batting_system.h" // BAT_SWING_MAX / BAT_LOAD_MAX for power intent units

#include <stdlib.h>
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
    int pitch_state = (int)g->state->match->pRAI.pitch_state;

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
    if (sb->period < 4 && (h->jokersLeft < 0 || h->jokersLeft > JOKER_COUNT)) {
        sim_fail((SimGame*)g, "invariant: jokers left out of [0, JOKER_COUNT]");
        return;
    }

    // §12: once the referee has pronounced the batting turn spent, only a joker may extend it. A
    // regular batter appearing here is bug #7's shape — somebody handed the bat that the rules had
    // already finished with.
    //
    // MEASURED 2026-08-23 and kept anyway: today this branch is never entered. A probe that failed
    // the sim on `turnExhausted` alone did not fire on any seed, because AI-vs-AI half-innings end
    // on three burns every single time — the same blindness PLAN.md §8.2 records for the scoring
    // half of the rules, now known to cover the side change as well. So this is a net for the
    // future, not a net today, and the honest place to say so is here rather than in a doc.
    if (sb->period < 4 && h->lastBatter.turnExhausted) {
        for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
            const PlayerInfo* p = &g->state->match->playerInfo[i];
            if (p->bTPI.state == PLAYER_STATE_AT_BAT && p->bTPI.joker == JOKER_REGULAR) {
                sim_fail((SimGame*)g, "invariant: a regular batter took the bat after §12 spent the turn");
                return;
            }
        }
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
        o->p_pitchState = pitch_state;
        return;
    }

    // Count a pitch on the rising edge into AIRBORNE. (gameEvents.pitchReleased can't be
    // used here: update_game_frame clears frame events before observers run.)
    if (pitch_state == PITCH_STAGE_AIRBORNE && o->p_pitchState != PITCH_STAGE_AIRBORNE) {
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

    int progressed = count_changed || (hasBall != o->p_hasBall) || (pitch_state != o->p_pitchState);
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
    o->p_pitchState = pitch_state;
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
                  "pitch_state,catchingAction,hasBall,"
                  "waitBatter,waitWalk,batter_ready,batterSel,planCalc,pitchDeclPhase,pitchTimer,batterReadyTimer,"
                  "meterCnt,swing,wrongPitch,batStyle,batOutcome,batting_going_on\n"
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
        m->gameFlowState.ballHome, (int)m->pRAI.pitch_state, (int)m->pendingActionState.current_catching_action,
        m->pII.hasBallIndex, m->flowControl.waitingForBatterDecision, m->flowControl.waitingForFreeWalkDecision,
        m->pRAI.batter_ready, m->pII.batterSelectionIndex, m->aiState.planCalculated, (int)m->aF.cTAF.pitch.phase,
        m->pendingActionState.pitchActualization.timer, m->aiState.batterReadyTimer,
        m->pendingActionState.meter_counter, (int)m->aF.bTAF.swing, m->aiState.aiWrongPitch, m->aiState.battingStyle,
        (int)r->betweenPitchState.batOutcome, m->pRAI.batting_going_on
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
    FOLD(h, m->pRAI.pitch_state);
    FOLD(h, m->pendingActionState.current_catching_action);
    FOLD(h, m->pendingActionState.meter_counter);

    // The engine's random stream is World state: it determines every future engine draw, so two
    // worlds that agree on everything else but not on this do NOT have the same future. The AI
    // controller's seed is deliberately NOT folded in — it lives outside the World by design.
    FOLD(h, m->rngSeed);

    FOLD(h, r->halfInningState.outs);
    FOLD(h, r->halfInningState.balls);
    FOLD(h, r->halfInningState.strikes);
    FOLD(h, r->scoreboard.inning);
    FOLD(h, r->scoreboard.period);
    FOLD(h, r->scoreboard.teams[0].runs);
    FOLD(h, r->scoreboard.teams[1].runs);
    FOLD(h, r->scoreboard.teams[0].batterOrderIndex);
    FOLD(h, r->scoreboard.teams[1].batterOrderIndex);

    // §12's own state. The batting order alone does not say when the turn ends: the designation, the
    // joker count and the pronounced verdict each determine a different future, so a checksum that
    // omitted them would call two genuinely different half-innings identical. (The counters this
    // replaced were never folded — the omission is being closed, not inherited.)
    FOLD(h, r->halfInningState.jokersLeft);
    FOLD(h, r->halfInningState.lastBatter.designatedIndex);
    FOLD(h, r->halfInningState.lastBatter.lastRegularIndex);
    FOLD(h, r->halfInningState.lastBatter.hasBattedAgain);
    FOLD(h, r->halfInningState.lastBatter.turnExhausted);

    o->hash = h;
}

/* ---- Box-score observer ------------------------------------------------ */

#define BOX_PLAYER_COUNT (2 * PLAYERS_IN_TEAM + JOKER_COUNT)

void box_score_observer_init(BoxScoreObserver* o, FILE* log)
{
    memset(o, 0, sizeof(*o));
    o->log = log;
    o->contact_power_min = 9999; // so the first contact sets a real minimum
    o->contact_power_max = -9999;
}

// Play-by-play line, prefixed with where we are in the game. No-op when silent.
static void pbp(const BoxScoreObserver* o, const SimGame* g, const GameRulesState* r, const char* msg)
{
    if (!o->log) return;
    fprintf(
        o->log, "  f%-6ld P%d I%d %d-%d %dout | %s\n", g->frame, r->scoreboard.period, r->scoreboard.inning,
        r->halfInningState.balls, r->halfInningState.strikes, r->halfInningState.outs, msg
    );
}

void box_score_observer_hook(const SimGame* g, void* ctx)
{
    BoxScoreObserver* o = (BoxScoreObserver*)ctx;
    const MatchSession* m = g->state->match;
    const GameRulesState* r = g->state->rules;
    const HalfInningState* h = &r->halfInningState;
    const Scoreboard* sb = &r->scoreboard;

    int pitch_state = (int)m->pRAI.pitch_state;
    int batOutcome = (int)r->betweenPitchState.batOutcome;
    int foulState = (int)r->referee.foulState;

    if (!o->initialized) {
        o->initialized = 1;
        o->p_pitchState = pitch_state;
        o->p_batOutcome = batOutcome;
        o->p_foulState = foulState;
        o->p_outs = h->outs;
        o->p_balls = h->balls;
        o->p_strikes = h->strikes;
        o->p_runs0 = sb->teams[0].runs;
        o->p_runs1 = sb->teams[1].runs;
        o->p_inning = sb->inning;
        o->p_period = sb->period;
        o->p_throwing = (m->pendingActionState.current_catching_action == CATCHING_ACTION_THROWING);
        for (int i = 0; i < BOX_PLAYER_COUNT; i++) {
            o->p_baseId[i] = (int)m->playerInfo[i].bTPI.baseId;
            o->p_state[i] = (int)m->playerInfo[i].bTPI.state;
        }
        return;
    }

    // Inning / period rollover — a natural section header for the play-by-play.
    if (sb->inning != o->p_inning || sb->period != o->p_period) {
        char buf[64];
        snprintf(buf, sizeof(buf), "--- half-inning over (now period %d, inning %d) ---", sb->period, sb->inning);
        pbp(o, g, r, buf);
    }

    // Pitch released: rising edge into AIRBORNE (gameEvents.pitchReleased is already cleared).
    if (pitch_state == PITCH_STAGE_AIRBORNE && o->p_pitchState != PITCH_STAGE_AIRBORNE) {
        o->pitches++;
        pbp(o, g, r, "pitch released");
    }

    // Contact / whiff: batOutcome is sticky per pitch, so count the transition into it.
    if (batOutcome != o->p_batOutcome) {
        if (batOutcome == BAT_OUTCOME_HIT) {
            o->contacts++;
            // Contact only — fair/foul is the referee's later call (a foul shows up as a STRIKE).
            pbp(o, g, r, "contact (bat meets ball)");
            // Actualized power and direction of THIS batted ball (all styles), for AI tuning.
            int power = m->pendingActionState.selected_batting_power_count;
            o->contact_power_sum += power;
            o->contact_power_n++;
            if (power < o->contact_power_min) o->contact_power_min = power;
            if (power > o->contact_power_max) o->contact_power_max = power;
            // Realized horizontal launch direction of the batted ball, STYLE-1 only (the normal
            // swing — the style whose direction is meant to be a uniform fan). We read the ACTUAL
            // ball velocity, not batter_angle: batter_angle is reset to 0 around contact, so reading
            // it here would fake a collapse to center. atan2(vx, -vz) is the true flight heading
            // (0 = straight to centre, + = one side) and is constant after launch (gravity only
            // touches vy). dir_bins splits ±1.0 rad into 5 buckets (right→left).
            if (m->aiState.battingStyle == 1) {
                float heading = atan2f(m->ballInfo.velocity.x, -m->ballInfo.velocity.z);
                int bin = (int)((heading + 1.0f) * 2.5f); // [-1,1] rad -> [0,5)
                if (bin < 0) bin = 0;
                if (bin > 4) bin = 4;
                o->dir_bins[bin]++;
            }
        } else if (batOutcome == BAT_OUTCOME_MISSED) {
            o->whiffs++;
            pbp(o, g, r, "swing and a miss");
        }
        // A swing just resolved — measure how well the AI hit its intended power, but ONLY for
        // batting style 1: that is the only style where `decidedSwingTrigger` is a real intent
        // (styles 0/2 leave it stale, so measuring them here would be meaningless). The AI aims to
        // release at meter level `decidedSwingTrigger`; realized power (same units, shifted by the
        // load offset) is `selected_batting_power_count`. err should be ≈ +1 (releases one tick late).
        if ((batOutcome == BAT_OUTCOME_HIT || batOutcome == BAT_OUTCOME_MISSED) && m->aiState.battingStyle == 1) {
            int intent = m->aiState.decidedSwingTrigger - (BAT_SWING_MAX - BAT_LOAD_MAX);
            int actual = m->pendingActionState.selected_batting_power_count;
            int err = actual - intent;
            o->s1_swings++;
            o->s1_power_err_sum += err;
            if (o->log) {
                char buf[96];
                snprintf(buf, sizeof(buf), "  style-1 swing power: intent=%d actual=%d (err %+d)", intent, actual, err);
                pbp(o, g, r, buf);
            }
        }
    }

    // Count deltas (only increments are real events; resets to 0 are bookkeeping).
    if (h->strikes > o->p_strikes) {
        o->strikes_called += h->strikes - o->p_strikes;
        pbp(o, g, r, "STRIKE");
    }
    if (h->balls > o->p_balls) {
        o->balls_called += h->balls - o->p_balls;
        pbp(o, g, r, "BALL");
    }
    if (h->outs > o->p_outs) {
        o->outs_made += h->outs - o->p_outs;
        pbp(o, g, r, "OUT");
    }
    if (sb->teams[0].runs > o->p_runs0 || sb->teams[1].runs > o->p_runs1) {
        o->runs_scored += (sb->teams[0].runs - o->p_runs0) + (sb->teams[1].runs - o->p_runs1);
        char buf[64];
        snprintf(buf, sizeof(buf), "RUN scored! (%d–%d)", sb->teams[0].runs, sb->teams[1].runs);
        pbp(o, g, r, buf);
    }

    // Foul / out of bounds: the referee arms FOUL_STATE_DETECTED when a batted ball lands outside
    // the lines. Counting the rising edge gives the "actualized out of bounds" rate (fouls/contacts).
    if (foulState == FOUL_STATE_DETECTED && o->p_foulState != FOUL_STATE_DETECTED) {
        o->fouls++;
        pbp(o, g, r, "FOUL (out of bounds)");
    }

    // Throw to a base started: a rising edge into CATCHING_ACTION_THROWING, which only
    // begin_throw_windup raises. (A drop counter was tried here and removed — a §30 drop's post-frame
    // state aliases a reset that also clears a fielder's ball; see sim_observers.h.)
    int throwing = (m->pendingActionState.current_catching_action == CATCHING_ACTION_THROWING);
    if (throwing == 1 && o->p_throwing == 0) {
        o->throws++;
        pbp(o, g, r, "throw to a base (windup)");
    }
    o->p_throwing = throwing;

    // Base running. We want to answer: when a runner reaches 3rd, do they try for home, and if
    // so do they score, get thrown out, get wounded — or just strand? A runner heading home keeps
    // baseId==THIRD (the source) until they touch home (BASE_HOME_SCORED), so "running from third"
    // = at base 3 in a RUNNING state.
    for (int i = 0; i < BOX_PLAYER_COUNT; i++) {
        int base = (int)m->playerInfo[i].bTPI.baseId;
        int prev = o->p_baseId[i];
        int state = (int)m->playerInfo[i].bTPI.state;
        int pstate = o->p_state[i];

        if (base != prev) {
            if (prev == BASE_HOME && base == BASE_FIRST) {
                o->reached_base++;
                pbp(o, g, r, "batter reached first");
            } else if (base == BASE_SECOND && prev != BASE_SECOND) {
                pbp(o, g, r, "runner reached second");
            } else if (base == BASE_THIRD && prev != BASE_THIRD) {
                o->reached_third++;
                pbp(o, g, r, "runner reached third");
            } else if (base == BASE_HOME_SCORED && prev == BASE_THIRD) {
                // KNOWN OVERCOUNT (found 2026-08-17, PLAN.md §3.3 F14 — recorded, not fixed here).
                // This is a PHYSICAL arrival at home, not a run: game_manipulation sets baseId to
                // BASE_HOME_SCORED even for a runner already WOUNDED or OUT (it only preserves the
                // state), while §41 / is_regular_run correctly denies the run to a wounded runner. So
                // this counter — and the word "SCORED" below — can fire with no run awarded, which is
                // exactly what slice 1a's run first surfaced (scored=1 alongside runs=0). The honest
                // run counter is `runs_scored`, fed from the scoreboard; the bands and PLAN.md §8.2
                // both rest on that one. Deliberately left alone in the slice that re-baselines
                // against this net: changing the instrument and the measurement in one commit makes
                // neither readable.
                o->scored_from_third++;
                pbp(o, g, r, "runner came home from third — SCORED");
            }
        }
        if ((base == BASE_FIRST || base == BASE_SECOND || base == BASE_THIRD) && base > o->furthest_base) {
            o->furthest_base = base;
        }
        // Broke for home: at 3rd and just entered RUNNING.
        if (base == BASE_THIRD && state == PLAYER_STATE_RUNNING && pstate != PLAYER_STATE_RUNNING) {
            o->ran_from_third++;
            pbp(o, g, r, "runner on third breaks for home");
        }
        // Lost while on/from third (the "thrown out at home" vs "stranded" distinction).
        int onThird = (base == BASE_THIRD || prev == BASE_THIRD);
        if (onThird && state == PLAYER_STATE_OUT && pstate != PLAYER_STATE_OUT) {
            o->out_from_third++;
            pbp(o, g, r, "runner from third is OUT");
        }
        if (onThird && state == PLAYER_STATE_WOUNDED && pstate != PLAYER_STATE_WOUNDED) {
            o->wound_from_third++;
            pbp(o, g, r, "runner from third is WOUNDED");
        }

        o->p_baseId[i] = base;
        o->p_state[i] = state;
    }

    o->p_pitchState = pitch_state;
    o->p_batOutcome = batOutcome;
    o->p_foulState = foulState;
    o->p_outs = h->outs;
    o->p_balls = h->balls;
    o->p_strikes = h->strikes;
    o->p_runs0 = sb->teams[0].runs;
    o->p_runs1 = sb->teams[1].runs;
    o->p_inning = sb->inning;
    o->p_period = sb->period;
}
