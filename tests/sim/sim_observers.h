#ifndef SIM_OBSERVERS_H
#define SIM_OBSERVERS_H

#include "sim_harness.h"
#include <stdio.h>

/**
 * @file sim_observers.h
 * @brief Built-in per-frame observers for the headless sim harness.
 *
 * Each observer is a (SimFrameHook, ctx) pair attached via sim_attach(). They are
 * the "attach to the running game and watch it" mechanism: assert invariants,
 * record a trace, or fingerprint the run for determinism checks.
 */

/* ---- Invariant observer ------------------------------------------------ *
 * Asserts the game never enters an invalid or stuck state. The production
 * state_validator (unique base occupancy, single batter) already guards the
 * frame via flowControl.pause; this adds bounds, monotonicity, and a no-stall
 * (liveness) check, and records that the game actually progressed. On a
 * violation it calls sim_fail(), which stops the run. */
typedef struct {
    long stall_limit; // max frames with no progress before declaring a stall

    // internal progress tracking (do not set)
    int initialized;
    long last_progress_frame;
    int p_outs, p_balls, p_strikes, p_runs0, p_runs1, p_inning, p_hasBall, p_pitchState;

    // observed totals (readable after the run for "game progressed" assertions)
    long pitches; // pitchReleased events seen
    long count_changes; // frames where outs/balls/strikes/runs/inning moved
} InvariantObserver;

void invariant_observer_init(InvariantObserver* o, long stall_limit);
void invariant_observer_hook(const SimGame* g, void* ctx);

/* ---- Trace observer ---------------------------------------------------- *
 * Appends one CSV row per (sampled) frame: the same key variables the debug
 * state dump tracks. This is the "see how the values develop" artifact and is
 * diffable across seeds and versions. */
typedef struct {
    FILE* f;
    long every; // sample period in frames (1 = every frame)
    int header_written;
} TraceObserver;

void trace_observer_init(TraceObserver* o, FILE* f, long every);
void trace_observer_hook(const SimGame* g, void* ctx);

/* ---- Checksum observer ------------------------------------------------- *
 * Folds a CURATED set of fields into a rolling FNV-1a hash each frame — NOT raw
 * MatchSession bytes (player structs embed char* names whose addresses differ per
 * run). Read checksum_observer_hook's field list before predicting whether a
 * change moves the hash: adding or removing a struct field does not, unless that
 * field is folded. */
typedef struct {
    unsigned long long hash;
} ChecksumObserver;

void checksum_observer_init(ChecksumObserver* o);
void checksum_observer_hook(const SimGame* g, void* ctx);

/* ---- Box-score observer ------------------------------------------------ *
 * Turns frame-by-frame state transitions into a baseball box score (and, if a
 * log file is attached, a human-readable play-by-play). The CSV trace answers
 * "what were the raw values each frame?"; this answers "what *happened* in the
 * game?" — so a headless run can be followed like a radio broadcast.
 *
 * Every counter is derived from a rising edge or a positive delta (game events
 * themselves are cleared before observers run), so it never double-counts. */
typedef struct {
    FILE* log; // optional play-by-play sink; NULL = count silently

    // internal edge-detection state (do not set)
    int initialized;
    int p_pitchState, p_outs, p_balls, p_strikes, p_runs0, p_runs1, p_inning, p_period;
    int p_batOutcome;
    int p_foulState;
    int p_throwing; // previous frame's (current_catching_action == CATCHING_ACTION_THROWING)
    int p_baseId[2 * PLAYERS_IN_TEAM + JOKER_COUNT];
    int p_state[2 * PLAYERS_IN_TEAM + JOKER_COUNT]; // PlayerUnitState last frame

    // box score (readable after the run)
    long pitches; // pitches released (rising edge into AIRBORNE)
    long throws; // throws to a base started (rising edge into CATCHING_ACTION_THROWING — only begin_throw_windup raises
                 // it)
    // NOTE: no `drops` counter. A §30 tactical drop's post-frame state (hasBallIndex −1, ball moving,
    // pitch NONE, throw 0) is indistinguishable from a half-inning / HR-pair RESET that clears a
    // fielder's ball — so any state-based drop count aliases resets (verified via PBP). The drop is
    // knighted by construction instead: tests/integration/contracts/test_ai_tactical_drop.c.
    long contacts; // bat made contact (batOutcome → HIT)
    long whiffs; // swing and a miss (batOutcome → MISSED)
    long fouls; // hits called foul / out of bounds (foulState NONE→DETECTED rising edge)
    long strikes_called; // strike-count increments
    long balls_called; // ball-count increments
    long outs_made; // out-count increments
    long runs_scored; // both teams' run increments
    long reached_base; // batters that became runners (HOME → 1st)
    int furthest_base; // furthest base any runner stood on (1..3), 0 if none

    // base-running breakdown — the "stranded vs thrown out vs never tried" question
    long reached_third; // distinct arrivals onto 3rd base
    long ran_from_third; // a runner on 3rd broke for home (entered RUNNING/LEADING)
    long scored_from_third; // a 3rd-base runner reached HOME_SCORED
    long out_from_third; // a 3rd-base runner went OUT while heading home
    long wound_from_third; // a 3rd-base runner was WOUNDED while heading home

    // Batting meter, STYLE-1 swings only. decidedSwingTrigger is the AI's real power intent only
    // for batting style 1 ("normal swing"); styles 0 (bunt) and 2 leave it stale, so their power
    // is NOT measurable this way and is deliberately excluded. Direction (decidedAngle vs the
    // realized batter_angle) is not measured here at all; measuring it wants an instrument of its own.
    long s1_swings; // style-1 swings measured
    long s1_power_err_sum; // Σ (actual − intent) in meter steps; ≈ +1 means the AI hit its target

    // Actualized batted-ball power and direction, over every contact (all styles). These answer the
    // two tuning questions directly: "are hits powerful enough?" and "is the direction a uniform
    // spread that lands fair most of the time?". Power is `selected_batting_power_count` (0..36).
    // Direction is the realized horizontal launch angle (= -batter_angle*2, the production formula);
    // dir_bins splits the reachable span [-1.0, +1.0] rad into 5 equal buckets (right→left) so a
    // collapse-to-center or a lopsided pull is visible at a glance.
    long contact_power_sum;
    long contact_power_n;
    int contact_power_min;
    int contact_power_max;
    long dir_bins[5];
} BoxScoreObserver;

void box_score_observer_init(BoxScoreObserver* o, FILE* log);
void box_score_observer_hook(const SimGame* g, void* ctx);

/* ---- Fielding observer ------------------------------------------------- *
 * Watches the catching side do its job. Every band in the offense breakdown is
 * batting- or pitching-side, so a fielding regression — a fielder that moves at
 * half speed, one that stops short of where it was sent, one that never sets off
 * at all — does not move a single one of them. These are the numbers that see it:
 *
 *   recovery       frames from bat contact until a fielder holds the ball again.
 *                  The end-to-end "how good is the defence" number.
 *   chase distance |controlled fielder - the engine's ball target point| each frame
 *                  the ball is loose. That point is the engine's own prediction of
 *                  where the ball can be met, so this is literally "did the fielder
 *                  go where it was sent". Falls when tracking improves.
 *   step           the controlled fielder's realized displacement per moving frame.
 *                  A pure speed probe: it pins the movement path to the speed it is
 *                  supposed to run at, and nothing else in the suite would notice a
 *                  fielder quietly switched onto a walk.
 *
 * A window opens at bat contact and closes on possession; one that never closes (a
 * foul, an out of bounds, a reset) is discarded rather than counted, so only real
 * recoveries reach the mean.
 *
 * Both position probes are deliberately about the CONTROLLED fielder — the one player
 * a controller steers, and the only one the movement slice changes. The auto-chasing
 * ranked fielders are engine behaviour and were never on the key stream. */
typedef struct {
    // internal edge-detection state (do not set)
    int initialized;
    int p_batOutcome;
    int p_controlIndex;
    Vector3D p_controlLocation;

    int chasing; // a batted ball is loose right now
    long chase_start_frame;

    // observed (readable after the run)
    long recoveries; // batted balls that came back into a fielder's hand
    long recovery_frames_sum;
    long recovery_frames_max;
    long abandoned; // windows that ended without a recovery (foul / reset)

    long chase_samples; // frames sampled while a batted ball was loose
    double chase_dist_sum; // sum of |controlled fielder - ball target point| over those frames

    long step_frames; // frames the controlled fielder moved without control changing
    double step_sum; // sum of its per-frame displacement -> mean = realized speed
} FieldingObserver;

void fielding_observer_init(FieldingObserver* o);
void fielding_observer_hook(const SimGame* g, void* ctx);

/* ---- Batting-selection observer ---------------------------------------- *
 * Watches WHO takes the bat and WHEN — the half of §12/§27 that no existing band
 * touches. The offense breakdown counts pitches, contacts and bases; none of it
 * moves if the batting side starts choosing a different player, takes longer to
 * answer, or stops being offered a batter at all.
 *
 *   at-bats        seatings that ANSWERED a prompt. Not every plate entry is one: a
 *                  foul reset restores the previous batter to the plate with no
 *                  decision involved, and counting those would put ~20% noise into
 *                  the selection numbers. Restorations are counted separately so the
 *                  difference is readable rather than mysterious.
 *   joker entries  seatings of a joker per half-inning. The batting controller's
 *                  joker appetite. It is a POLICY probe: when the controller stops
 *                  walking the offer and starts choosing outright, this is the band
 *                  that says whether the preference order survived the rewrite.
 *   offer→seat     frames from the prompt being raised to a batter being seated.
 *                  Measures the answer latency of whichever producer is batting —
 *                  and the window in which a select can be declared but not yet
 *                  actionable, because the previous batter still holds home.
 *   abandoned      prompts that closed with nobody seated AND no half-inning ending
 *                  to explain it. A prompt cancelled by the inning ending is ordinary
 *                  play (three outs land while the question is on the table); one
 *                  that simply evaporates mid-play is the batter-selection deadlock
 *                  family, so the two are counted apart rather than together.
 *   joker openings half-innings whose §12 opening designation landed on a JOKER.
 *                  §7 says a joker takes nobody's batting turn, so the designation
 *                  belongs to the regular in turn; when it lands on a joker,
 *                  §12(2)'s "next in order == designated" can never come true,
 *                  because the order index only ever names a regular slot. Kept as
 *                  a band rather than an invariant so the number is readable on
 *                  both sides of the fix.
 *
 * Every count is taken on a rising edge (seatings from batterEntered's own effect,
 * prompts from waitingForBatterDecision), so a frame held open never double-counts. */
typedef struct {
    // internal edge-detection state (do not set)
    int initialized;
    int p_waiting;
    int p_batter; // previous frame's active batter index (-1 = nobody at the plate)
    int p_designated;
    int p_inning;
    long prompt_frame; // frame the open prompt was raised on

    // observed (readable after the run)
    long half_innings;
    long at_bats; // seatings that answered an open prompt
    long joker_at_bats; // ...of which were jokers
    long restorations; // plate entries with no prompt open (a foul reset putting the batter back)
    long prompts; // batter prompts raised
    long cancelled; // prompts closed by the half-inning ending — ordinary play
    long abandoned; // prompts closed with nobody seated and no inning end to explain it
    long answer_frames_sum;
    long answer_frames_max;

    long designations; // half-innings that reached an opening §12 designation
    long joker_designations; // ...where the designation landed on a joker
} BattingSelectionObserver;

void batting_selection_observer_init(BattingSelectionObserver* o);
void batting_selection_observer_hook(const SimGame* g, void* ctx);

/* ---- Swing observer ---------------------------------------------------- *
 * Watches the SWING's timing, which no existing band can see. The offense
 * breakdown counts contacts and measures batted-ball power and direction; none
 * of it moves if the batter starts meeting the ball at the wrong point of its
 * arc, or if a producer starts cutting its declaration closer to the contact
 * frame than it used to.
 *
 *   elevation error  |V| at contact, where V is the launch elevation the swing
 *                    actually produced. V == 0 is a swing met dead centre; the
 *                    physics misses at |V| > VERTICAL_ANGLE_LIMIT. So the mean
 *                    says how central the timing is, and it is the ONE number
 *                    that reads the same before and after a rewrite that changes
 *                    how the value is expressed — because it is the physics,
 *                    not the representation.
 *   near misses      swings whose |V| passed half the limit: still hits, but the
 *                    margin is gone. A geometry that gets harder shows up here
 *                    before it shows up in the contact rate.
 *   miss causes      a whiff is either bad timing (|V| over the limit) or a ball
 *                    too far off the plate to reach (BALL_MAX_OFFSET). They are
 *                    counted apart because only the first is the swing's fault,
 *                    and a band that adds them cannot tell a timing regression
 *                    from a pitching change.
 *   lead frames      frames between the swing's LAST declared value and contact.
 *                    The margin the producer leaves the engine — and, later, the
 *                    margin a message has to cross a wire in. A mean that falls
 *                    is a producer cutting it finer; a MINIMUM at zero is one
 *                    that has already been late once.
 *   passes           at-bats where the batter carried no swing through at all.
 *
 * Everything is sampled on the batOutcome rising edge, using the PREVIOUS frame's
 * ball state: observers run after the whole frame, by which time a contact has
 * already replaced the ball's velocity with the batted one. */
typedef struct {
    // internal edge-detection state (do not set)
    int initialized;
    int p_batOutcome;
    float p_ball_vy; // last frame's ball vertical speed (the one the hit test used)
    float p_ball_x; // last frame's ball offset across the plate
    int p_swing_done; // last frame's "the swing's values are all in" reading
    long decided_frame; // frame the last value landed on; -1 = nothing declared this pitch
    int p_batterReady;
    int p_stopped; // last frame's batting_stopped — a pass is its rising edge

    // observed (readable after the run)
    long swings; // swings carried through to contact (hits + whiffs)
    long passes; // at-bats that declined to swing
    long miss_elevation; // whiffs caused by timing
    long miss_offplate; // whiffs caused by an unreachable ball
    double elev_abs_sum; // Σ |V| over swings that connected
    long elev_n;
    long near_misses; // ...of which |V| was past half the limit
    long lead_frames_sum;
    long lead_frames_n;
    long lead_frames_min; // the tightest margin any producer left
} SwingObserver;

void swing_observer_init(SwingObserver* o);
void swing_observer_hook(const SimGame* g, void* ctx);

/* ---- shared helper ----------------------------------------------------- */

/** Count batting-team players currently standing on 1st/2nd/3rd base. */
int sim_runners_on_base(const SimGame* g);

#endif /* SIM_OBSERVERS_H */
