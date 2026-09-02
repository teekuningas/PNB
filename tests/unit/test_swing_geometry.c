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

// The legacy meter lengths the equivalence is pinned to. Deliberately literals and not an include:
// the claim is about these historical numbers, and it stays true whatever the meters become.
#define LEGACY_SWING_MAX 52
#define LEGACY_LOAD_MAX 36

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
                float legacy = calculate_batting_vertical_angle(
                    power_count, angle_count, ball_vy, LEGACY_SWING_MAX, LEGACY_LOAD_MAX
                );
                // What the human was reading off the screen all along — the value that is now
                // declared outright instead of being inferred from when a key was pressed.
                float declared = calculate_angle_meter_value(
                    angle_count, LEGACY_SWING_MAX, power_count, LEGACY_SWING_MAX, LEGACY_LOAD_MAX
                );
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

// 2. The marker's top is the legacy meter's top, and it degenerates to the sweet spot at zero power
//    — which is why a bunt cannot be mistimed into loft.
int test_swing_marker_top_matches_the_legacy_meter_top(void)
{
    for (int power_count = 0; power_count <= LEGACY_LOAD_MAX; power_count++) {
        float power = (float)power_count / (float)LEGACY_LOAD_MAX;
        float legacy = (float)(power_count + (LEGACY_SWING_MAX - LEGACY_LOAD_MAX)) / (float)LEGACY_SWING_MAX;
        ASSERT(fabsf(legacy - swing_marker_top(power)) < 0.0005f, "marker top must match the legacy meter top");
    }
    ASSERT(
        fabsf(swing_marker_top(0.0f) - SWING_VERTICAL_FOCAL) < 0.0005f,
        "at zero power the marker STARTS on the sweet spot — a bunt is level by construction"
    );
    ASSERT(fabsf(swing_marker_top(1.0f) - 1.0f) < 0.0005f, "at full power the marker spans the whole meter");
    ASSERT_EQ(
        (int)(SWING_VERTICAL_FOCAL * 1000.0f), (int)(swing_marker_top(-4.0f) * 1000.0f),
        "a power below the range clamps rather than inverting the meter"
    );
    return TEST_PASSED;
}

// The hit window for one (pitch power, swing power) pair, in frames: how long the marker spends
// close enough to the sweet spot that the swing still connects, clipped to the sweep's own ends.
static float hit_window_frames(float pitch_power, float swing_power)
{
    float launch_vy = pitch_velocity_from_aim(pitch_power, 0.0f).y;
    int flight = calculate_pitch_frame_time(launch_vy, GRAVITY, 0.0f, SWING_CONTACT_TWEAK_FRAMES);
    int sweep = swing_vertical_sweep_frames(flight);
    float top = swing_marker_top(swing_power);

    float tolerance = (float)VERTICAL_ANGLE_LIMIT / (SWING_ELEVATION_GAIN * launch_vy); // in declared units
    float per_frame = top / (float)sweep;
    float half = tolerance / per_frame;
    float crossing = (float)sweep * (1.0f - SWING_VERTICAL_FOCAL / top);

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
    const float swing_powers[] = {0.0f, 0.5f, 1.0f};

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

        // A batter who wants a particular power needs the ping-pong to offer it more than once
        // inside the windup, or picking a level is luck rather than a decision.
        float cycles = (float)pitch_windup_total_frames(pitch_powers[i]) / (float)(2 * SWING_POWER_SWEEP_FRAMES);
        ASSERT(cycles >= 1.5f, "the power ping-pong must offer at least one and a half cycles inside the windup");

        for (int j = 0; j < 3; j++) {
            // Reachability: the sweet spot must lie on the sweep, or that power could never connect.
            float top = swing_marker_top(swing_powers[j]);
            ASSERT(top >= SWING_VERTICAL_FOCAL - 0.0005f, "the sweet spot must lie on the marker's travel");

            float window = hit_window_frames(pitch_powers[i], swing_powers[j]);
            ASSERT(window > 0.0f, "every (pitch, power) pair must have a reachable hit window");
            if (window < hardest) hardest = window;
            if (window > easiest) easiest = window;
        }
    }

    // 5 frames is 100 ms at the 50Hz update: below that it stops being a skill test and becomes a
    // coin flip. 30 frames is 600 ms: above that it stops being a decision.
    ASSERT(hardest >= 5.0f, "the hardest swing must still be humanly timeable");
    ASSERT(easiest <= 30.0f, "the easiest swing must still be a decision rather than a formality");

    // The gradient has to EXIST (or the pitcher's power choice means nothing to the batter) and stay
    // bounded (or the easy end is free and the hard end is hopeless).
    float ratio = easiest / hardest;
    ASSERT(ratio >= 1.5f, "a higher toss and a harder swing must be measurably harder to time");
    ASSERT(ratio <= 4.0f, "the difficulty spread must not run away between the easiest and hardest swing");
    return TEST_PASSED;
}

// 4. The two difficulty axes are real and point the right way — stated separately from the bands,
//    because a band that merely holds does not say WHICH direction the game gets harder in.
int test_swing_difficulty_rises_with_the_toss_and_with_the_power(void)
{
    ASSERT(
        hit_window_frames(1.0f, 1.0f) < hit_window_frames(0.5f, 1.0f),
        "a higher toss must demand more exact timing (the ball-speed axis)"
    );
    ASSERT(
        hit_window_frames(0.5f, 1.0f) < hit_window_frames(0.5f, 0.0f),
        "a harder swing must demand more exact timing than a bunt (the power axis)"
    );
    return TEST_PASSED;
}
