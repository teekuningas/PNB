#include "batting_ai_strategy.h"
#include "rules_state.h"
#include "actions_pure/swing_geometry.h"
#include <math.h>
#include <stdlib.h>

// Batting styles and base-occupancy grades (matching BattingStrategy.style and batting_ai.c:field_status).
enum { AI_STYLE_BUNT = 0, AI_STYLE_NORMAL = 1, AI_STYLE_WOUND = 2 };
enum { AI_FIELD_EMPTY = 0, AI_FIELD_SCORING = 1, AI_FIELD_FIRST_OCCUPIED = 2 };

#define AI_POWER_THRESHOLD 2
#define AI_SPEED_THRESHOLD 2

// Largest bat angle (each side) the AI aims for. The batting system doubles arc_angle into the
// launch heading (theta = -arc_angle*2), and the foul lines sit at about ±0.75 rad. Normal swings
// span ±0.38 rad (±0.76 rad launch: fills the fair field, rare foul), bunts stay tighter at ±0.32
// rad, and wounding koppi stay inside ±0.22 rad (deep fair outfield). All stay inside the engine's
// own arc clamp (PI/7 ≈ 0.449).
#define AI_MAX_BATTING_ANGLE 0.38f
#define AI_BUNT_BATTING_ANGLE 0.32f
#define AI_WOUND_BATTING_ANGLE 0.22f

#define AI_BUNT_MIN_ANGLE 0.14f
#define AI_BUNT_LEFT_WEIGHT 0.60f

#define AI_NORMAL_SEAM_INNER 0.15f
#define AI_NORMAL_SEAM_WEIGHT 0.40f
#define AI_NORMAL_CENTER_WEIGHT 0.20f

#define AI_ANGLE_BUCKETS 1000
#define AI_POWER_DRAW_MAX 18.0f
#define AI_VERTICAL_DRAW_MID 100.0f

typedef struct {
    float power_min;
    float power_span;
    float vertical_bias;
    float vertical_spread;
} SwingProfile;

// Style-specific power and elevation profiles, in declared [0,1] units.
// At contact the pitch is falling (ball_vy < 0), so in
// swing_vertical_angle = GAIN * ball_vy * (SWING_VERTICAL_FOCAL - vertical):
//   vertical < FOCAL -> V < 0 -> low line drives / grounders (bounce quickly, dodge koppi)
//   vertical > FOCAL -> V > 0 -> lofted fly balls (koppi into the outfield)
static const SwingProfile AI_SWING_PROFILES[3] = {
    [AI_STYLE_BUNT] = {.power_min = 0.30f, .power_span = 0.14f, .vertical_bias = -0.042f, .vertical_spread = 0.015f},
    [AI_STYLE_NORMAL] = {.power_min = 0.64f, .power_span = 0.32f, .vertical_bias = -0.015f, .vertical_spread = 0.055f},
    [AI_STYLE_WOUND] = {.power_min = 0.78f, .power_span = 0.00f, .vertical_bias = 0.048f, .vertical_spread = 0.018f},
};

static const SwingProfile* swing_profile_for_style(int battingStyle)
{
    if (battingStyle == AI_STYLE_BUNT) return &AI_SWING_PROFILES[AI_STYLE_BUNT];
    if (battingStyle == AI_STYLE_WOUND) return &AI_SWING_PROFILES[AI_STYLE_WOUND];
    return &AI_SWING_PROFILES[AI_STYLE_NORMAL];
}

static float lerp_f(float a, float b, float u)
{
    return a + u * (b - a);
}

BattingStrategy
calculate_batting_strategy(const HalfInningState* halfInningState, int fieldStatus, int power, int speed, int period)
{
    BattingStrategy strategy = {AI_STYLE_NORMAL, 0, 0};
    int hasPower = (power > AI_POWER_THRESHOLD) ? 1 : 0;
    int isFast = (speed > AI_SPEED_THRESHOLD) ? 1 : 0;

    if (period < 4) {
        if (halfInningState->strikes == 0) {
            // First strike: probe with a normal swing. Running only on strike 1+ keeps at-bats
            // natural (giving the pitcher room to mix in väärä pitches) while still scoring on
            // clean läpilyönti hits via advance_runners_on_loose_hit.
            strategy.style = AI_STYLE_NORMAL;
            strategy.run_base_runners = 0;
            strategy.run_batter = 0;
        } else if (halfInningState->strikes == 1) {
            if (fieldStatus == AI_FIELD_EMPTY) {
                strategy.style = isFast ? AI_STYLE_BUNT : AI_STYLE_NORMAL;
                strategy.run_base_runners = 0;
                strategy.run_batter = 1;
            } else {
                strategy.style = hasPower ? AI_STYLE_NORMAL : AI_STYLE_BUNT;
                strategy.run_base_runners = 1;
                strategy.run_batter = isFast;
            }
        } else if (halfInningState->strikes == 2) {
            if (fieldStatus == AI_FIELD_EMPTY) {
                strategy.style = isFast ? AI_STYLE_BUNT : AI_STYLE_WOUND;
                strategy.run_base_runners = 0;
                strategy.run_batter = 1;
            } else {
                // Runners on base (including 2nd/3rd with 1st empty — previously a blind spot that
                // threw a wounding swing with run_base_runners = 0): drive or bunt to advance/score.
                strategy.style = hasPower ? AI_STYLE_NORMAL : AI_STYLE_BUNT;
                strategy.run_base_runners = 1;
                strategy.run_batter = 1;
            }
        }
    } else {
        if (halfInningState->strikes == 0) {
            strategy.style = AI_STYLE_NORMAL;
            strategy.run_base_runners = 0;
            strategy.run_batter = 0;
        } else if (halfInningState->strikes == 1) {
            strategy.style = hasPower ? AI_STYLE_NORMAL : AI_STYLE_BUNT;
            strategy.run_base_runners = 1;
            strategy.run_batter = 0;
        } else {
            strategy.style = hasPower ? AI_STYLE_NORMAL : AI_STYLE_BUNT;
            strategy.run_base_runners = 1;
            strategy.run_batter = hasPower;
        }
    }
    return strategy;
}

int should_change_batter(int fieldStatus, int power, int speed)
{
    if (fieldStatus == AI_FIELD_EMPTY) {
        return (speed > AI_SPEED_THRESHOLD) ? 0 : 1;
    }
    return (power > AI_POWER_THRESHOLD) ? 0 : 1;
}

// Which of the players allowed to bat this controller wants, given the field.
//
// This is the same preference `should_change_batter` has always expressed, said outright instead of
// walked. The old shape asked "am I happy with the one being shown?" and, when the answer was no,
// mutated engine state to be shown the next one — so the deliberation happened IN the world, took a
// frame or two per step for the click-simulation locks, and needed a cycle-detection guard plus a
// band-aid counter to stop it looping forever over a joker slot it kept skipping (bug #5). Choosing
// from a list needs none of that: there is nothing to loop.
//
// The preference order is preserved exactly. The old walk started at the engine's offer and stepped
// through the cycle, stopping at the first candidate it did not want to change away from, and
// falling back to where it started when it had seen them all. So: the first acceptable candidate in
// the caller's order, else the first candidate.
int choose_batter(const BatterCandidate* candidates, int count, int fieldStatus)
{
    if (count <= 0) return -1;
    for (int i = 0; i < count; i++) {
        if (!should_change_batter(fieldStatus, candidates[i].power, candidates[i].speed)) {
            return candidates[i].index;
        }
    }
    return candidates[0].index;
}

int is_wrong_pitch(float vx, float vy, float gravity, float plate_width)
{
    float v_x_abs = fabsf(vx);
    float t = vy * 2.0f / gravity;
    float offset = v_x_abs * t;
    if (offset > plate_width / 2.0f) {
        return 1;
    }
    return 0;
}

// Decide the direction (bat angle) of an AI swing.
//
// Returned value is in bat-angle units (0 = straight ahead, positive = left field, negative = right
// field; launch heading is theta = -2 * angle). Each style weights toward its real pesäpallo gaps
// rather than sending most balls into the crowded centre or foul lines:
//   - Style 0 (bunt): favors the open front-left and front-right pockets away from the pitcher.
//   - Style 1 (normal): full-field fan weighted toward the left and right seams, keeping centre live.
//   - Style 2 (wounding / koppi): aims into the fair deep outfield so a lofted koppi stays fair.
float calculate_ai_batting_angle(int battingStyle, int randomValue)
{
    int r = randomValue % AI_ANGLE_BUCKETS;
    if (r < 0) r += AI_ANGLE_BUCKETS;
    float t = r / (float)AI_ANGLE_BUCKETS; // [0, 1)

    if (battingStyle == AI_STYLE_WOUND) {
        return lerp_f(-AI_WOUND_BATTING_ANGLE, AI_WOUND_BATTING_ANGLE, t);
    }

    if (battingStyle == AI_STYLE_BUNT) {
        if (t < AI_BUNT_LEFT_WEIGHT) {
            return lerp_f(AI_BUNT_MIN_ANGLE, AI_BUNT_BATTING_ANGLE, t / AI_BUNT_LEFT_WEIGHT);
        }
        return lerp_f(
            -AI_BUNT_MIN_ANGLE, -AI_BUNT_BATTING_ANGLE, (t - AI_BUNT_LEFT_WEIGHT) / (1.0f - AI_BUNT_LEFT_WEIGHT)
        );
    }

    if (t < AI_NORMAL_SEAM_WEIGHT) {
        return lerp_f(-AI_MAX_BATTING_ANGLE, -AI_NORMAL_SEAM_INNER, t / AI_NORMAL_SEAM_WEIGHT);
    }
    if (t < AI_NORMAL_SEAM_WEIGHT + AI_NORMAL_CENTER_WEIGHT) {
        return lerp_f(
            -AI_NORMAL_SEAM_INNER, AI_NORMAL_SEAM_INNER, (t - AI_NORMAL_SEAM_WEIGHT) / AI_NORMAL_CENTER_WEIGHT
        );
    }
    float u = (t - (AI_NORMAL_SEAM_WEIGHT + AI_NORMAL_CENTER_WEIGHT)) / AI_NORMAL_SEAM_WEIGHT;
    return lerp_f(AI_NORMAL_SEAM_INNER, AI_MAX_BATTING_ANGLE, u);
}

SwingDecision decide_swing(int battingStyle, int rand_power, int rand_vertical)
{
    const SwingProfile* profile = swing_profile_for_style(battingStyle);
    float pFrac = (float)rand_power / AI_POWER_DRAW_MAX; // [0, 1] for rand_power in [0, 19)
    float offset = ((float)rand_vertical - AI_VERTICAL_DRAW_MID) / AI_VERTICAL_DRAW_MID; // [-1, 1] for [0, 201)

    SwingDecision decision;
    decision.power = profile->power_min + pFrac * profile->power_span;
    decision.vertical = SWING_VERTICAL_FOCAL + profile->vertical_bias + offset * profile->vertical_spread;

    if (decision.power < 0.0f) decision.power = 0.0f;
    if (decision.power > 1.0f) decision.power = 1.0f;
    if (decision.vertical > 1.0f) decision.vertical = 1.0f;
    if (decision.vertical < 0.0f) decision.vertical = 0.0f;

    return decision;
}
