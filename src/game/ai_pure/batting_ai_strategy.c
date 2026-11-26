#include "batting_ai_strategy.h"
#include "rules_state.h"
#include "actions_pure/swing_geometry.h"
#include <math.h>
#include <stdlib.h>

#define AI_POWER_THRESHOLD 2
#define AI_SPEED_THRESHOLD 2

// Largest bat angle (each side) the AI aims for. The batting system doubles arc_angle into the
// launch heading (theta = -arc_angle*2). The front foul lines sit at ~±0.74 rad and then run
// parallel at x = ±31.2 past z = -33, so trimming normal swings to ±0.36 rad (±0.72 rad launch)
// keeps full-field seam pressure while cutting side-line foul clipping.
// Bunts aim predominantly down the 1st-base line (+0.18..+0.32 rad, pulling the 1st baseman off
// the bag) with a small right-pocket mix (15%) for human-facing variety, staying clear of the
// lukkari near centre (|a| >= 0.18). Wounding koppi stay inside ±0.22 rad (fair deep outfield).
#define AI_MAX_BATTING_ANGLE 0.36f
#define AI_BUNT_LEFT_MAX_ANGLE 0.32f
#define AI_BUNT_RIGHT_MAX_ANGLE 0.30f
#define AI_WOUND_BATTING_ANGLE 0.22f

#define AI_BUNT_MIN_ANGLE 0.18f
#define AI_BUNT_LEFT_WEIGHT 0.85f

#define AI_NORMAL_SEAM_INNER 0.14f
#define AI_NORMAL_SEAM_WEIGHT 0.38f
#define AI_NORMAL_CENTER_WEIGHT 0.24f

// Loose fair-ball advance thresholds (after fair ground contact, so koppi-safe):
//   - a breakthrough past the boundary lines must be ahead of home plate (z < -10),
//   - a fair ball still loose in the deep outfield past the middle fielders (middle_right sits at
//     z = -48.8) lets stationary runners (e.g. strike-0 probes) break safely without running into
//     a waiting infielder's hands.
#define AI_BREAKTHROUGH_MIN_Z -10.0f
#define AI_DEEP_GAP_Z -58.0f

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
// Note that batting_physics adds a +5*0.05 = +0.25 rad (+14.3°) base elevation at V = 0, so
// normal swings center below FOCAL (-0.018 ± 0.042) to produce mostly fair grounders and line
// drives while keeping an explicit upper tail for lofted drives, catchable koppi, and rare fouls.
static const SwingProfile AI_SWING_PROFILES[3] = {
    [AI_STYLE_BUNT] = {.power_min = 0.30f, .power_span = 0.14f, .vertical_bias = -0.042f, .vertical_spread = 0.015f},
    [AI_STYLE_NORMAL] = {.power_min = 0.66f, .power_span = 0.30f, .vertical_bias = -0.018f, .vertical_spread = 0.042f},
    [AI_STYLE_WOUND] = {.power_min = 0.78f, .power_span = 0.00f, .vertical_bias = 0.048f, .vertical_spread = 0.018f},
};

static const SwingProfile* swing_profile_for_style(AIBattingStyle battingStyle)
{
    if (battingStyle == AI_STYLE_BUNT) return &AI_SWING_PROFILES[AI_STYLE_BUNT];
    if (battingStyle == AI_STYLE_WOUND) return &AI_SWING_PROFILES[AI_STYLE_WOUND];
    return &AI_SWING_PROFILES[AI_STYLE_NORMAL];
}

static float lerp_f(float a, float b, float u)
{
    return a + u * (b - a);
}

BattingStrategy calculate_batting_strategy(
    const HalfInningState* halfInningState, AIFieldOccupancy fieldStatus, int power, int speed, int period
)
{
    BattingStrategy strategy = {AI_STYLE_NORMAL, 0, 0};
    int hasPower = (power > AI_POWER_THRESHOLD) ? 1 : 0;
    int isFast = (speed > AI_SPEED_THRESHOLD) ? 1 : 0;

    if (period < 4) {
        if (halfInningState->strikes == 0) {
            // Strike 0: probe with a normal swing and hold runners at their bases. Gives the
            // pitcher room to mix väärä and avoids cheap koppi wounds/burns, while deep fair gap
            // hits and breakthroughs still advance everyone via should_advance_on_loose_ball.
            strategy.style = AI_STYLE_NORMAL;
            strategy.run_base_runners = 0;
            strategy.run_batter = 0;
        } else if (halfInningState->strikes == 1) {
            if (fieldStatus == AI_FIELD_EMPTY) {
                strategy.style = isFast ? AI_STYLE_BUNT : AI_STYLE_NORMAL;
                strategy.run_base_runners = 0;
                strategy.run_batter = 1;
            } else {
                // Runners on base: drive the ball rather than forcing low-power batters into
                // suicide bunts against waiting infielders.
                strategy.style = AI_STYLE_NORMAL;
                strategy.run_base_runners = 1;
                strategy.run_batter = isFast;
            }
        } else if (halfInningState->strikes == 2) {
            if (fieldStatus == AI_FIELD_EMPTY) {
                strategy.style = isFast ? AI_STYLE_BUNT : AI_STYLE_WOUND;
                strategy.run_base_runners = 0;
                strategy.run_batter = 1;
            } else {
                // Final try with runners on base: drive the ball and send everyone.
                strategy.style = AI_STYLE_NORMAL;
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
            strategy.style = AI_STYLE_NORMAL;
            strategy.run_base_runners = 1;
            strategy.run_batter = 0;
        } else {
            strategy.style = AI_STYLE_NORMAL;
            strategy.run_base_runners = 1;
            strategy.run_batter = hasPower;
        }
    }
    return strategy;
}

int should_change_batter(AIFieldOccupancy fieldStatus, int power, int speed)
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
int choose_batter(const BatterCandidate* candidates, int count, AIFieldOccupancy fieldStatus)
{
    if (count <= 0) return -1;
    for (int i = 0; i < count; i++) {
        if (!should_change_batter(fieldStatus, candidates[i].power, candidates[i].speed)) {
            return candidates[i].index;
        }
    }
    return candidates[0].index;
}

int is_wrong_pitch(float x, float y, float vx, float vy, float gravity, float plate_width)
{
    if (gravity <= 0.0f) return 0;
    float height = (y > 0.0f) ? y : 0.0f;
    float disc = vy * vy + 2.0f * gravity * height;
    float t = (vy + sqrtf(disc)) / gravity;
    float landing_x = x + vx * t;
    if (fabsf(landing_x) > plate_width / 2.0f) {
        return 1;
    }
    return 0;
}

int should_advance_on_loose_ball(
    BatOutcome bat_outcome, int catch_has_been_made, FoulPlayState foul_state, BaseID thrown_to, int ball_holder,
    int ball_moving, int has_ball_hit_ground, float ball_z, int ball_out_of_bounds
)
{
    if (bat_outcome != BAT_OUTCOME_HIT || catch_has_been_made != 0 || foul_state != FOUL_STATE_NONE ||
        thrown_to != BASE_NONE || ball_holder != -1 || ball_moving != 1 || has_ball_hit_ground != 1) {
        return 0;
    }
    if (ball_out_of_bounds && ball_z < AI_BREAKTHROUGH_MIN_Z) {
        return 1;
    }
    if (ball_z < AI_DEEP_GAP_Z) {
        return 1;
    }
    return 0;
}

// Decide the direction (bat angle) of an AI swing.
//
// Returned value is in bat-angle units (0 = straight ahead, positive = left field, negative = right
// field; launch heading is theta = -2 * angle). Each style weights toward its real pesäpallo gaps
// rather than sending most balls into the crowded centre or foul lines:
//   - AI_STYLE_BUNT: favors the 1st-base line (left pocket) to pull the 1st baseman off his bag,
//     with a small right-pocket mix for variety and a centre exclusion away from the lukkari.
//   - AI_STYLE_NORMAL: full-field fan weighted toward the left and right seams, keeping centre live.
//   - AI_STYLE_WOUND: aims into the fair deep outfield so a lofted koppi stays fair.
float calculate_ai_batting_angle(AIBattingStyle battingStyle, int randomValue)
{
    int r = randomValue % AI_ANGLE_BUCKETS;
    if (r < 0) r += AI_ANGLE_BUCKETS;
    float t = r / (float)AI_ANGLE_BUCKETS; // [0, 1)

    if (battingStyle == AI_STYLE_WOUND) {
        return lerp_f(-AI_WOUND_BATTING_ANGLE, AI_WOUND_BATTING_ANGLE, t);
    }

    if (battingStyle == AI_STYLE_BUNT) {
        if (t < AI_BUNT_LEFT_WEIGHT) {
            return lerp_f(AI_BUNT_MIN_ANGLE, AI_BUNT_LEFT_MAX_ANGLE, t / AI_BUNT_LEFT_WEIGHT);
        }
        return lerp_f(
            -AI_BUNT_MIN_ANGLE, -AI_BUNT_RIGHT_MAX_ANGLE, (t - AI_BUNT_LEFT_WEIGHT) / (1.0f - AI_BUNT_LEFT_WEIGHT)
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

SwingDecision decide_swing(AIBattingStyle battingStyle, int rand_power, int rand_vertical)
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
