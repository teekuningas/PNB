#include "pitching_ai_strategy.h"

// Semantic pitch-aim tuning. Every power in [0, 1] is §23-legal above PITCH_BASE_SPEED (0.106f).
// The direction jitter (±0.03) stays inside |x| < PLATE_WIDTH/2 even on a full power=1.0 tolppa
// (0.03 * 0.15 * 157 = 0.71 < 0.75).
#define PITCH_AI_DIR_STEP 0.01f // placement jitter per rand_dir step (stays in the strike zone)
#define PITCH_AI_BALL_OFFSET 0.25f // deliberate väärä placement when ahead in the count

// Explicit 5-band toss-height tables indexed by rand_power in [0, 5):
//   - empty bases: balanced low / medium / high variety so every at-bat has rhythm changes.
//   - runners on base, even count (strikes == 0): quicker low bias to limit runner jumps.
//   - runners on base, ahead (strikes >= 1): sharper low-quick vs high-tolppa contrast to
//     challenge batter timing and induce mistimed pop-up koppi.
static const float PITCH_HEIGHT_EMPTY[5] = {0.18f, 0.36f, 0.50f, 0.68f, 0.86f};
static const float PITCH_HEIGHT_RUNNERS_EVEN[5] = {0.12f, 0.26f, 0.45f, 0.64f, 0.84f};
static const float PITCH_HEIGHT_RUNNERS_AHEAD[5] = {0.10f, 0.24f, 0.48f, 0.78f, 0.94f};

static int clamp_power_bucket(int rand_power)
{
    if (rand_power < 0) return 0;
    if (rand_power > 4) return 4;
    return rand_power;
}

// Semantic AI pitch strategy — decide the DECLARED aim {power, direction} directly. A pure
// function of the base occupancy + count + three RNG draws supplied by the caller (so determinism
// stays the caller's concern). The AI commits at once, so both values are always declared.
//
//   power     : chosen from the explicit situational height table above (low quick tosses,
//               medium strikes, high tolppa).
//   direction : default to the plate centre (a strike) with tiny jitter that stays inside the
//               strike zone at any toss height. Ahead in the count (a strike already and no balls)
//               we occasionally place the pitch deliberately off the plate (a väärä) to tempt the
//               batter.
//
// rand_power in [0, 5), rand_dir in [0, 7), rand_choice in [0, 10).
PitchAim decide_pitch_aim(
    int batting_team_players_on_field_count, int strikes, int balls, int rand_power, int rand_dir, int rand_choice
)
{
    PitchAim aim;
    int bucket = clamp_power_bucket(rand_power);

    if (batting_team_players_on_field_count <= 1) {
        aim.power = PITCH_HEIGHT_EMPTY[bucket];
    } else if (strikes > 0) {
        aim.power = PITCH_HEIGHT_RUNNERS_AHEAD[bucket];
    } else {
        aim.power = PITCH_HEIGHT_RUNNERS_EVEN[bucket];
    }

    aim.direction = (rand_dir - 3) * PITCH_AI_DIR_STEP; // [0,7) -> [-3,3] steps; stays in the strike zone
    if (strikes != 0 && balls == 0) {
        if (rand_choice == 9) {
            aim.direction = PITCH_AI_BALL_OFFSET;
        } else if (rand_choice == 8) {
            aim.direction = -PITCH_AI_BALL_OFFSET;
        }
    }

    return aim;
}
