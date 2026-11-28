#ifndef RULES_PITCH_RIGHTS_H
#define RULES_PITCH_RIGHTS_H

/*
 * §24 Syöttöoikeus — pitch rights state transitions.
 *
 * After the previous batter becomes permanently a runner (§18), the lukkari earns the right to pitch
 * to the next batter once the ball has been held by a fielder at home, then at any field base
 * (1st, 2nd, or 3rd), and then back at home (Home -> Field Base -> Home).
 *
 * Pure: no World reads, no hidden state. The referee owns HalfInningState.pitch_rights and calls
 * these transitions each tick; the home-run contest is exempt per §8.
 */

#include "rules_state.h"

/**
 * @brief Advance §24 pitch rights given where a fielder currently holds the ball.
 *
 * @param current      Current pitch-rights stage.
 * @param ball_at_base Base index where the ball is held (0 = home, 1..3 = field bases, -1 = elsewhere).
 * @return Next pitch-rights stage.
 */
PitchRightsState advance_pitch_rights(PitchRightsState current, int ball_at_base);

/**
 * @brief Pitch rights after a foul-play reset (§29, §24).
 *
 * On a 1st- or 2nd-strike foul (strikes_at_pitch_start < 2), the same batter returns to the plate
 * and pitch rights remain granted. On a 3rd-strike foul, the batter burns (§18(6)) and the next
 * batter requires a fresh Home -> Field Base -> Home round-trip.
 */
PitchRightsState pitch_rights_after_foul_reset(int strikes_at_pitch_start);

#endif /* RULES_PITCH_RIGHTS_H */
