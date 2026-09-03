#include "test_helpers.h"
#include "all_unit.h"
#include "actions_pure/swing_geometry.h"
#include "actions_pure/batting_physics.h"
#include "actions_pure/pitching_physics.h"
#include <math.h>

/*
 * The swing minigame's geometry, in two halves.
 *
 * 1. EQUIVALENCE. The collapsed law must be the same physics as the meter-count expression it
 *    replaces — not approximately, at every point of the grid. That is what lets the slice claim the
 *    swing feels the same while the values crossing the boundary change completely.
 *
 * 2. THE BANDS. The knobs in swing_geometry.h are not free numbers: their joint effect on how hard
 *    the game is has a closed form, so the acceptance bands can be asserted rather than playtested.
 *    This is the test that says which COMBINATIONS are playable. Turn a knob past what the game can
 *    take and this goes red — which is the point, because the alternative is finding out at a
 *    screen, once, months later.
 */

// The law this replaced, kept HERE and not in the tree. It is the reference an equivalence is
// measured against, which makes it a test fixture — production carries only the law it actually uses,
// and the historical one lives with the claim it exists to support. The meter lengths are literals
// for the same reason: the claim is about these numbers, whatever the meters later become.
#define LEGACY_SWING_MAX 52
#define LEGACY_LOAD_MAX 36

// What the batter's meter displayed while the elevation was being chosen: a marker starting at a
// power-dependent top and falling to zero.
static float legacy_angle_meter_value(int counter, int max, int power_count)
{
    float upper = (float)(power_count + (LEGACY_SWING_MAX - LEGACY_LOAD_MAX)) / (float)LEGACY_SWING_MAX;
    return upper - (1.0f * counter / max) * upper;
}

// And the elevation it produced from the two meter counts.
static float legacy_vertical_angle(int power_count, int angle_count, float ball_vy)
{
    float scale = (float)(power_count + (LEGACY_SWING_MAX - LEGACY_LOAD_MAX));
    if (scale < 0.00001f && scale > -0.00001f) return 0.0f;
    float zero = LEGACY_SWING_MAX * (1.0f * power_count / scale);
    return 7.0f * ball_vy * (angle_count - zero) * (scale / LEGACY_SWING_MAX);
}

// 1. The collapsed law IS the legacy law, at every reachable (power, vertical) pair.
int test_swing_vertical_angle_matches_the_legacy_meter_law(void)
{
    const float speeds[] = {0.065f, 0.125f, 0.185f};

    for (int s = 0; s < 3; s++) {
        // The ball is falling at contact: the legacy call site passes the CURRENT vy, which a
        // symmetric flight makes the negative of the launch speed.
        const float ball_vy = -speeds[s];

        for (int power_count = 0; power_count <= LEGACY_LOAD_MAX; power_count += 4) {
            for (int angle_count = 0; angle_count <= LEGACY_SWING_MAX; angle_count += 4) {
                float legacy = legacy_vertical_angle(power_count, angle_count, ball_vy);
                // What the human was reading off the screen all along — the value that is now
                // declared outright instead of being inferred from when a key was pressed.
                float declared = legacy_angle_meter_value(angle_count, LEGACY_SWING_MAX, power_count);
                float collapsed = swing_vertical_angle(declared, ball_vy);

                ASSERT(
                    fabsf(legacy - collapsed) < 0.002f,
                    "the collapsed elevation law must reproduce the legacy meter-count law exactly"
                );
            }
        }
    }
    return TEST_PASSED;
}

// (The test that pinned the marker's TOP to the legacy meter's went with the concept on 2026-09-03.
// The marker crosses the whole bar now, so the declared vertical is the bar's own level and there is
// no mapping left in this module to assert. What the human declares is what the human sees, and the
// scripted tier is where that claim belongs — it can press a key.)

// The hit window for one pitch, in frames: how long the marker spends close enough to the sweet spot
// that the swing still connects, clipped to the sweep's own ends.
//
// It takes no swing power, and that is the whole of the 2026-09-03 change: the marker crosses the
// whole bar whatever was declared, so the batter's own decision no longer changes how hard his swing
// is to time. One lever each — the pitcher's toss sets the difficulty, the batter's power sets the
// reward — instead of the pitcher's lever plus a second one the batter pointed at himself.
static float hit_window_frames(float pitch_power)
{
    float launch_vy = pitch_velocity_from_aim(pitch_power, 0.0f).y;
    int flight = calculate_pitch_frame_time(launch_vy, GRAVITY, 0.0f, SWING_CONTACT_TWEAK_FRAMES);
    int sweep = swing_vertical_sweep_frames(flight);

    float tolerance = (float)VERTICAL_ANGLE_LIMIT / (SWING_ELEVATION_GAIN * launch_vy); // in declared units
    float per_frame = 1.0f / (float)sweep; // the whole bar, every time
    float half = tolerance / per_frame;
    float crossing = (float)sweep * (1.0f - SWING_VERTICAL_FOCAL); // the same fraction of every sweep

    float low = crossing - half;
    float high = crossing + half;
    if (low < 0.0f) low = 0.0f;
    if (high > (float)sweep) high = (float)sweep;
    return (high > low) ? (high - low) : 0.0f;
}

// 3. The acceptance bands — the reason the knobs above can be trusted without a playtest.
int test_swing_geometry_stays_inside_its_acceptance_bands(void)
{
    const float pitch_powers[] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};

    float hardest = 1e9f, easiest = 0.0f;

    for (int i = 0; i < 5; i++) {
        float launch_vy = pitch_velocity_from_aim(pitch_powers[i], 0.0f).y;
        int flight = calculate_pitch_frame_time(launch_vy, GRAVITY, 0.0f, SWING_CONTACT_TWEAK_FRAMES);
        int sweep = swing_vertical_sweep_frames(flight);

        // The lead is the whole point of the clamp: the value must be in the world before the frame
        // that consumes it, on every pitch and not just the comfortable ones.
        ASSERT(
            sweep <= flight - SWING_LEAD_FRAMES || flight <= SWING_LEAD_FRAMES + 1,
            "the vertical sweep must finish a lead's worth of frames before contact"
        );

        // The power sweep runs ONCE and must finish inside the windup, on every pitch including the
        // shortest — otherwise the batter is still choosing a power when the ball is already gone,
        // and the beat the four-beat dance puts before the release does not exist.
        //
        // This replaced a band asking for at least one and a half CYCLES, written when the meter
        // looped. Playing it showed why looping was wrong: a meter that never runs out gives the
        // batter unlimited time, which is not a decision, and shows nothing at stake, so the natural
        // moment to press ends up after the release instead of during the windup.
        ASSERT(
            2 * SWING_POWER_SWEEP_FRAMES <= pitch_windup_total_frames(pitch_powers[i]),
            "one full power sweep must fit inside the windup, at every toss height"
        );

        float window = hit_window_frames(pitch_powers[i]);
        ASSERT(window > 0.0f, "every pitch must have a reachable hit window");
        if (window < hardest) hardest = window;
        if (window > easiest) easiest = window;
    }

    // The hard end was re-set on 2026-09-02 from the first play session, which is the only evidence
    // that can set it: the original 5 frames (100 ms) was a guess, and at 154 ms the hardest swing
    // was reported unhittable. 12 frames is 240 ms; today's hardest is 16.6 (332 ms).
    ASSERT(hardest >= 12.0f, "the hardest swing must still be humanly timeable");

    // The easy end was tightened 55 -> 40 on 2026-09-03, and WHY is the interesting part. 55 frames
    // was not a generous window, it was the whole sweep: with the marker's top scaled by the declared
    // power, a bunt's marker STARTED on the sweet spot and never left tolerance, so any press during
    // the entire descent connected. The "second difficulty axis" was an off switch at its easy end.
    // With the marker crossing the whole bar there is no such case left, and the band now says so.
    ASSERT(easiest <= 40.0f, "the easiest swing must still be a decision rather than a formality");

    // The gradient has to EXIST (or the pitcher's toss means nothing to the batter) and stay bounded
    // (or the easy end is free and the hard end is hopeless). It runs 1.3-3.0 rather than the old
    // 1.5-4.0 because it now comes from ONE axis instead of two: the toss alone spreads the window
    // 16.6 -> 25.9 frames, where the deleted power axis used to multiply the easy end by another 3.
    float ratio = easiest / hardest;
    ASSERT(ratio >= 1.3f, "a higher toss must be measurably harder to time than a low one");
    ASSERT(ratio <= 3.0f, "the difficulty spread must not run away between the easiest and hardest swing");
    return TEST_PASSED;
}

// 4. The one difficulty axis is real and points the right way — stated separately from the bands,
//    because a band that merely holds does not say WHICH direction the game gets harder in.
//
//    There used to be a second assertion here, that a harder swing demands more exact timing than a
//    bunt. It is gone because the axis is gone, and deliberately so: it came from a marker whose top
//    rose with the declared power, which made the sweet spot arrive at a different moment for every
//    power and put the difficulty lever in the hands of the player choosing the reward. The axis
//    that remains is the physical one, and it belongs to the other player.
int test_swing_difficulty_rises_with_the_toss(void)
{
    ASSERT(
        hit_window_frames(1.0f) < hit_window_frames(0.5f),
        "a higher toss must demand more exact timing (the ball-speed axis)"
    );
    // And the curve is not monotonic, which is SWING_LEAD_FRAMES doing its job rather than an
    // accident. Below about a quarter power the sweep is clamped to the flight, so it shrinks with
    // the flight and cancels the ball speed straight back out; above it the sweep hits its cap and
    // the ball speed starts to bite. The result is a peak in the middle and a lowest toss that is
    // NOT the free hit it would otherwise be — the header claims exactly this, and until now nothing
    // held it to the claim.
    ASSERT(
        hit_window_frames(0.0f) < hit_window_frames(0.25f),
        "the lowest toss must not be the easiest pitch in the game — that is what the lead clamp buys"
    );
    return TEST_PASSED;
}
