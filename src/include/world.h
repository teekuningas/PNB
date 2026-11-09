#ifndef WORLD_H
#define WORLD_H

// The physical World — MatchSession and everything it holds: the players, the ball, the catching team's
// control and posts, the actions in progress, the at-bat, the frame's events. Blittable: no pointers (a test
// holds that). It includes nothing of the legal world (rules_state.h) and nothing of a controller's memory,
// so code holding only this header can name neither.

#include "foundation.h"
#include "player_events.h"

typedef enum {
    PLAYER_STATE_IDLE = 0,
    PLAYER_STATE_AT_BAT,
    PLAYER_STATE_ON_BASE, // Replaces isOnBase=1
    PLAYER_STATE_RUNNING, // Replaces isOnBase=0, out=0, wounded=0
    PLAYER_STATE_ADVANCING_FREELY, // Replaces takingFreeWalk=1
    PLAYER_STATE_LEADING, // Replaces leading=1
    PLAYER_STATE_OUT, // Replaces out=1
    PLAYER_STATE_WOUNDED, // Replaces wounded=1
    PLAYER_STATE_SCORED // Explicit state for having scored
} PlayerUnitState;

// ball related information
typedef struct _BallInfo {
    Vector3D velocity;
    Vector3D location;
    Vector3D lastLocation;
    int visible; // is ball visible. ball is not visible when some player has it
    int moving; // is ball moving, only update player orientations and ball's position if ball is moving
    int currentFlightHasHitGround; // has the ball hit ground
    int onGround; // is ball rolling on ground
    int hitsGroundToUnWound; // if ball hits ground after being catched as wounding catch, is set to 1. checked only
                             // after wounding catch so, it is set to 0 when the catch is made.
    int needsMoveUpdate; // when ball having players' velocity changes, ball's velocity must change too
    int lastLastLocationUpdate; // when ball stops, we must sync lastLocation and location.

    // What the loose ball's current flight IS, beside what it is doing. Both are set when the ball leaves
    // a hand and gone by the time a hand has it again, which is the same lifetime as
    // currentFlightHasHitGround's — so they live with it, on the ball, rather than in a grab-bag of
    // "action info" beside it.
    //
    // A pitched ball in the air, not yet judged: set at the release, and over when the bat meets it or
    // the referee pronounces the pitch (the first time it touches the ground). Exactly that question and
    // no other — the windup is the catching action's, and pitch_is_being_delivered asks both.
    int pitch_in_flight;
    // The base a thrown ball was thrown toward, BASE_NONE for any other flight: set at the release and
    // cleared by the catch that ends it. Basemen read it so the one being thrown to stays on his base.
    BaseID thrown_to;

    // Who has the ball: the player holding it, -1 while it is loose. And who let go of it last — set by
    // every release from a hand (a pitch, a throw, a drop) and asked only while the ball is loose, so
    // that a fielder cannot catch what he has just thrown before it has touched the ground. A hit does
    // not set it, so a batted ball still names the pitcher.
    int holder;
    int released_by;
} BallInfo;
// The catching team's own engine state — facts about fielding, owned by the engine, written by
// ingestion and by the engine's own behaviours, read by both.
//
// `controlledMoveTarget` is what an INTENT_MOVE_TARGET becomes: nothing intent-shaped survives the
// gate, so the message is gone by the end of ingestion and only this destination remains. The
// `Active` flag is load-bearing rather than tidiness — a zeroed Vector3D is a real point on the
// field (the middle of the home area), so "nobody has said where yet" has to be representable as
// something other than the origin, or every reset would quietly send the fielder home.
typedef struct _CatchingTeamState {
    Vector3D controlledMoveTarget;
    int controlledMoveTargetActive; // 0 = no destination declared (fresh match, or after a reset)

    // WHO the destination was given to. A message says "the fielder I am steering goes here", and
    // the gate binds that to whoever is controlled at the top of the tick — the interpretation step
    // doing its job, turning a producer's sentence into engine terms.
    //
    // Without it a destination outlives the fielder it was meant for: control passes at the catch,
    // and the newcomer inherits a point that meant something for somebody standing somewhere else.
    // One frame, and small — but a destination that belongs to nobody in particular is an invalid
    // state left representable, and the producer re-declares on the very next frame anyway, so the
    // cost of being exact is a single frame of standing still.
    int controlledMoveTargetFor;

    // Where the engine reckons the ball can next be met: the prediction it ranks fielders against
    // and walks the auto-chasers to, and the point a catching controller aims its fielder at.
    // Computed from the ball's flight by the fielders' chase ranking (fielder_behaviours.c), whenever a
    // chase re-plan is asked for — not every frame.
    //
    // It lived in CameraState until the movement slice, which was simply the wrong drawer — a
    // fielding fact filed with the view vectors. Nothing about the camera ever read it; the comment
    // that said so was false, and the compiler had no way to notice.
    Vector3D ballTargetPoint;

    // CONTROL — which fielder the catching team's controller steers, and the fielders it can hand control
    // to. Control follows the ball: whoever catches it is controlled, a throw lets go of control, and the
    // ranking re-plans who chases (rank_players_and_move_them).
    int controlled_fielder; // -1 when nobody is controlled
    int chase_ranking[RANKED_FIELDERS_COUNT]; // the fielders best placed to meet the ball, best first
    int ranking_cursor; // which entry of chase_ranking control was last handed to; -1 when none was
} CatchingTeamState;
// spatial data for every player
typedef struct _TechnicalPlayerInfo {
    Vector3D location;
    Vector3D lastLocation;
    Vector3D homeLocation;
    Vector3D targetLocation;
    Vector3D velocity;
    Vector3D orientation;
} TechnicalPlayerInfo;

// MILESTONE 7.5 - Separating Control State from Domain State
typedef struct _PlayerRuntimeState {
    int arrivedToBase; // Optimization flag
    int passedPathPoint; // State machine variable
    int goingForward; // Direction tracking
    int hasMadeRunOnThirdBase; // Guard flag
} PlayerRuntimeState;

typedef enum { JOKER_REGULAR = 0, JOKER_AVAILABLE = 1, JOKER_USED = 2 } JokerStatus;

// batting team related flags.
typedef struct _BattingTeamPlayerInfo {
    int speed; // used to make some player a bit faster than others
    int power; // used to make some players bat a bit harder than others
    int number; // number is shown on screen
    JokerStatus joker; // 0 regular player, 1 has right to be used, 2 has been used already

    // MILESTONE 7 (DATA RENAISSANCE) - Type-safe state fields
    PlayerUnitState state;
    BaseID baseId;
} BattingTeamPlayerInfo;

typedef enum { TEAM_BATTING = 0, TEAM_CATCHING = 1 } TeamSide;

typedef enum { REPLACEMENT_IDLE = 0, REPLACEMENT_ACTIVE = 1 } ReplacementState;

typedef struct _CommonPlayerInfo {
    // player information. name is shown on the screen sometimes, team is used in players.c
    // and stats are used to make players behave in different ways on some situations.
    TeamSide team;

    int moving; // 0 doesnt, 1 moves
    int running; // 0 doesnt, 1 runs
    int looksForTarget; // used in conjuction with targetLocation when trying to go catching a ball for example
    int lastLastLocationUpdate; // player needs to have his lastLocation updated, necessary when controlling player and
                                // moving stops. twitching without this
} CommonPlayerInfo;

typedef struct _CatchingTeamPlayerInfo {
    int position; // pitcher, catcher, 1st baseman..
    int isNearHomeLocation; // used to do base replacing stuff.
    ReplacementState
        replacingStage; // 1 is going to replace or is at the base, 0 is coming back or is at home location.
    BaseID replacingBase; // in which base is the player replacing
    int busyCatching; // flag set when player is trying to run in hopes of catching the ball
    int throw_recovery_ticks; // Physical movement lock; decremented at the end of SIMULATE.
} CatchingTeamPlayerInfo;

typedef struct _PlayerInfo {
    TechnicalPlayerInfo tPI;
    CommonPlayerInfo cPI;
    CatchingTeamPlayerInfo cTPI;
    BattingTeamPlayerInfo bTPI;
    PlayerActivityEvents events; // Drained at the SIMULATE boundary and again by CLEAR.

} PlayerInfo;

// Which fielder keeps which base: the baseman of each base, and the fielder who covers it while its
// baseman is away. Set when a half-inning is set up and unchanged through it — the catching team's
// formation, not something a play moves.
typedef struct _FieldingPosts {
    int baseman[BASE_COUNT];
    int cover[BASE_COUNT];
} FieldingPosts;

// One-tick physical facts, drained by CLEAR. ACTUALIZE and SIMULATE produce them; ENFORCE can also
// start runner routes. Read by the referee and the tick's output assembly, never by controllers.
typedef struct _GameEvents {
    // Event flags (cleared each frame)
    int catchMade; // Fly ball was caught
    int playerArrivedAtBase; // Runner arrived at base

    int pitchReleased; // Ball left pitcher's hand
    int ballHitByBat; // Bat made contact
    int ballMissedByBat; // Bat swung and missed
    int ballHitGround; // Ball touched the ground this frame
    int freeWalkAccepted; // Player accepted free walk
    int batterEntered; // New: Signal that new batter is ready
    int homeward_runs_started; // Number of third-to-home routes begun this tick, before CLEAR.
} GameEvents;

// The two questions the engine puts to the batting side and waits on: who takes the bat next (§27), and
// whether the lead runner takes the free walk (§26). Consolidation raises each; its answer, a reset, or
// the situation that raised it going away closes it. Only that a question is OPEN lives here — what a
// free walk would give, and to whom, is the entitlement below, which has a lifetime of its own.
typedef struct _BattingOffers {
    int batter_seat; // a batter is wanted: somebody may be named to take the bat
    int free_walk; // the entitled runner may take the free walk, or decline it
} BattingOffers;

// §26: what the latest BALL entitles the batting side to — the lead runner, and the base he would walk
// from — worked out once per BALL. Each new BALL asks the question afresh; a reset leaves nothing to work
// out until the next one.
typedef struct _FreeWalkEntitlement {
    int worked_out; // this BALL's entitlement has been worked out (or there is nothing to work out)
    int runner; // the lead runner it entitles; -1 when nobody is
    BaseID from; // the base he would walk from; BASE_NONE when nobody is
} FreeWalkEntitlement;

// What the catching team's ball-holder is doing with the ball, if anything. One at a time, which is
// why it is one tag: a pitch and a throw both claim the same hands, and two independent flags would
// leave "both at once" representable.
typedef enum { CATCHING_ACTION_NONE = 0, CATCHING_ACTION_THROWING, CATCHING_ACTION_PITCHING } CatchingActionKind;

// A pitch being delivered. The power is known from the first frame, because it is what sizes the
// windup; the aim may arrive at any point before release. A windup that runs out unaimed is a
// valesyöttö, which needs no value of its own: it is simply a pitch that was never aimed.
typedef struct _PitchWindup {
    float power; // [0,1]
    int aimed; // 0 until a direction has been declared — 0.0 is a real direction, straight over the plate
    float direction;
} PitchWindup;

// A throw being gathered. The target is known from the first frame, because it is what the thrower
// turns to face; the power may arrive later, and the ball leaves once it has and the windup for it has
// elapsed. `distance` and `direction` are the line to the target, latched when the gather began.
typedef struct _ThrowWindup {
    BaseID target;
    int powered; // 0 until a power has been declared
    float power;
    float distance;
    Vector3D direction;
} ThrowWindup;

// The catching action in progress — the values a producer declared and the clock the engine runs them
// on, in ONE record because they are one fact: "is the ball-holder pitching or throwing, with what, and
// for how long". They used to be two halves, a declaration the producer drove through phases and a
// clock the engine kept beside it, and the producer had to read the declaration back to know which
// phase it was in. Now the engine owns the whole answer and producers contribute values to it.
//
// `timer` counts frames since the windup began and is the master of the cadence: it decides the
// release, and the animation follows it, never the reverse — it runs identically headless. The record
// is zeroed when the action resolves, which is the engine's "this one is done", and is nobody's
// message. Which half of the union means anything is said by `kind`, as with IntentMessage.
typedef struct _CatchingAction {
    CatchingActionKind kind;
    int timer;
    union {
        PitchWindup pitch;
        ThrowWindup throw;
    } as;
} CatchingAction;

// The swing, as the engine owns it — the values a producer has declared and the clock that consumes
// them, in ONE struct because they are one fact: "is a swing being assembled, with what, and how long
// until it is used". The catching side's CatchingAction is the same shape.
//
// The `Active` flags are load-bearing, not tidiness — the same argument `controlledMoveTargetActive`
// carries on the fielding side. Zero is a real power (the softest bunt) and a real vertical (the
// bottom of the sweep), so "nobody has declared one yet" cannot be represented by the value alone.
// Reading a value nobody set is not hypothetical here: the count the old code read at contact was
// reset by neither the per-at-bat init nor the power selection, so a batter who never clicked could
// swing on the PREVIOUS at-bat's number.
//
// There is no `phase` and no "window open" flag. How far along a swing is, is not a thing a producer
// tells the engine; and whether one may be declared at all is derived from durable world state at the
// gate (swing_may_be_declared), so there is no second copy of it to keep true.
typedef struct _SwingActualization {
    int contactFrame; // the at-bat's flight_frame on which the bat meets the ball; from the ball, at release
    int powerActive;
    float power;
    int verticalActive;
    float vertical;
    // The bat has already met this ball — set the moment the swing resolves, hit or miss. It is the
    // one fact behind two questions that used to be asked of two different things: the INGEST gate's
    // "is this swing still open?" and the actualizer's "have I already fired?". The actualizer used
    // to ask the referee's sticky batOutcome, which is legal state with a longer life than the play
    // it describes — the same reading that, tried at the gate, refused every power committed during
    // a windup. Here it lives with the swing and dies with it.
    int consumed;
} SwingActualization;

// Where the batting cycle has got to — one pitch faced, from the ready spot to the end of the follow-through.
// One value where there used to be three flags (ready, setup-pending, going-on) written at six sites, whose
// eight combinations held four meanings; measured over every golden run before the merge, those four were
// the only ones that ever occurred, so nothing was lost by making the other four unsayable.
typedef enum {
    BATTING_CYCLE_CLOSED = 0, // nobody at the ready spot: no batter yet, walking up, or set off for first
    BATTING_CYCLE_ARRIVED, // at the ready spot; the cycle opens, and the body is set up, once no pitch is in the air
    BATTING_CYCLE_AWAITING_PITCH, // open, standing ready, no ball released to him yet
    BATTING_CYCLE_FACING_PITCH // the ball was released to him; open until the follow-through has played out
} BattingCycle;

// The batter at the plate, as the engine runs him: one struct because it is one lifetime, the at-bat, and
// everything in it is about the one player standing on the arc. Inside the at-bat the batter goes round a
// BATTING CYCLE once per pitch he faces — he arrives at the ready spot, the cycle opens once no pitch is in
// the air, the ball is released to him, he swings or does not, and the cycle closes when the follow-through
// has played out and he walks back to the ready spot to begin the next.
typedef struct _AtBat {
    // §32: "Lyöntivuoroinen lyöjä saa edetä kotipesästä heti, kun pallo on irronnut lukkarin kädestä
    // ensimmäiseen syöttöön, riippumatta syötön laadusta." Granted when the FIRST pitch of the turn leaves
    // the hand. That it is then held for the rest of the turn is a READING, not the text: §32 names only
    // when it begins, and §17 lets a batter leave home and come back to bat until he has become a runner
    // for good. So no pitch clears it; seating the next batter starts it at 0, and so does a reset — a
    // foul's included, which the reading would not (an open question for the rules audit).
    int may_leave_home;

    BattingCycle cycle;

    SwingActualization swing; // the batter's declared values and the frame they are consumed on

    // The body through the cycle. Set up when the cycle opens; the engine walks it, never a producer.
    int flight_frame; // frames of the pitch's flight the batter has watched; contactFrame is counted on it
    int flight_clock_running; // the release has been seen and flight_frame is counting
    float arc_angle; // where on the arc round the pitching plate he stands; walked toward the declared aim
    float step_in; // how far he has stepped in toward the plate, a fraction of the batting radius
    float step_in_speed;
    float step_in_limit; // how far the body's shape lets him step in — a swing, a bunt or spread hands
    int swing_stopped; // withdrawn, or never begun: the bat is not coming
    int swing_started; // the swing's step-in and animation have begun
    int pose_dirty; // location and orientation are to be recomputed from arc_angle and step_in this frame
} AtBat;

// Re-plan the chase. Something has just changed the ball's flight — a throw, a hit or a drop sent it off,
// or it bounced, came to rest or met the fence — so the fielders' ranking and the point the ball can next
// be met are out of date. Whatever changed the flight raises the request; the ranking
// (rank_players_and_move_them) answers it later in the same tick and is the one thing that clears it, so
// nothing is ever pending at a frame boundary, and the state validator holds that.
//
// A throw, a hit and a drop also hand control to the best-placed fielder; a bounce or the fence only
// re-plans. One value and not two flags: "hand control over" without "re-plan" was representable as two
// flags and meant nothing. Raising never lowers (request_chase_replan), so a bounce in the tick of a hit
// keeps the hit's hand-over.
typedef enum { CHASE_REPLAN_NONE = 0, CHASE_REPLAN, CHASE_REPLAN_AND_HAND_OVER } ChaseReplan;

// The runners told to go on the next pitch, per base. A RUN_FORWARD on a settled runner arms him, and the
// engine fires the order: a base runner sets off when the next pitch leaves the hand, the batter when the
// bat meets the ball. Held until it fires or a RUN_BACK disarms it — so it outlives a valesyöttö and a
// missed swing, which is what "on the next pitch" means — and taken off the base the moment its runner
// leaves it by any run. Keyed by base and not by player because the order is "whoever is on this base",
// as the controller who gave it saw it.
typedef struct _RunnerCommitments {
    int armed[BASE_COUNT];
} RunnerCommitments;

typedef struct _MatchSession {
    PlayerInfo playerInfo[2 * PLAYERS_IN_TEAM + JOKER_COUNT];
    PlayerRuntimeState playerRuntime[2 * PLAYERS_IN_TEAM + JOKER_COUNT]; // Milestone 7.5 - Control state
    CatchingTeamState catchingState;
    FieldingPosts fielding_posts;
    GameEvents gameEvents; // MILESTONE 16 - Event notifications (Phase 1)
    BattingOffers offers;
    FreeWalkEntitlement free_walk;
    CatchingAction catching_action; // the pitch or throw in progress, if any
    AtBat at_bat;
    ChaseReplan chase_replan; // one tick long: raised by a change of flight, answered by the ranking
    RunnerCommitments runner_commitments;
    BallInfo ballInfo;

    // The engine's random stream. This is World state, not an ambient service: under the tick
    // equation World(T) = tick(World(T-1), messages), a snapshot that omits the seed cannot
    // re-tick to the same world. It therefore lives in the struct whose lifetime it matches (the
    // match) and rides every snapshot — including state_validator's memcpy of this struct, which
    // was silently incomplete while the seed was a main.c local.
    //
    // Set once at match start (initialize_game_from_menu) and then advanced only by engine draws.
    // No reset recipe clears it: it is one continuous stream for the whole match, not per-play state.
    unsigned int rngSeed;
} MatchSession;

#endif /* WORLD_H */
