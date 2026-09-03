#include "test_helpers.h"
#include "sim_harness.h"
#include "sim_observers.h"
#include <stdlib.h>

// Characterise AI offense across many seeds × a full first period, and then hold the
// characterisation to asserted bands.
//
// This test used to print its numbers and assert almost nothing (contacts != 0, stalls == 0),
// which meant an AI that swung at 40% of pitches, or whose direction fan collapsed to centre,
// or that stopped reaching base, went green. The numbers themselves lived in commit messages,
// so every session re-derived its own baseline and a slow drift across slices was invisible.
//
// The bands below fix both halves of that: the currently-measured value sits beside each band
// in the table, so the baseline lives in the repo, and the band is wide enough to survive a
// legitimate re-baseline while still catching degradation. They exist because a slice that changes
// when the AI observes the world re-baselines the determinism hash BY DESIGN — the hash cannot gate
// that change, so this net has to.
//
// A BAND THAT CANNOT SEE A REGRESSION IS A TEST THAT CANNOT FAIL. Several of these were first sized
// when nothing was expected to move them, and then something did: `batters reaching first` fell 17%
// (137 -> 109) without its floor of 50 twitching once, and the out-of-bounds rate fell by a factor
// of three inside a band that ran to 30%. So each band was re-cut around its measurement, roughly a
// quarter either side for the soft ones and hard against the arithmetic where there is any
// (`outs recorded` cannot honestly exceed 3 x 3 x 24; a fifth of a five-bucket fan cannot exceed
// 20%). A slice that legitimately moves one of these will now BREACH, which is the point: a breach
// is a decision to make, not a failure to route around. The table under the run says which.
//
// WHAT THIS TEST STILL CANNOT SEE. Runs are barely reachable here — 3 across 24 seeds — so the award
// paths, should_period_end, the pool refresh and period transitions remain essentially uncovered by
// these bands, by the box score and by the determinism hash. For referee scoring and flow work, the
// scenario tier is the primary net. Deliberately, no band mentions runs: at n=3 a band would be
// measuring noise, and a green band must not imply coverage it does not have.
//
// Set SIM_PBP=1 to also see the play-by-play.

#define SEED_COUNT 24
#define PERIOD_MAX_FRAMES 120000L
#define STALL_LIMIT 20000L
#define BAND_MAX 28

// Stay strictly inside the first period: it ends at inning == halfInningsInPeriod (4), and the
// period boundary hands off to a menu the single-phase sim harness does not yet autopilot
// (a documented deferred gap). Three half-innings of offense per seed, no boundary crossing.
static int three_half_innings_done(const SimGame* g)
{
    return (g->state->rules->scoreboard.inning - g->start_inning) >= 3 || g->state->screen != SCREEN_GAME;
}

// One asserted property of the AI's play. `measured` is the value this run produced; `lo`/`hi`
// are the band it must fall inside; `baseline` records what the number was when the band was
// written, so a drift is readable without digging up an old PR.
typedef struct {
    const char* name;
    double measured;
    double lo;
    double hi;
    const char* baseline;
    const char* guards; // the degradation this band exists to catch
} Band;

static void band_add(Band* bands, int* n, Band b)
{
    if (*n < BAND_MAX) bands[(*n)++] = b;
}

static double percent(long part, long whole)
{
    return whole ? 100.0 * (double)part / (double)whole : 0.0;
}

int test_ai_offense_breakdown(void)
{
    long T_pitches = 0, T_contacts = 0, T_whiffs = 0, T_strikes = 0, T_balls = 0, T_outs = 0;
    long T_reached_base = 0, T_reached_third = 0, T_ran_third = 0, T_scored_third = 0;
    long T_out_third = 0, T_wound_third = 0, T_runs = 0;
    long T_fouls = 0, T_power_sum = 0, T_power_n = 0, T_dir[5] = {0};
    int T_power_min = 9999, T_power_max = -9999;
    int seeds_reached_third = 0, seeds_ran_third = 0, seeds_incomplete = 0, seeds_stalled = 0;
    long T_recoveries = 0, T_recovery_frames = 0, T_recovery_max = 0, T_abandoned = 0;
    long T_chase_samples = 0, T_step_frames = 0;
    double T_chase_dist = 0.0, T_step_sum = 0.0;
    long T_at_bats = 0, T_joker_at_bats = 0, T_restorations = 0, T_prompts = 0;
    long T_cancelled = 0, T_sel_abandoned = 0, T_answer_frames = 0, T_answer_max = 0;
    long T_designations = 0, T_joker_designations = 0;
    long T_sw_swings = 0, T_sw_passes = 0, T_sw_miss_elev = 0, T_sw_miss_plate = 0;
    long T_sw_elev_n = 0, T_sw_near = 0, T_sw_lead_sum = 0, T_sw_lead_n = 0;
    long T_sw_lead_min = -1;
    double T_sw_elev_sum = 0.0;

    for (int s = 0; s < SEED_COUNT; s++) {
        unsigned int seed = 0xA11CE000u + (unsigned int)s * 0x9E3779B1u;

        GameSetup setup;
        sim_make_normal_setup(&setup, 0, 1, CONTROL_AI, CONTROL_AI);
        SimGame* g = sim_create(&setup, seed);
        ASSERT_NOT_NULL(g, "sim_create returned NULL");

        InvariantObserver inv;
        invariant_observer_init(&inv, STALL_LIMIT);
        sim_attach(g, invariant_observer_hook, &inv);

        BoxScoreObserver box;
        box_score_observer_init(&box, getenv("SIM_PBP") ? stdout : NULL);
        sim_attach(g, box_score_observer_hook, &box);

        FieldingObserver fld;
        fielding_observer_init(&fld);
        sim_attach(g, fielding_observer_hook, &fld);

        BattingSelectionObserver sel;
        batting_selection_observer_init(&sel);
        sim_attach(g, batting_selection_observer_hook, &sel);

        SwingObserver sw;
        swing_observer_init(&sw);
        sim_attach(g, swing_observer_hook, &sw);

        long frames = sim_run_until(g, three_half_innings_done, PERIOD_MAX_FRAMES);
        if (frames < 0) seeds_incomplete++; // budget hit or failure; counters still valid

        // A stall is a real finding (an inning-rollover hang), but it must not abort the
        // measurement — record it, keep the counters gathered up to the stall, continue.
        if (g->failed) {
            seeds_stalled++;
            printf("  seed 0x%08X stalled: %s\n", seed, g->fail_reason);
        }

        T_pitches += box.pitches;
        T_contacts += box.contacts;
        T_whiffs += box.whiffs;
        T_strikes += box.strikes_called;
        T_balls += box.balls_called;
        T_outs += box.outs_made;
        T_reached_base += box.reached_base;
        T_reached_third += box.reached_third;
        T_ran_third += box.ran_from_third;
        T_scored_third += box.scored_from_third;
        T_out_third += box.out_from_third;
        T_wound_third += box.wound_from_third;
        T_runs += box.runs_scored;
        T_fouls += box.fouls;
        T_power_sum += box.contact_power_sum;
        T_power_n += box.contact_power_n;
        if (box.contact_power_n > 0) {
            if (box.contact_power_min < T_power_min) T_power_min = box.contact_power_min;
            if (box.contact_power_max > T_power_max) T_power_max = box.contact_power_max;
        }
        for (int b = 0; b < 5; b++)
            T_dir[b] += box.dir_bins[b];

        T_recoveries += fld.recoveries;
        T_recovery_frames += fld.recovery_frames_sum;
        if (fld.recovery_frames_max > T_recovery_max) T_recovery_max = fld.recovery_frames_max;
        T_abandoned += fld.abandoned;
        T_chase_samples += fld.chase_samples;
        T_chase_dist += fld.chase_dist_sum;
        T_step_frames += fld.step_frames;
        T_step_sum += fld.step_sum;

        T_at_bats += sel.at_bats;
        T_joker_at_bats += sel.joker_at_bats;
        T_restorations += sel.restorations;
        T_prompts += sel.prompts;
        T_cancelled += sel.cancelled;
        T_sel_abandoned += sel.abandoned;
        T_answer_frames += sel.answer_frames_sum;
        if (sel.answer_frames_max > T_answer_max) T_answer_max = sel.answer_frames_max;
        T_designations += sel.designations;
        T_joker_designations += sel.joker_designations;

        T_sw_swings += sw.swings;
        T_sw_passes += sw.passes;
        T_sw_miss_elev += sw.miss_elevation;
        T_sw_miss_plate += sw.miss_offplate;
        T_sw_elev_sum += sw.elev_abs_sum;
        T_sw_elev_n += sw.elev_n;
        T_sw_near += sw.near_misses;
        T_sw_lead_sum += sw.lead_frames_sum;
        T_sw_lead_n += sw.lead_frames_n;
        if (sw.lead_frames_min >= 0 && (T_sw_lead_min < 0 || sw.lead_frames_min < T_sw_lead_min))
            T_sw_lead_min = sw.lead_frames_min;

        if (box.reached_third > 0) seeds_reached_third++;
        if (box.ran_from_third > 0) seeds_ran_third++;

        sim_destroy(g);
    }

    printf(
        "\n  AI offense over %d seeds × 3 half-innings (incomplete=%d, stalled=%d):\n", SEED_COUNT, seeds_incomplete,
        seeds_stalled
    );
    printf(
        "    pitches=%ld contacts=%ld (%.0f%%) whiffs=%ld | strikes=%ld balls=%ld outs=%ld runs=%ld\n", T_pitches,
        T_contacts, percent(T_contacts, T_pitches), T_whiffs, T_strikes, T_balls, T_outs, T_runs
    );
    printf(
        "    bases: reachedFirst=%ld reachedThird=%ld (in %d seeds)\n", T_reached_base, T_reached_third,
        seeds_reached_third
    );
    printf(
        "    third→home: ranForHome=%ld (in %d seeds) → scored=%ld, OUT=%ld, wounded=%ld\n", T_ran_third,
        seeds_ran_third, T_scored_third, T_out_third, T_wound_third
    );
    // Style-1 power-meter accuracy (intent vs realized). ≈ +1 means correct meter use.
    // Actualized out-of-bounds rate: a hit is either OUT OF BOUNDS or not (a caught ball counts as
    // in-bounds — "else"). Most hits should land fair, but some fouls are wanted for variety.
    printf(
        "    out of bounds: %ld of %ld contacts = %.0f%% (rest land fair or are caught)\n", T_fouls, T_contacts,
        percent(T_fouls, T_contacts)
    );
    // Actualized batted-ball power (0..36): the AI should generally hit with real strength.
    printf(
        "    power (all contacts): n=%ld mean=%.1f min=%d max=%d (of 36 max)\n", T_power_n,
        T_power_n ? (double)T_power_sum / T_power_n : 0.0, T_power_n ? T_power_min : 0, T_power_n ? T_power_max : 0
    );
    // Normal-swing (style-1) direction spread (realized launch angle, right→left, 5 equal buckets):
    // should be a broad, roughly uniform fan — NOT collapsed to center, NOT piled at the extremes.
    printf("    style-1 direction R→L: [%ld %ld %ld %ld %ld]\n", T_dir[0], T_dir[1], T_dir[2], T_dir[3], T_dir[4]);
    printf(
        "    fielding: recoveries=%ld abandoned=%ld meanFrames=%.2f maxFrames=%ld meanChase=%.3f meanStep=%.5f\n",
        T_recoveries, T_abandoned, T_recoveries ? (double)T_recovery_frames / (double)T_recoveries : 0.0,
        T_recovery_max, T_chase_samples ? T_chase_dist / (double)T_chase_samples : 0.0,
        T_step_frames ? T_step_sum / (double)T_step_frames : 0.0
    );

    printf(
        "    swing: swings=%ld passes=%ld | miss timing=%ld offplate=%ld | |V| mean=%.2f near=%.0f%% | "
        "lead mean=%.1f min=%ld\n",
        T_sw_swings, T_sw_passes, T_sw_miss_elev, T_sw_miss_plate,
        T_sw_elev_n ? T_sw_elev_sum / (double)T_sw_elev_n : 0.0, percent(T_sw_near, T_sw_elev_n),
        T_sw_lead_n ? (double)T_sw_lead_sum / (double)T_sw_lead_n : 0.0, T_sw_lead_min
    );
    printf(
        "    selection: seated=%ld (jokers=%ld, %.0f%%) restored=%ld | prompts=%ld cancelled=%ld abandoned=%ld\n",
        T_at_bats, T_joker_at_bats, percent(T_joker_at_bats, T_at_bats), T_restorations, T_prompts, T_cancelled,
        T_sel_abandoned
    );
    printf(
        "    prompt→seat: mean=%.1f frames max=%ld\n", T_at_bats ? (double)T_answer_frames / (double)T_at_bats : 0.0,
        T_answer_max
    );
    printf(
        "    §12 opening designations: %ld, of which JOKER: %ld (%.1f%%)\n", T_designations, T_joker_designations,
        percent(T_joker_designations, T_designations)
    );

    // A stall IS a hard defect (an AI-vs-AI deadlock), distinct from the quality bands below:
    // the sim tier is the regression net, so a silent stall must not pass green.
    if (seeds_stalled != 0) {
        printf("  %d seed(s) stalled (AI-vs-AI deadlock)\n", seeds_stalled);
        return TEST_FAILED;
    }

    // ---- the bands -------------------------------------------------------------------
    // Every baseline below is the value this suite produced at sim hash a0d339d08a516f85, 24 seeds ×
    // 3 half-innings, when the bands were last re-cut. A baseline is not a target: it is what the
    // number WAS, so a drift is readable here rather than by digging up an old PR.
    //
    // Two floors are looser than the quarter-either-side rule: `batters seated` and `joker at-bats`.
    // Both moved several points when the batter's power window was repaired — not because the
    // selection policy changed, but because a different set of games gets played once the batting
    // controller stops re-deciding its plan every frame of every windup. They are cut to survive
    // that class of reshuffle and still catch a policy actually changing.

    long dir_total = 0, dir_min = -1, dir_max = -1;
    for (int b = 0; b < 5; b++) {
        dir_total += T_dir[b];
        if (dir_min < 0 || T_dir[b] < dir_min) dir_min = T_dir[b];
        if (T_dir[b] > dir_max) dir_max = T_dir[b];
    }

    Band bands[BAND_MAX];
    int band_count = 0;

    band_add(
        bands, &band_count,
        (Band){"pitches thrown", (double)T_pitches, 450, 800, "573", "the pitcher stops pitching, or at-bats never end"}
    );
    band_add(
        bands, &band_count,
        (Band){"contact rate %", percent(T_contacts, T_pitches), 85, 97, "90.4",
               "the batter stops meeting the ball (a swing that no longer connects)"}
    );
    band_add(
        bands, &band_count,
        (Band){"balls called", (double)T_balls, 30, 80, "55",
               "pitch aim collapsing onto the plate — every pitch a called strike"}
    );
    band_add(
        bands, &band_count,
        (Band){"outs recorded", (double)T_outs, 190, 220, "216",
               "half-innings ending some other way than on three outs — and, at the ceiling, an out counted "
               "twice (bug #6): 24 seeds x 3 half-innings x 3 burns is EXACTLY 216, so 220 is unreachable "
               "by honest play"}
    );
    band_add(
        bands, &band_count,
        (Band){"batters reaching first", (double)T_reached_base, 90, 132, "109",
               "contact that never turns into a runner. KNOW ITS RESOLUTION: this is an aggregate of "
               "everything between the bat and the base, and a legitimate slice moved it 20% (137 -> 109) "
               "in one go, so the band cannot be cut fine enough to see a drift of that size and stay "
               "usable. It is a collapse detector. The instruments that DO see a weaker swing are the "
               "direct ones — batted-ball power, out-of-bounds rate, mean chase distance — all three of "
               "which breach on a 17% power cut that leaves this band green"}
    );
    band_add(
        bands, &band_count,
        (Band){"batted-ball power mean (of 36)", T_power_n ? (double)T_power_sum / T_power_n : 0.0, 18.5, 24, "21.0",
               "the swing going soft — the meter starved, as bug #4 did it"}
    );
    band_add(
        bands, &band_count,
        (Band){"out-of-bounds rate %", percent(T_fouls, T_contacts), 2, 7, "5.0",
               "direction drifting foul, or every ball dumped safely into the field"}
    );
    band_add(
        bands, &band_count,
        (Band){"direction fan: smallest bucket %", percent(dir_min, dir_total), 7, 20, "10.3",
               "the fan collapsing — the centre-bias the swing slice removes structurally"}
    );
    band_add(
        bands, &band_count,
        (Band){"direction fan: largest bucket %", percent(dir_max, dir_total), 20, 38, "27.7",
               "the fan piling into one direction"}
    );

    // ---- the fielding bands ----------------------------------------------------------
    // The ten bands above are every one of them batting- or pitching-side, so until these
    // arrived the whole catching half of the game could regress without a single number
    // moving. They were baselined 2026-08-27 on UNCHANGED code, before the movement slice
    // touched anything — recording them first is what lets the slice argue from them
    // afterwards, since the slice re-baselines the determinism hash by design and cannot
    // lean on it.
    band_add(
        bands, &band_count,
        (Band){"mean frames to recover a batted ball",
               T_recoveries ? (double)T_recovery_frames / (double)T_recoveries : 0.0, 85, 118, "97.2",
               "the defence getting slower end to end — a fielder that sets off late, or not at all"}
    );
    band_add(
        bands, &band_count,
        (Band){"worst frames to recover", (double)T_recovery_max, 0, 1100, "675",
               "one chase that never converges, hidden inside a healthy mean"}
    );
    band_add(
        bands, &band_count,
        (Band){"chases that ended in possession %", percent(T_recoveries, T_recoveries + T_abandoned), 92, 100, "95.6",
               "balls the catching side simply never brings back"}
    );
    band_add(
        bands, &band_count,
        (Band){"mean chase distance", T_chase_samples ? T_chase_dist / (double)T_chase_samples : 0.0, 7.5, 11.5, "9.62",
               "the controlled fielder not going where the engine sent it"}
    );
    // The floor here is the load-bearing one: RUN_SPEED is 0.12 and WALK_SPEED is 0.06, so a
    // controlled fielder quietly moved onto the auto-fielders' walk — exactly what reusing
    // move_to_target would have done — cannot stay inside this band.
    band_add(
        bands, &band_count,
        (Band){"mean step per moving frame", T_step_frames ? T_step_sum / (double)T_step_frames : 0.0, 0.100, 0.115,
               "0.1061", "the controlled fielder silently switched to a different speed"}
    );

    // What the swing slice did to the numbers above, kept because it is the argument the bands are
    // now cut against — and because every one of these was a SIDE EFFECT of the physics being
    // written honestly rather than of anything aimed at fielding:
    //
    //   declaration lead   35.0 -> 85.8 frames. The AI used to declare when a meter reached a
    //     threshold, which is late by construction; it now declares as soon as the ball is up and
    //     the values are consumed at contact. The minimum across 24 seeds went from 2 frames to 72 —
    //     the margin a message would one day have to cross a wire in.
    //   out of bounds      12% -> 3.5%. The old elevation was read off a meter whose scale moved
    //     with power, so loft and power were accidentally paired and their extreme combination flew
    //     off the field. The declared values are independent, so that pairing now happens only by
    //     chance.
    //   mean chase         15.12 -> 9.06, and chases ending in possession 87.6% -> 96.2%. NOT a
    //     fielding change — the catching side's code is untouched and its per-chase behaviour is
    //     unchanged. The far chases were the ones that went out of bounds, and they stopped happening.
    //
    // ---- the swing bands -------------------------------------------------------------
    // The SWING's timing, which nothing above can see. Baselined on UNCHANGED code on
    // 2026-09-02, before the swing slice moved anything — and the measurement immediately
    // showed why they were needed: of 550 AI swings, ZERO whiffs were caused by timing (all
    // four were balls too far off the plate to reach). So `contact rate %` is not a timing
    // band, and a slice that rewrites the swing's timing would have had no net at all.
    band_add(
        bands, &band_count,
        (Band){"swing elevation |V| mean (limit 8)", T_sw_elev_n ? T_sw_elev_sum / (double)T_sw_elev_n : 0.0, 1.5, 2.5,
               "1.90", "swings drifting off the centre of the ball — the miss the contact rate cannot see"}
    );
    // The early-warning band: margin goes before contact does. A geometry that got harder moves this
    // well before it moves anything downstream. Its FLOOR was dropped to zero when
    // VERTICAL_ANGLE_LIMIT went 5 -> 8: "half the limit" became a bar the AI's deliberate scatter
    // does not reach, so a floor would only assert that the batter is imperfect, which is the
    // sensitivity knob's business and not this band's. The ceiling is the load-bearing side.
    band_add(
        bands, &band_count,
        (Band){"swings past half the elevation limit %", percent(T_sw_near, T_sw_elev_n), 0, 8, "1.54",
               "the timing margin collapsing while every swing still technically connects"}
    );
    // The floor is zero and the measurement is zero, and that is honest rather than vacuous: since
    // the swing slice the AI DECLARES a vertical outright, so it has no timing to get wrong — it can
    // only miss by its own deliberate scatter, which today never crosses the limit. So this band
    // has no teeth against the AI at all; its teeth are for a geometry that gets much harder, for a
    // scatter that widens, and for the day a producer in this tier acquires a timed gesture. Stated
    // outright because a green band whose reason for being green is "the thing it measures cannot
    // happen here" is the most misleading kind.
    band_add(
        bands, &band_count,
        (Band){"whiffs caused by timing", (double)T_sw_miss_elev, 0, 25, "0",
               "a swing that stops connecting for timing reasons, or an AI made unrealistically perfect"}
    );
    // How much margin the producer leaves the engine before the value is consumed — and, on a
    // wire one day, how much a late message could eat. The minimum seen today is 2 frames.
    band_add(
        bands, &band_count,
        (Band){"declaration lead frames, mean", T_sw_lead_n ? (double)T_sw_lead_sum / (double)T_sw_lead_n : 0.0, 70,
               110, "86.1", "a producer drifting back toward declaring at the last possible moment"}
    );
    band_add(
        bands, &band_count,
        (Band){"declined swings %", percent(T_sw_passes, T_sw_swings + T_sw_passes), 4, 16, "9.6",
               "the batter swinging at everything, or refusing to swing at all"}
    );

    // ---- the batting-selection bands -------------------------------------------------
    // WHO takes the bat, and how quickly. Every band above is about what a batter DOES once
    // seated; none of them moves if the batting side starts seating a different player, or
    // stops being offered one. Baselined on UNCHANGED code before the batter-selection
    // slice, for the same reason the fielding bands were: the slice re-baselines the
    // determinism hash by design, so the hash cannot be the net for it.
    band_add(
        bands, &band_count,
        (Band){"batters seated", (double)T_at_bats, 245, 315, "268",
               "the selection path stalling, double-seating, or ending half-innings early"}
    );
    // The policy probe. The batting controller walks the offer today and chooses outright
    // afterwards; if the preference order survives that rewrite, this number does not move.
    band_add(
        bands, &band_count,
        (Band){"joker at-bats %", percent(T_joker_at_bats, T_at_bats), 26, 44, "31.3",
               "the batting controller's joker preference silently changing"}
    );
    // Not "prompts answered %": a prompt overtaken by the third burn is ordinary play and made
    // that number read 84% on healthy code. What is never ordinary is a prompt that closes with
    // nobody seated and no inning ending to explain it, so the band is an absolute floor of zero
    // on exactly that — the batter-selection deadlock family (bug #5), stated as a number.
    band_add(
        bands, &band_count,
        (Band){"prompts abandoned mid-play", (double)T_sel_abandoned, 0, 0, "0",
               "a prompt the batting side never answers while play continues — the deadlock family"}
    );
    band_add(
        bands, &band_count,
        (Band){"mean frames from prompt to seating", T_at_bats ? (double)T_answer_frames / (double)T_at_bats : 0.0, 135,
               190, "156.7", "the batting side taking longer to answer, or retrying blindly"}
    );
    // §7: "Jokeripelaaja ei vie kenenkään lyöntivuoroa", so §12(2)'s "vuoron aloittanut pelaaja"
    // is the regular whose turn it is, never the joker that happened to swing first. When the
    // designation lands on a joker, §12(2) can never fire again that half-inning: the comparison
    // it makes is against the batting-order index, which only ever names a regular slot. This
    // read 33.3% before the referee stopped designating jokers, and an absolute zero is the only
    // honest band for it — there is no rate of this that is acceptable.
    band_add(
        bands, &band_count,
        (Band){"§12 openings designated to a joker %", percent(T_joker_designations, T_designations), 0, 0, "0.0",
               "§12(2) becoming unfirable for a whole half-inning"}
    );

    int breached = 0;
    printf("\n  AI-quality bands:\n");
    for (int i = 0; i < band_count; i++) {
        const Band* b = &bands[i];
        int ok = b->measured >= b->lo && b->measured <= b->hi;
        if (!ok) breached++;
        printf(
            "    %-36s %8.2f  in [%g, %g]  baseline %-6s %s\n", b->name, b->measured, b->lo, b->hi, b->baseline,
            ok ? "ok" : "OUT OF BAND"
        );
        if (!ok) printf("        this band guards against: %s\n", b->guards);
    }

    if (breached != 0) {
        printf(
            "\n  %d AI-quality band(s) breached. These are not tuning targets: a breach means the AI\n"
            "  plays measurably worse than it did, or the band was wrong. Decide which — and if the\n"
            "  change is intended, move the band and its baseline in the same commit.\n",
            breached
        );
        return TEST_FAILED;
    }
    return TEST_PASSED;
}
