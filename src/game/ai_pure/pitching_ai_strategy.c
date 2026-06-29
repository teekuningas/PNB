#include "pitching_ai_strategy.h"
#include "actions_pure/pitching_physics.h"

// Semantic pitch-aim tuning (feel knobs; tune at the graphical pass). The direction jitter is kept small
// enough that the AI's "strike" pitches land inside |x| < PLATE_WIDTH/2 even at the highest toss height.
#define PITCH_AI_POWER_BASE 0.5f
#define PITCH_AI_POWER_STEP 0.08f // toss-height jitter per rand_power step
#define PITCH_AI_DIR_STEP 0.01f // placement jitter per rand_dir step (stays in the strike zone)
#define PITCH_AI_BALL_OFFSET 0.3f // deliberate väärä placement when ahead in the count

// Semantic AI pitch strategy — decide the DECLARED aim {power, direction} directly (replaced the legacy
// calculate_ai_pitch_targets, which returned meter levels to press at). A pure function of the count + how
// many batters remain + three RNG draws supplied by the caller (so determinism stays the caller's
// concern). The AI commits at once, so both windows are always declared.
//
//   power     : a medium toss with small jitter. With only one batter left we throw a steady fixed toss
//               (no jitter) — the legacy "rand1=0 when one player on field".
//   direction : default to the plate centre (a strike) with tiny jitter that stays inside the strike zone
//               at any toss height (direction=0 is a dead-centre strike regardless of power). Ahead in the
//               count (a strike already and no balls) we occasionally place the pitch deliberately off the
//               plate (a väärä) to tempt the batter — the legacy var=±10.
//
// rand_power in [0, 5), rand_dir in [0, 7), rand_choice in [0, 10) (mirrors the legacy rand3 range).
PitchAim decide_pitch_aim(
    int batting_team_players_on_field_count, int strikes, int balls, int rand_power, int rand_dir, int rand_choice
)
{
    PitchAim aim;

    if (batting_team_players_on_field_count == 1) {
        aim.power = PITCH_AI_POWER_BASE;
    } else {
        aim.power = PITCH_AI_POWER_BASE + (rand_power - 2) * PITCH_AI_POWER_STEP; // [0,5) -> [-2,2] steps
    }

    aim.direction = (rand_dir - 3) * PITCH_AI_DIR_STEP; // [0,7) -> [-3,3] steps; stays in the strike zone
    if (strikes != 0 && balls == 0) {
        if (rand_choice == 9) {
            aim.direction = PITCH_AI_BALL_OFFSET;
        } else if (rand_choice == 8) {
            aim.direction = -PITCH_AI_BALL_OFFSET;
        }
    }

    // The AI produces a complete aim at once; there is no gathering and nothing left undeclared.
    return aim;
}
