#include "rules_state.h"
#include "world.h"
#include "session.h"
#include "client_input.h"
#include "ai/ai_memory.h"
#include "home_run_view.h"
#include "player_view.h"
#include <stdint.h>
// The version-specific half of the golden projection: how this tree's structs answer the semantic
// questions golden.c asks. Edit THIS file when a refactor reshapes a struct, keeping every line it prints
// the same. Values a refactor legitimately leaves stale (a union half nobody is using, a scratch field the
// next action overwrites) are printed only while they mean something, so a trace records play and not
// leftovers.

static unsigned long long rules_projection_hash(const GameRulesState* r)
{
    unsigned long long h = 1469598103934665603ULL;
    // Four-byte values in declaration order preserve the old all-integer fingerprint,
    // without inheriting a future bool's width or a struct's padding.
#define RULE_VALUE(v) do { const uint32_t word = (v); h = fnv(&word, sizeof(word), h); } while (0)
    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        const RefereePlayerState* p = &r->referee.batting_players[i];
        RULE_VALUE(p->status);
        RULE_VALUE(p->base_at_pitch_start);
        RULE_VALUE(p->current_safety_base);
        RULE_VALUE(p->has_scored);
        RULE_VALUE(p->run_of_honor_scored);
        RULE_VALUE(p->has_pending_run);
        RULE_VALUE(p->has_pending_run_of_honor);
    }
    RULE_VALUE(r->referee.strikes_at_pitch_start);
    RULE_VALUE(r->referee.wounding_evaluation_active);
    RULE_VALUE(r->referee.wounding_evaluation_finished);
    RULE_VALUE(r->referee.wounding_evaluation_timer);
    RULE_VALUE(r->referee.foul_state);
    RULE_VALUE(r->referee.foul_timer);
    RULE_VALUE(r->referee.next_pair_state);
    RULE_VALUE(r->referee.next_pair_timer);
    RULE_VALUE(r->referee.end_of_inning_state);
    RULE_VALUE(r->referee.end_of_inning_timer);
    RULE_VALUE(r->referee.period_transition);
    RULE_VALUE(r->referee.period_transition_winner);
    RULE_VALUE(r->referee.ball_in_third_base_since_pitch);
    RULE_VALUE(r->referee.homerun_pair_has_pitch);
    RULE_VALUE(r->half_inning.outs);
    RULE_VALUE(r->half_inning.balls);
    RULE_VALUE(r->half_inning.strikes);
    RULE_VALUE(r->half_inning.batter_may_leave_home);
    RULE_VALUE(r->half_inning.pitch_rights);
    RULE_VALUE(r->half_inning.batter_in_turn);
    RULE_VALUE(r->half_inning.runs_in_inning);
    RULE_VALUE(r->half_inning.end_period);
    RULE_VALUE(r->half_inning.jokers_left);
    RULE_VALUE(r->half_inning.last_batter.designated_index);
    RULE_VALUE(r->half_inning.last_batter.last_regular_index);
    RULE_VALUE(r->half_inning.last_batter.has_batted_again);
    RULE_VALUE(r->half_inning.last_batter.regular_order_spent);
    RULE_VALUE(r->half_inning.last_batter.turn_exhausted);
    RULE_VALUE(r->between_pitch.catch_has_been_made);
    RULE_VALUE(r->between_pitch.has_ball_hit_ground);
    RULE_VALUE(r->between_pitch.pitch_result);
    RULE_VALUE(r->between_pitch.bat_outcome);
    RULE_VALUE(r->homerun_contest.pair_index);
    for (int i = 0; i < 2; i++) {
        const TeamInfo* t = &r->scoreboard.teams[i];
        RULE_VALUE(t->value);
        RULE_VALUE(t->control);
        RULE_VALUE(t->runs);
        RULE_VALUE(t->period0_runs);
        RULE_VALUE(t->period1_runs);
        RULE_VALUE(t->period2_runs);
        RULE_VALUE(t->period3_runs);
        for (size_t role = 0; role < sizeof(t->batter_runner_indices) / sizeof(t->batter_runner_indices[0]); role++)
            for (size_t pair = 0; pair < sizeof(t->batter_runner_indices[0]) / sizeof(t->batter_runner_indices[0][0]); pair++)
                RULE_VALUE(t->batter_runner_indices[role][pair]);
        for (size_t slot = 0; slot < sizeof(t->batter_order) / sizeof(t->batter_order[0]); slot++)
            RULE_VALUE(t->batter_order[slot]);
        RULE_VALUE(t->batter_order_index);
    }
    RULE_VALUE(r->scoreboard.half_innings_in_period);
    RULE_VALUE(r->scoreboard.inning);
    RULE_VALUE(r->scoreboard.period);
    RULE_VALUE(r->scoreboard.mode);
    RULE_VALUE(r->scoreboard.pair_count);
    RULE_VALUE(r->scoreboard.plays_first);
    RULE_VALUE(r->scoreboard.is_cup_game);
#undef RULE_VALUE
    return h;
}

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
    if (m->ball.holder == -1) put(6, " going=%d", m->ball.thrown_to);
}

static void project_widget(const char* name, const InputWidget* w)
{
    put(9, "%s=%d,%d,%d,%d;", name, w->counter, w->counter_max, w->dir, (int)w->mode);
}

static void project(const StateInfo* st, const HomeRunView* home_run, const PlayerView* players)
{
    const MatchSession* m = st->match;
    const GameRulesState* r = st->rules;

    for (int i = 0; i < 2 * PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        const PlayerInfo* p = &m->players[i];
        put(0, "[%d]", i);
        V(0, p->body.location);
        V(0, p->body.previous_location);
        V(0, p->body.target_location);
        V(0, p->body.velocity);
        V(0, p->body.orientation);
        put(0, "c%d,%d,%d,%d,%d,%d,%d,%d,%d;", p->side, p->motion.moving, p->motion.running, p->motion.following_target,
            p->motion.sync_previous_location, (int)players->players[i].model, players->players[i].frame_tick,
            players->players[i].frame_count, players->players[i].ticks_per_frame);
        // Keep the never-used position slot zero; recovery retains its historical active/not-active slot.
        put(0, "f%d,%d,%d,%d,%d,%d;", 0, p->fielding.near_home, (int)p->fielding.replacement,
            (int)p->fielding.replacement_base, p->fielding.busy_catching, p->fielding.throw_recovery_ticks != 0);
        put(0, "b%d,%d,%d,%d,%d,%d;", p->attributes.speed, p->attributes.power, p->batting.number, (int)p->batting.joker,
            (int)p->batting.presence.state, (int)p->batting.presence.base_id);
        const PlayerPathState* path = &p->path;
        // The removed write-only guard occupied the final historical zero slot.
        put(0, "r%d,%d,%d,%d;", path->arrived_at_base, path->waypoint, path->going_forward, 0);
    }

    const BallInfo* b = &m->ball;
    V(1, b->location);
    V(1, b->previous_location);
    V(1, b->velocity);
    put(1, "%d,%d,%d,%d,%d,%d,%d,%d", b->visible, b->moving, b->flight_has_hit_ground, b->on_ground,
        b->hit_ground_to_unwound, b->needs_move_update, b->sync_previous_location, b->catch_delay_ticks);

    for (int i = 0; i < 4; i++) put(2, "%d,%d;", m->fielding_posts.baseman[i], m->fielding_posts.cover[i]);
    for (int i = 0; i < RANKED_FIELDERS_COUNT; i++) put(2, "%d,", m->catching.chase_ranking[i]);
    int jokers[JOKER_COUNT];
    batting_team_jokers(&r->scoreboard, jokers);
    for (int i = 0; i < JOKER_COUNT; i++) put(2, "j%d,", jokers[i]);
    put(2, "has=%d last=%d ctl=%d chg=%d", m->ball.holder, m->ball.released_by,
        m->catching.controlled_fielder, m->catching.ranking_cursor);

    put(3, "air=%d ", m->ball.pitch_in_flight);
    for (int i = 0; i < BASE_COUNT; i++) put(3, "w%d,", m->runner_commitments.armed[i]);
    const AtBat* ab = &m->at_bat;
    // The batting cycle, printed as the three flags it used to be: arrived (setup pending), standing ready, open.
    put(3, "ib=%d br=%d bgo=%d bca=%d rcc=%d ips=%d", ab->cycle == BATTING_CYCLE_ARRIVED, batter_stands_ready(ab),
        batting_cycle_is_open(ab), r->half_inning.batter_may_leave_home, m->chase_replan != CHASE_REPLAN_NONE,
        m->chase_replan == CHASE_REPLAN_AND_HAND_OVER);

    put(4, "pause=%d wbd=%d wfw=%d fwi=%d fwb=%d;", st->paused, m->offers.batter_seat,
        m->offers.free_walk, m->free_walk.runner, (int)m->free_walk.from);
    put(4, "gfs=%d,%d;", home_run->hold_ticks, ball_is_home(m, st->field_positions));
    put(4, "cam=%d;", home_run->active);
    V(4, m->catching.controlled_move_target);
    put(4, "%d,%d;", m->catching.controlled_move_target_active, m->catching.controlled_move_target_for);
    V(4, m->catching.ball_target_point);
    put(4, "cca=%d", catching_action_code(m));

    project_pitch(m);
    project_throw(m);

    const SwingActualization* s = &ab->swing;
    put(7, "cf=%d pa=%d p=%a va=%d v=%a c=%d;", s->contact_frame, s->power_active, (double)s->power, s->vertical_active,
        (double)s->vertical, s->consumed);
    // The batter's body through a batting cycle. Printed only while a cycle is open: every field is set up
    // when one opens and read only inside it, so between cycles they are leftovers, not play.
    if (batting_cycle_is_open(ab)) {
        put(7, "bfc=%d ibfc=%d ang=%a as=%a adv=%a lim=%a stop=%d mov=%d upd=%d", ab->flight_frame,
            ab->flight_clock_running, (double)ab->arc_angle, (double)ab->step_in_speed, (double)ab->step_in,
            (double)ab->step_in_limit, ab->swing_stopped, ab->swing_started, ab->pose_dirty);
    }

    put(8, "%016llx", rules_projection_hash(r));

    const ClientInputState* c = st->client_input;
    for (int i = 0; i < BASE_COUNT; i++) put(9, "rp%d,", c->run_press_window[i]);
    project_widget("pw", &c->pitch_widget.meter);
    project_widget("tw", &c->throw_widget.meter);
    put(9, "twp=%d,%a;", c->throw_widget.power_committed, (double)c->throw_widget.power);
    put(9, "mv=%d,%d,%d,%d;", c->move_widget.held, c->move_widget.control_index, c->move_widget.declared,
        c->move_widget.frames_since_declared);
    V(9, c->move_widget.point);
    put(9, "bs=%d,%d;", c->batter_widget.highlight, c->batter_widget.selected);
    project_widget("sw", &c->swing_widget.meter);
    put(9, "swf=%d,%d,%a;", c->swing_widget.flight_frames, (int)c->swing_widget.beat, (double)c->swing_widget.power);
    put(9, "aim=%a;", (double)c->batter_aim);
    put(9, "cview=%a;", (double)catching_widget_view(c));
    SwingMeterView sv = swing_widget_view(&c->swing_widget);
    put(9, "sview=%d,%a,%d,%a", sv.cursor_live, (double)sv.cursor, sv.power_committed, (double)sv.power);

    const AIControllerState* ai = st->ai_controller;
    put(10, "seed=%u brt=%d;", ai->rng_seed, ai->batter_ready_timer);
    put(10, "sw=%d,%d,%a,%a,%a,%d;", ai->swing.decided, ai->swing.style, (double)ai->swing.power,
        (double)ai->swing.vertical, (double)ai->swing.angle, ai->swing.wrong_pitch);
    V(10, ai->last_declared_move_target);
    put(10, "%d,%d,%d", ai->has_declared_move_target, ai->frames_since_move_declared, ai->last_steered_fielder);

    put(11, "ann=%d snd=%d stop=%d rng=%u", (int)st->announcement, st->play_sound_effect, st->stop_sound_effect,
        m->rng_seed);
}
