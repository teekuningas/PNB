#ifndef CLIENT_INPUT_H
#define CLIENT_INPUT_H

// The human controller's private memory — ClientInputState and its widgets. Client-local: never in the
// World, never on the wire, in no snapshot. The engine includes neither this header nor any that includes
// it, so no stage can name what a human is in the middle of doing (make guardrails holds that).

#include "foundation.h"

// Client-local input layer. Input *interpretation* memory — tap
// windows, hold-charge, meter widgets — belongs to the input SOURCE (a client), NOT to the shared
// physical world. It is therefore deliberately NOT part of MatchSession (the blittable peer / wire unit,
// §9): it lives as a sibling in StateInfo, so a stage holding only MatchSession* physically cannot read
// it, and a peer snapshot never ships one client's tap-timing to another. One instance per process (this
// machine's local input, covering whichever team(s) this process drives). The AI never uses any of it —
// it declares intent directly.
// How a widget's cursor moves — the client-local INPUT MECHANISM, shared by every minigame (pitch, throw,
// and the swing). The AI never uses any of this; it declares values directly (producer symmetry).
// The mode governs advance_widget's motion and gates whether the meter is displayed (IDLE = not shown):
//   PING_PONG — 0 → max → 0 then stop        (pitch power; a click samples the level, right peak = max).
//   DESCENT   — starts at max, → 0 then stop  (pitch aim; slow and aimable, resting at 0 when it runs out
//               unclicked = valesyöttö pending).
//   CHARGE    — 0 → max then HOLD at max      (throw power; hold-release — the level is sampled on release).
// The swing uses PING_PONG then DESCENT, the same two the pitch uses. A looping ping-pong was tried
// for the swing's power and removed: a meter that never runs out gives the batter unlimited time,
// which is not a decision, and it shows nothing at stake so there is no cue to press.
typedef enum { WIDGET_IDLE = 0, WIDGET_PING_PONG, WIDGET_DESCENT, WIDGET_CHARGE } WidgetMode;

// A meter the human samples to convert input TIMING into a declared VALUE — the client-local sampler the AI
// never needs (it declares values from strategy directly). A read takes counter/counter_max as the chosen
// [0,1] level: a click samples it (pitch), or a key-release samples it (throw charge). This is pure input
// interpretation, NOT engine timing — the engine's deterministic clock (CatchingAction.timer) is
// separate and authoritative; this only turns a human's input moment into a number.
// The same struct serves the pitch (PING_PONG power then DESCENT aim), the throw (CHARGE power) and the
// swing (PING_PONG power then DESCENT elevation) — one general input widget, per the
// engine↔client contract.
// The human's steering memory: which direction keys were held when it last spoke, and for whom.
// Client-local like the pitch and throw widgets — it never crosses the wire and is in no snapshot.
//
// It exists because the engine is told a destination and the human presses directions: something has
// to notice that the held set CHANGED, and that something is the client, not the engine. Holding a
// key is not a stream of events, so a producer that re-sent its destination every frame would be
// spending messages to say what the engine already knows.
#define MOVE_HELD_UP 1
#define MOVE_HELD_RIGHT 2
#define MOVE_HELD_DOWN 4
#define MOVE_HELD_LEFT 8

typedef struct _MoveWidget {
    int held; // the held direction set as last declared (MOVE_HELD_* bits)
    int controlIndex; // the fielder it was declared for; a different one needs telling afresh
    int declared; // 0 until the first declaration
    int framesSinceDeclared; // drives the blind heartbeat below
    Vector3D point; // the destination last sent
} MoveWidget;

typedef struct _InputWidget {
    int counter; // cursor position in [0, counter_max]
    int counter_max; // sweep length (0 = never armed)
    int dir; // +1 rising, -1 falling, 0 = stopped or holding (edge reached, or not started)
    WidgetMode mode; // how the cursor moves (also gates whether it is displayed)
} InputWidget;

// The human's batter cursor: which of the legal candidates is highlighted, and whether a choice has
// been made. Client-local, like the pitch gesture and the steering widget — the engine never learns
// that a human was scrolling through a list, only which player was finally named.
//
// `highlight` is an INDEX INTO THE CANDIDATE LIST, not a player, and that is what keeps it from
// going stale. The list is rebuilt from the world every frame (list_batter_candidates), so a run
// scored mid-decision that re-designates the last batter simply shortens the list, and the cursor is
// clamped back into it — where the engine's old offer had to be actively withdrawn by consolidation
// when the same thing happened.
//
// `selected` is the gesture, held here because §27 holds it here too: the choice is the team's until
// the bat is taken ("Lyömään asettunutta pelaajaa ei voi vaihtaa pois lyömästä"), and until then it
// is nobody else's business. It makes the human restate the choice every frame the decision is open,
// which is what lets the engine seat the batter on the first tick the seat is free without the
// producer knowing anything about home safety.
// The human's swing gesture: one cursor that is a looping power ping-pong during the windup and a
// one-way vertical descent once the ball is in the air, plus the two things the client must remember
// on its own side of the boundary.
//
// `power` is the client's OWN copy of what it declared, and it has to be: reading the engine's copy
// back to scale the descent is exactly the read-back the fourth law forbids. The producer's unfinished
// gesture lives in the producer.
//
// `flightFrames` is solved ONCE, on the frame the client saw the ball leave the hand, and from the
// LAUNCH velocity. That is not an optimisation, it is the whole correctness of the descent: the
// ball's vertical speed decays toward zero as it rises, so a flight time recomputed from the current
// velocity shrinks as the pitch climbs, and a batter who commits late gets a sweep of a frame or two
// — a marker that drops like a stone. The engine solves its contact frame exactly once for the same
// reason, and the two agree because they use the same pure solution on the same launch.
// Which beat of the gesture the batter is on. The client's OWN account of what it has said, which is
// the only account it is allowed to keep: the engine's `SwingActualization` is where a declaration
// ARRIVES, and asking it "have I declared yet" is the read-back the fourth law forbids. Locally the
// two agree within a tick, so the read looked harmless; across a wire they do not. A message stamped
// for tick T+D leaves the engine's copy saying "no power declared" for D ticks after the human
// pressed — long enough for a gesture keyed off it to re-arm the sweep the human just spent and
// invite a second commitment on the same pitch.
//
// One enum rather than a bag of bits, so "declined" and "committed" cannot both be true and the
// meaningless combinations are unrepresentable. It is a controller's private memory, exactly as the
// AI's swing plan (`AISwingPlan.decided`) is: the same fact, on the other side of the same boundary.
typedef enum {
    SWING_BEAT_POWER = 0, // the power sweep is running, or about to arm; nothing said yet
    SWING_BEAT_LOADED, // a power has been declared; the elevation is the beat still outstanding
    SWING_BEAT_COMMITTED, // both values are out; there is nothing further to say this pitch
    SWING_BEAT_DECLINED // the one power sweep ran out unpressed — this batter is not swinging
} SwingBeat;

typedef struct _SwingWidget {
    InputWidget meter;
    int flightFrames; // -1 while no ball is in the air; solved once at release, from the launch
    SwingBeat beat; // where this gesture has got to, remembered here and nowhere else
    float power; // the power this gesture declared; only meaningful once one has been
} SwingWidget;

// The human's pitch gesture: one cursor that is a power ping-pong before the windup and an aim descent
// during it, and the power it declared. `power` is the client's own copy, kept so the bar can go on
// showing the committed power while the aim is chosen — the swing's committed mark, on the pitching
// side. Reading the engine's copy back instead would be correct only while a declaration lands inside
// the tick that made it.
typedef struct _PitchWidget {
    InputWidget meter;
    float power;
} PitchWidget;

typedef struct _BatterSelectWidget {
    int highlight; // cursor position within the current candidate list
    int selected; // 1 once the human has accepted the highlighted candidate
} BatterSelectWidget;

typedef struct _ClientInputState {
    // Per-base countdown after a base-run key release: a second release while >0 is a double-press
    // (RUN_COMMIT, "run now"); a lone press is RUN_FORWARD/RUN_BACK. Decremented each frame in
    // action_invocations.c. Human path only — the AI declares RUN_COMMIT directly.
    int run_press_window[BASE_COUNT];

    // The pitch gesture the human runs across the dance (power ping-pong → aim descent). Human path only.
    PitchWidget pitchWidget;

    // The throw charge meter — a CHARGE widget that rises while the human holds KEY_2 and is sampled on
    // release to declare the throw power as a VALUE (the engine↔client contract). Self-contained: it runs
    // its own timing and never reads the engine's windup clock (which drives only the render animation).
    // Human path only — the AI declares the target and the power together, using no widget.
    InputWidget throwWidget;

    // The steering widget — held arrows in, a destination out. Human path only; the AI keeps the
    // same memory on its own side of the boundary, in AIControllerState.
    MoveWidget moveWidget;

    // The batter cursor — a list to scroll and a choice to make. Human path only; the AI needs no
    // widget because it has no gesture, it just picks.
    BatterSelectWidget batterWidget;

    // The swing gesture — power during the windup, elevation during the flight. Human path only; the
    // AI needs no widget because it has no gesture, it decides two numbers and says them.
    SwingWidget swingWidget;

    // The aim cursor: where on the batter's arc the held +/- keys have walked to. Human path only —
    // the AI decides an angle outright and declares it. Cleared whenever there is no batting going
    // on, so one at-bat's aim never leaks into the next.
    float batterAim;
} ClientInputState;

#endif /* CLIENT_INPUT_H */
