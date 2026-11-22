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
 * values a batter declares are independent — power sets magnitude, `d` sets elevation — and since
 * 2026-09-03 nothing couples them anywhere. The widget used to, through a marker whose top rose with
 * the declared power; that was the last trace of the old apparatus, and it made the sweet spot fall
 * at a different moment for every power, so no rhythm could be learned. The marker now crosses the
 * whole bar whatever was declared, which puts the sweet spot at a fixed 1 - FOCAL of the way down —
 * the SAME fraction, in the same direction, as the pitcher's aim descent. The two minigames were
 * always the same shape; now they are also the same gesture.
 *
 * WHY THE KNOBS ARE HERE AND NOT SPREAD OUT. Each one alone controls a different property, and their
 * joint effect on how hard the game is has a closed form. That form is asserted by the unit tier
 * against acceptance bands, so these are not free numbers: the test says which COMBINATIONS are
 * playable, and a retune that breaks the difficulty curve fails the build rather than the playtest.
 * Turn them, then run the unit tier.
 */

// The sweet spot: the declared vertical that produces a level hit, whatever the power. Historically
// the meter's "4/13" mark, and — not by coincidence — the same value the pitch's aim meter settled on
// as PITCH_AIM_FOCAL. Since the marker crosses the whole bar it is also a POSITION and not only a
// value: the marker falls from 1 and the level hit is at this mark, every pitch, every power.
#define SWING_VERTICAL_FOCAL 0.30769231f

// How steeply elevation grows as the declared vertical leaves the sweet spot, per unit of ball speed.
#define SWING_ELEVATION_GAIN 364.0f

// The power ping-pong's half-length, in frames: the marker rises for this many frames and falls for
// the same, ONCE, and that is the whole power decision — exactly the pitch's gesture. Sized so a full
// there-and-back fits inside the SHORTEST windup any pitcher can throw, so the beat always completes
// before the ball is released whatever the toss. It alone decides how freely a power can be picked.
//
// A looping sweep was tried first and is wrong: it gives the batter unlimited time to pick a level,
// which is not a decision, and it leaves a meter oscillating on screen with nothing at stake.
#define SWING_POWER_SWEEP_FRAMES 36

// The vertical descent's length, in frames, when the flight is long enough to hold it. It sets how
// fast the marker travels — but NOT, mostly, how hard the swing is, and that surprise is worth
// recording. Because the sweep clamps to the flight, stretching it past about 100 stops widening the
// hit window (52 -> 120 buys only 154ms -> 304ms at the hard end) and flattens the difficulty
// gradient on the way, since a sweep proportional to the flight cancels the ball speed out of the
// tolerance. If the swing needs to be MORE FORGIVING, VERTICAL_ANGLE_LIMIT is the knob; this one is
// for how the meter reads.
#define SWING_VERTICAL_SWEEP_FRAMES 70

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

// (There is no marker_top any more, and its absence is the point. It used to start the descent at a
// power-dependent height, which meant a bunt's marker began ON the sweet spot and a full swing's
// began three times above it — so the moment to press moved with a decision the batter had already
// made, and the two gestures looked coupled where the physics says they are not. The declared
// vertical is now simply where the marker is: the widget maps the bar to [0,1] and there is nothing
// left here to ask. What it cost is a second difficulty axis; what the game keeps is the physical
// one — a faster ball demands a more exact d — which is the pitcher's lever, so the duel now has one
// lever each rather than one of them doubled.)

// How long the vertical descent may run given the frames remaining until contact: the full sweep
// where the flight can hold it, clamped by the lead where it cannot. Never zero — a sweep of one
// frame is still a (very hard) decision, where a sweep of none would be an unrepresentable state.
int swing_vertical_sweep_frames(int frames_to_contact);

#endif /* SWING_GEOMETRY_H */
