#include "rules_state.h"
#include "world.h"
#include "session.h"
#include "client_input.h"
#include "ai/ai_memory.h"
// The version-specific half of the golden projection: how this tree's structs answer the semantic
// questions golden.c asks. Edit THIS file when a refactor reshapes a struct, keeping every line it prints
// the same. Values a refactor legitimately leaves stale (a union half nobody is using, a scratch field the
// next action overwrites) are printed only while they mean something, so a trace records play and not
// leftovers.

// 0 none, 1 throwing, 2 pitching
static int catching_action_code(const MatchSession* m)
{
    switch (m->catching_action.kind) {
    case CATCHING_ACTION_THROWING: return 1;
    case CATCHING_ACTION_PITCHING: return 2;
    case CATCHING_ACTION_NONE: return 0;
    default: return 9;
    }
}

static void project_pitch(const MatchSession* m)
{
    const CatchingAction* a = &m->catching_action;
    if (catching_action_code(m) == 2) {
        put(5, "winding t=%d pw=%a aimed=%d", a->timer, (double)a->as.pitch.power, a->as.pitch.aimed);
        if (a->as.pitch.aimed) put(5, " d=%a", (double)a->as.pitch.direction);
    } else {
        put(5, "-");
    }
}

static void project_throw(const MatchSession* m)
{
    const CatchingAction* a = &m->catching_action;
    if (catching_action_code(m) == 1) {
        put(6, "gathering t=%d tgt=%d powered=%d", a->timer, (int)a->as.throw.target, a->as.throw.powered);
        if (a->as.throw.powered) put(6, " pw=%a", (double)a->as.throw.power);
        put(6, " dist=%a dir=", (double)a->as.throw.distance);
        put(6, "%a,%a;", (double)a->as.throw.direction.x, (double)a->as.throw.direction.z);
    } else {
        put(6, "-");
    }
    // Which base a loose ball was thrown toward. Printed only while the ball IS loose: every reader asks it
    // with nobody holding the ball, so what it says while a hand holds one is a leftover, not play.
    if (m->ballInfo.holder == -1) put(6, " going=%d", m->ballInfo.thrown_to);
}

static void project_widget(const char* name, const InputWidget* w)
{
    put(9, "%s=%d,%d,%d,%d;", name, w->counter, w->counter_max, w->dir, (int)w->mode);
}

static void project(const StateInfo* st)
{
    const MatchSession* m = st->match;
    const GameRulesState* r = st->rules;

    for (int i = 0; i < 2 * PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        const PlayerInfo* p = &m->playerInfo[i];
        put(0, "[%d]", i);
        V(0, p->tPI.location);
        V(0, p->tPI.lastLocation);
        V(0, p->tPI.targetLocation);
        V(0, p->tPI.velocity);
        V(0, p->tPI.orientation);
        put(0, "c%d,%d,%d,%d,%d,%d,%d,%d,%d;", p->cPI.team, p->cPI.moving, p->cPI.running, p->cPI.looksForTarget,
            p->cPI.lastLastLocationUpdate, (int)p->cPI.model, p->cPI.animationStage, p->cPI.animationStageCount,
            p->cPI.animationFrequency);
        put(0, "f%d,%d,%d,%d,%d,%d;", p->cTPI.position, p->cTPI.isNearHomeLocation, (int)p->cTPI.replacingStage,
            (int)p->cTPI.replacingBase, p->cTPI.busyCatching, p->cTPI.throwRecoil);
        put(0, "b%d,%d,%d,%d,%d,%d;", p->bTPI.speed, p->bTPI.power, p->bTPI.number, (int)p->bTPI.joker,
            (int)p->bTPI.state, (int)p->bTPI.baseId);
        const PlayerRuntimeState* rt = &m->playerRuntime[i];
        put(0, "r%d,%d,%d,%d;", rt->arrivedToBase, rt->passedPathPoint, rt->goingForward, rt->hasMadeRunOnThirdBase);
    }

    const BallInfo* b = &m->ballInfo;
    V(1, b->location);
    V(1, b->lastLocation);
    V(1, b->velocity);
    put(1, "%d,%d,%d,%d,%d,%d,%d", b->visible, b->moving, b->currentFlightHasHitGround, b->onGround,
        b->hitsGroundToUnWound, b->needsMoveUpdate, b->lastLastLocationUpdate);

    for (int i = 0; i < 4; i++) put(2, "%d,%d;", m->fielding_posts.baseman[i], m->fielding_posts.cover[i]);
    for (int i = 0; i < RANKED_FIELDERS_COUNT; i++) put(2, "%d,", m->catchingState.chase_ranking[i]);
    int jokers[JOKER_COUNT];
    batting_team_jokers(&r->scoreboard, jokers);
    for (int i = 0; i < JOKER_COUNT; i++) put(2, "j%d,", jokers[i]);
    put(2, "has=%d last=%d ctl=%d chg=%d", m->ballInfo.holder, m->ballInfo.released_by,
        m->catchingState.controlled_fielder, m->catchingState.ranking_cursor);

    put(3, "air=%d ", m->ballInfo.pitch_in_flight);
    for (int i = 0; i < BASE_COUNT; i++) put(3, "w%d,", m->runner_commitments.armed[i]);
    const AtBat* ab = &m->at_bat;
    // The batting cycle, printed as the three flags it used to be: arrived (setup pending), standing ready, open.
    put(3, "ib=%d br=%d bgo=%d bca=%d rcc=%d ips=%d", ab->cycle == BATTING_CYCLE_ARRIVED, batter_stands_ready(ab),
        batting_cycle_is_open(ab),
        ab->may_leave_home, m->chase_replan != CHASE_REPLAN_NONE, m->chase_replan == CHASE_REPLAN_AND_HAND_OVER);

    put(4, "pause=%d wbd=%d wfw=%d fwc=%d fwi=%d fwb=%d;", st->paused, m->offers.batter_seat,
        m->offers.free_walk, m->free_walk.worked_out, m->free_walk.runner, (int)m->free_walk.from);
    put(4, "gfs=%d,%d;", m->home_run_camera.hold, ball_is_home(m, st->fieldPositions));
    put(4, "cam=%d;", m->home_run_camera.active);
    V(4, m->catchingState.controlledMoveTarget);
    put(4, "%d,%d;", m->catchingState.controlledMoveTargetActive, m->catchingState.controlledMoveTargetFor);
    V(4, m->catchingState.ballTargetPoint);
    put(4, "cca=%d", catching_action_code(m));

    project_pitch(m);
    project_throw(m);

    const SwingActualization* s = &ab->swing;
    put(7, "cf=%d pa=%d p=%a va=%d v=%a c=%d;", s->contactFrame, s->powerActive, (double)s->power, s->verticalActive,
        (double)s->vertical, s->consumed);
    // The batter's body through a batting cycle. Printed only while a cycle is open: every field is set up
    // when one opens and read only inside it, so between cycles they are leftovers, not play.
    if (batting_cycle_is_open(ab)) {
        put(7, "bfc=%d ibfc=%d ang=%a as=%a adv=%a lim=%a stop=%d mov=%d upd=%d", ab->flight_frame,
            ab->flight_clock_running, (double)ab->arc_angle, (double)ab->step_in_speed, (double)ab->step_in,
            (double)ab->step_in_limit, ab->swing_stopped, ab->swing_started, ab->pose_dirty);
    }

    put(8, "%016llx", fnv(r, sizeof(*r), 1469598103934665603ULL));

    const ClientInputState* c = st->clientInput;
    for (int i = 0; i < BASE_COUNT; i++) put(9, "rp%d,", c->run_press_window[i]);
    project_widget("pw", &c->pitchWidget.meter);
    project_widget("tw", &c->throwWidget);
    put(9, "mv=%d,%d,%d,%d;", c->moveWidget.held, c->moveWidget.controlIndex, c->moveWidget.declared,
        c->moveWidget.framesSinceDeclared);
    V(9, c->moveWidget.point);
    put(9, "bs=%d,%d;", c->batterWidget.highlight, c->batterWidget.selected);
    project_widget("sw", &c->swingWidget.meter);
    put(9, "swf=%d,%d,%a;", c->swingWidget.flightFrames, (int)c->swingWidget.beat, (double)c->swingWidget.power);
    put(9, "aim=%a;", (double)c->batterAim);
    put(9, "cview=%a;", (double)catching_widget_view(c));
    SwingMeterView sv = swing_widget_view(&c->swingWidget);
    put(9, "sview=%d,%a,%d,%a", sv.cursorLive, (double)sv.cursor, sv.powerCommitted, (double)sv.power);

    const AIControllerState* ai = st->aiController;
    put(10, "seed=%u brt=%d;", ai->rngSeed, ai->batterReadyTimer);
    put(10, "sw=%d,%d,%a,%a,%a,%d;", ai->swing.decided, ai->swing.style, (double)ai->swing.power,
        (double)ai->swing.vertical, (double)ai->swing.angle, ai->swing.wrongPitch);
    V(10, ai->lastDeclaredMoveTarget);
    put(10, "%d,%d,%d", ai->hasDeclaredMoveTarget, ai->framesSinceMoveDeclared, ai->lastSteeredFielder);

    put(11, "ann=%d snd=%d stop=%d rng=%u", (int)st->announcement, st->playSoundEffect, st->stopSoundEffect,
        m->rngSeed);
}
