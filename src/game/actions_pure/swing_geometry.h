#ifndef SWING_GEOMETRY_H
#define SWING_GEOMETRY_H

/*
 * The swing minigame's geometry — where the batter's two timed decisions live, as one small set of
 * knobs and the pure law they feed.
 *
 * THE LAW. The batter declares a vertical `d` in [0,1] and the engine turns it into the launch
 * elevation. The whole of it is:
 *
 *     V = SWING_ELEVATION_GAIN * ball_vy * (SWING_VERTICAL_FOCAL - d)
 *
 * `ball_vy` is the ball's vertical speed at the moment of contact (falling, so negative), which is
 * what makes a faster ball demand a more exact `d`: the same error in `d` becomes a bigger error in
 * elevation. The physics misses past VERTICAL_ANGLE_LIMIT.
 *
 * This replaces a four-term expression over two meter counts. Substituting the meter's DISPLAYED
 * position into the old form cancels its `scaleNumber`/`zeroNumber` apparatus completely, and POWER
 * DROPS OUT: that apparatus existed only to undo a meter whose scale depended on power. So the two
 * values a batter declares are independent in the physics — power sets magnitude, `d` sets
 * elevation — and only the WIDGET couples them, through the marker's top below.
 *
 * WHY THE KNOBS ARE HERE AND NOT SPREAD OUT. Each one alone controls a different property, and their
 * joint effect on how hard the game is has a closed form. That form is asserted by the unit tier
 * against acceptance bands, so these are not free numbers: the test says which COMBINATIONS are
 * playable, and a retune that breaks the difficulty curve fails the build rather than the playtest.
 * Turn them, then run the unit tier.
 */

// The sweet spot: the declared vertical that produces a level hit, whatever the power. Historically
// the meter's "4/13" mark, and — not by coincidence — the same value the pitch's aim meter settled on
// as PITCH_AIM_FOCAL: the two minigames have always been the same shape.
#define SWING_VERTICAL_FOCAL 0.30769231f

// How steeply elevation grows as the declared vertical leaves the sweet spot, per unit of ball speed.
#define SWING_ELEVATION_GAIN 364.0f

// The power ping-pong's half-length, in frames (a full there-and-back cycle is twice this). It alone
// decides how freely a power can be picked: the batter needs enough cycles inside the pitcher's
// windup to reach the level it wants, and nothing else depends on it.
#define SWING_POWER_SWEEP_FRAMES 20

// The vertical descent's length, in frames, when the flight is long enough to hold it. It alone
// decides the hit window: a longer sweep moves the marker slower past the sweet spot. This is the
// knob to turn if the swing feels too tight — the bands allow up to 70.
#define SWING_VERTICAL_SWEEP_FRAMES 52

// How many frames before contact the descent must be finished. Two jobs: it guarantees the value is
// in the world BEFORE the frame that consumes it (the margin a late message will one day need), and
// it clamps the sweep on the shortest flights, which is what stops the lowest toss being far easier
// than every other pitch.
#define SWING_LEAD_FRAMES 10

// An experience-based nudge onto the frame the bat actually meets the ball, added to the ballistic
// solution. Shared by the engine and the client so both size the same window from one definition.
#define SWING_CONTACT_TWEAK_FRAMES 3

// The launch elevation a declared vertical produces against a ball falling at `ball_vy`. Zero when
// the vertical is exactly at the sweet spot; the sign follows the ball's own (a falling ball has
// negative vy, so a vertical above the focal lofts and one below drives it down).
float swing_vertical_angle(float vertical, float ball_vy);

// Where the vertical marker starts for a given declared power [0,1] — the SECOND difficulty axis, and
// a client-side concern only (the engine never asks). It runs from exactly the sweet spot at zero
// power, where the top IS a level hit and a bunt cannot be mistimed into loft, up to 1.0 at full
// power, where the marker must travel three times as far and moves three times as fast. That is what
// makes a bunt the safe option and a full swing the risky one, without either costing a rule.
float swing_marker_top(float power);

// How long the vertical descent may run given the frames remaining until contact: the full sweep
// where the flight can hold it, clamped by the lead where it cannot. Never zero — a sweep of one
// frame is still a (very hard) decision, where a sweep of none would be an unrepresentable state.
int swing_vertical_sweep_frames(int frames_to_contact);

#endif /* SWING_GEOMETRY_H */
