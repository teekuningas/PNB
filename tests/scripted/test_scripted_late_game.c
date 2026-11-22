#include "test_helpers.h"
#include <stdio.h>
#include "fixtures.h"
#include "key_states.h"
#include "menu_types.h"
#include "rules_state.h"
#include "world.h"
#include "client_input.h"
#include "home_run_view.h"
#include "scripted_harness.h"
#include "all_scripted.h"
#include "actions/batting_system.h"
#include "actions_pure/swing_geometry.h"
#include "rules_pure/base_control.h"
#include "rules_pure/player_utils.h"

/*
 * Late-game modes through the real human input path: home-run contest and super inning.
 *
 * Every other test in tests/scripted/ plays normal mode (period 0). Here both sides are driven by
 * human pads through real KeyStates -> action_invocations -> ClientInputState -> IntentChannels,
 * exercising two paths previously reached only by AI-vs-AI and fuzzed golden runs:
 *
 *   1. Home-run-contest pair: no batter-selection offer ever opens; a human pitch, human swing, and
 *      human third-base run score a run, engage HomeRunView, return the ball home to end the pair,
 *      advance pair_index 0 -> 1, seat the next pair, and leave widgets clean for pair 1's pitch.
 *   2. Super-inning walk-off: real human keys play a scoreless top half (3 outs) into the bottom
 *      half, where the batting team scores the winning run with < 3 outs and immediately walks off
 *      into MENU_ENTRY_GAME_OVER with winner and period2_runs populated.
 */

#define TEAM0_PAD CONTROL_PLAYER_1
#define TEAM1_PAD CONTROL_PLAYER_2

static void press(ScriptedGame* g, int pad, int key)
{
    scripted_hold(g, pad, key);
    scripted_tick(g);
    scripted_release(g, pad, key);
}

static int tick_until_pitch_ready(ScriptedGame* g, int budget)
{
    for (int i = 0; i < budget; i++) {
        MatchSession* m = scripted_match(g);
        int p = m->fielding_posts.baseman[0];
        if (p != -1 && m->ball.holder == p && m->catching.controlled_fielder == p && batter_stands_ready(&m->at_bat) &&
            m->catching_action.kind == CATCHING_ACTION_NONE && !m->ball.pitch_in_flight) {
            return 1;
        }
        if (!scripted_tick(g)) return 0;
    }
    return 0;
}

static int tick_until_batter_ready(ScriptedGame* g, int bat_pad, int budget)
{
    if (!scripted_tick_until_batter_decision(g, budget)) return 0;
    scripted_tap(g, bat_pad, KEY_2);
    for (int i = 0; i < budget; i++) {
        if (batting_cycle_is_open(&scripted_match(g)->at_bat)) return 1;
        if (!scripted_tick(g)) return 0;
    }
    return 0;
}

/* Steer the controlled fielder to a loose ball with real arrow keys until somebody holds it. */
static int pick_up_loose_ball(ScriptedGame* g, int catch_pad, int budget)
{
    MatchSession* m = scripted_match(g);
    for (int i = 0; i < budget; i++) {
        if (m->ball.holder != -1) {
            scripted_release_all(g);
            return 1;
        }
        int cf = m->catching.controlled_fielder;
        if (cf != -1) {
            float dx = m->ball.location.x - m->players[cf].body.location.x;
            float dz = m->ball.location.z - m->players[cf].body.location.z;
            if (dx > 0.3f)
                scripted_hold(g, catch_pad, KEY_RIGHT);
            else
                scripted_release(g, catch_pad, KEY_RIGHT);
            if (dx < -0.3f)
                scripted_hold(g, catch_pad, KEY_LEFT);
            else
                scripted_release(g, catch_pad, KEY_LEFT);
            if (dz > 0.3f)
                scripted_hold(g, catch_pad, KEY_DOWN);
            else
                scripted_release(g, catch_pad, KEY_DOWN);
            if (dz < -0.3f)
                scripted_hold(g, catch_pad, KEY_UP);
            else
                scripted_release(g, catch_pad, KEY_UP);
        }
        if (!scripted_tick(g)) {
            scripted_release_all(g);
            return 0;
        }
    }
    scripted_release_all(g);
    return 0;
}

/* Charge and release a real human throw toward `base_key` (KEY_DOWN=home, KEY_LEFT=1st, ...). */
static int throw_ball_to_base_key(ScriptedGame* g, int catch_pad, int base_key, int charge_ticks)
{
    scripted_hold(g, catch_pad, KEY_2);
    scripted_hold(g, catch_pad, base_key);
    if (scripted_run(g, charge_ticks) < charge_ticks) {
        scripted_release_all(g);
        return 0;
    }
    scripted_release(g, catch_pad, KEY_2);
    scripted_release(g, catch_pad, base_key);
    return scripted_tick(g);
}

/* Steer the controlled fielder to the loose ball with real arrow keys, then throw home if a non-pitcher
 * holds it. Returns 1 once the pitcher holds the ball at home, 0 on budget exhaustion or screen exit. */
static int field_and_return_ball_home(ScriptedGame* g, int catch_pad, int budget)
{
    MatchSession* m = scripted_match(g);
    int pitcher = m->fielding_posts.baseman[0];
    if (pitcher == -1) return 0;

    int remaining = budget;
    while (remaining > 0) {
        if (m->ball.holder == pitcher && m->players[pitcher].fielding.near_home) {
            scripted_release_all(g);
            return 1;
        }
        if (m->ball.holder == -1) {
            long before = scripted_frame(g);
            if (!pick_up_loose_ball(g, catch_pad, remaining)) return 0;
            remaining -= (int)(scripted_frame(g) - before);
            continue;
        }
        if (m->ball.holder != pitcher) {
            if (!throw_ball_to_base_key(g, catch_pad, KEY_DOWN, 16)) return 0;
            remaining -= 17;
            // Wait until the ball arrives or drops loose again.
            for (int i = 0; i < 200 && remaining > 0; i++, remaining--) {
                if (m->ball.holder == pitcher && m->players[pitcher].fielding.near_home) return 1;
                if (m->ball.holder == -1 && m->catching_action.kind == CATCHING_ACTION_NONE && !m->ball.moving) break;
                if (!scripted_tick(g)) return 0;
            }
            continue;
        }
        if (!scripted_tick(g)) return 0;
        remaining--;
    }
    scripted_release_all(g);
    return 0;
}

/* Deliver one strike-zone pitch through the human pitching widget; optionally swing on the batting pad.
 * When `swing` is non-zero, `swing_power_ticks` picks the power level during the windup and elevation is
 * sampled at the sweet spot (SWING_VERTICAL_FOCAL). */
static int play_human_pitch(ScriptedGame* g, int bat_pad, int catch_pad, int swing, int swing_power_ticks)
{
    ClientInputState* ci = scripted_state(g)->client_input;
    MatchSession* m = scripted_match(g);

    press(g, catch_pad, KEY_2); // start pitcher power ping-pong
    if (!scripted_tick(g)) return 0;
    if (ci->pitch_widget.meter.dir == 0) return 0;
    scripted_run(g, 8);
    press(g, catch_pad, KEY_2); // commit pitch power -> windup begins, batter power sweep arms
    if (!scripted_tick(g)) return 0;
    if (m->catching_action.kind != CATCHING_ACTION_PITCHING) return 0;

    if (swing) {
        if (ci->swing_widget.meter.mode != WIDGET_PING_PONG) return 0;
        scripted_run(g, swing_power_ticks);
        press(g, bat_pad, KEY_2); // commit swing power during windup
        if (!scripted_tick(g)) return 0;
    }

    int guard = 0;
    while (ci->pitch_widget.meter.dir != 0 &&
           ci->pitch_widget.meter.counter * 100 > ci->pitch_widget.meter.counter_max * 35 && guard++ < 400) {
        if (!scripted_tick(g)) return 0;
    }
    if (ci->pitch_widget.meter.dir == 0) return 0;
    press(g, catch_pad, KEY_2); // commit strike-zone aim
    if (!scripted_tick(g)) return 0;

    int airborne = 0;
    for (int i = 0; i < 250 && !airborne; i++) {
        if (!scripted_tick(g)) return 0;
        airborne = m->ball.pitch_in_flight;
    }
    if (!airborne) return 0;

    if (swing) {
        if (!scripted_tick(g)) return 0;
        if (ci->swing_widget.meter.mode != WIDGET_DESCENT) return 0;
        guard = 0;
        while (ci->swing_widget.meter.dir != 0 && guard++ < 400) {
            float here = (float)ci->swing_widget.meter.counter / (float)ci->swing_widget.meter.counter_max;
            if (here <= SWING_VERTICAL_FOCAL) break;
            if (!scripted_tick(g)) return 0;
        }
        press(g, bat_pad, KEY_2); // commit sweet-spot elevation
        if (!scripted_tick(g)) return 0;
    }

    return 1;
}

typedef struct {
    HomeRunView home_run;
    int home_run_ever_active;
    int batter_seat_ever_offered;
    int saw_next_pair_announcement;
    int saw_pair_detected;
} ContestObserver;

static void contest_observer_hook(const SimGame* game, void* ctx)
{
    ContestObserver* o = (ContestObserver*)ctx;
    home_run_view_after_tick(&o->home_run, &game->state->tick_facts);
    if (o->home_run.active) o->home_run_ever_active = 1;
    if (game->state->match->offers.batter_seat != 0) o->batter_seat_ever_offered = 1;
    if (game->state->announcement == EVENT_NEXT_PAIR) o->saw_next_pair_announcement = 1;
    if (game->state->rules->referee.next_pair_state == HR_PAIR_STATE_DETECTED) o->saw_pair_detected = 1;
}

int test_scripted_homerun_contest_pair(void)
{
    GameSetup setup;
    fixture_create_homerun_contest(&setup, 0, 1, TEAM0_PAD, TEAM1_PAD);
    ScriptedGame* g = scripted_create_from_setup(&setup, 0x48520001u);
    ASSERT_NOT_NULL(g, "scripted_create_from_setup returned NULL");

    ContestObserver obs = {.home_run = {.hold_ticks = -1}};
    scripted_attach(g, contest_observer_hook, &obs);

    StateInfo* st = scripted_state(g);
    MatchSession* m = scripted_match(g);
    GameRulesState* r = st->rules;

    ASSERT_EQ(4, r->scoreboard.period, "home-run contest must boot at period 4");
    ASSERT_EQ(0, r->homerun_contest.pair_index, "contest must start on pair 0");
    ASSERT_EQ(5, r->scoreboard.pair_count, "fixture must configure 5 contest pairs");
    ASSERT_EQ(0, m->offers.batter_seat, "contest must not open a batter-selection offer");

    int pair0_batter = r->scoreboard.teams[0].batter_runner_indices[0][0];
    int pair0_runner = r->scoreboard.teams[0].batter_runner_indices[1][0];
    ASSERT_EQ(
        (int)PLAYER_STATE_AT_BAT, (int)m->players[pair0_batter].batting.presence.state,
        "pair 0 batter must start AT_BAT without a selection offer"
    );
    ASSERT_EQ(
        (int)BASE_THIRD, (int)m->players[pair0_runner].batting.presence.base_id,
        "pair 0 runner must start on third base"
    );

    ASSERT(tick_until_pitch_ready(g, 1200), "pair 0 never became pitch-ready");

    // Arm the third-base runner with a real human tap so he breaks for home on pitch release.
    scripted_tap(g, TEAM0_PAD, KEY_UP);
    ASSERT_EQ(1, m->runner_commitments.armed[BASE_THIRD], "KEY_UP tap must arm the third-base runner");

    // Real 4-beat human pitch and sweet-spot swing.
    ASSERT(play_human_pitch(g, TEAM0_PAD, TEAM1_PAD, 1, 20), "human pitch/swing dance failed in contest pair 0");

    // Wait for contact and for runner 0 to score a run from third.
    int scored = 0;
    for (int i = 0; i < 600 && !scored; i++) {
        ASSERT(scripted_tick(g), "contest ended unexpectedly while pair 0 was in play");
        if (r->scoreboard.teams[0].runs >= 1) scored = 1;
    }
    ASSERT(scored, "the third-base runner must score on the fair human hit");
    ASSERT_EQ((int)BAT_OUTCOME_HIT, (int)r->between_pitch.bat_outcome, "the human swing must have made contact");
    ASSERT_EQ((int)FOUL_STATE_NONE, (int)r->referee.foul_state, "the scoring drive must be fair");
    ASSERT_EQ(1, obs.home_run_ever_active, "HomeRunView must engage when the third-base runner breaks for home");

    // Field the ball and return it home so the referee ends pair 0 and transitions to pair 1.
    ASSERT(field_and_return_ball_home(g, TEAM1_PAD, 1500), "human catching side could not return the ball home");

    // Tick through the pair-end handshake (DETECTED -> 200-frame timer -> RESETTING -> NONE).
    int pair1_ready = 0;
    int pair1_batter = r->scoreboard.teams[0].batter_runner_indices[0][1];
    int pair1_runner = r->scoreboard.teams[0].batter_runner_indices[1][1];
    for (int i = 0; i < 800 && !pair1_ready; i++) {
        ASSERT(scripted_tick(g), "contest exited unexpectedly during pair 0 -> pair 1 reset");
        if (r->homerun_contest.pair_index == 1 && r->referee.next_pair_state == HR_PAIR_STATE_NONE &&
            m->players[pair1_batter].batting.presence.state == PLAYER_STATE_AT_BAT &&
            m->players[pair1_runner].batting.presence.base_id == BASE_THIRD) {
            pair1_ready = 1;
        }
    }

    ASSERT(pair1_ready, "contest must advance to pair 1 and seat the second pair");
    ASSERT_EQ(1, obs.saw_pair_detected, "referee must have entered HR_PAIR_STATE_DETECTED");
    ASSERT_EQ(1, obs.saw_next_pair_announcement, "referee must announce EVENT_NEXT_PAIR");
    ASSERT_EQ(0, obs.batter_seat_ever_offered, "contest must never open a batter-selection offer");
    ASSERT_EQ(1, r->scoreboard.teams[0].runs, "pair 0's run must persist after the pair reset");

    // Second-unit-of-play check: pair 1 reaches pitch-ready with clean widgets and begins a fresh pitch.
    ASSERT(tick_until_pitch_ready(g, 1200), "pair 1 never became pitch-ready after the pair reset");
    ASSERT_EQ(
        (int)WIDGET_IDLE, (int)st->client_input->pitch_widget.meter.mode,
        "pitch widget must be idle before pair 1's first pitch"
    );
    ASSERT_EQ(
        (int)WIDGET_IDLE, (int)st->client_input->swing_widget.meter.mode,
        "swing widget must be idle before pair 1's first pitch"
    );
    press(g, TEAM1_PAD, KEY_2);
    ASSERT(scripted_tick(g), "game must stay live on pair 1's first pitch arm");
    scripted_run(g, 6);
    press(g, TEAM1_PAD, KEY_2);
    ASSERT(scripted_tick(g), "game must stay live on pair 1's pitch commit");
    ASSERT_EQ(
        (int)CATCHING_ACTION_PITCHING, (int)m->catching_action.kind, "pair 1 must be able to begin a fresh human pitch"
    );

    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);

    ASSERT(!failed, reason);
    return TEST_PASSED;
}

typedef struct {
    int saw_end_period;
    int outs_when_end_period_set;
} WalkoffObserver;

static void walkoff_observer_hook(const SimGame* game, void* ctx)
{
    WalkoffObserver* o = (WalkoffObserver*)ctx;
    if (!o->saw_end_period && game->state->rules->half_inning.end_period == 1) {
        o->saw_end_period = 1;
        o->outs_when_end_period_set = game->state->rules->half_inning.outs;
    }
}

/* Strike out one human batter on three taken strike pitches through the real input path. */
static int strike_out_batter(ScriptedGame* g, int bat_pad, int catch_pad)
{
    GameRulesState* r = scripted_state(g)->rules;
    int start_outs = r->half_inning.outs;
    if (!tick_until_batter_ready(g, bat_pad, 1500)) return 0;

    for (int pitch = 0; pitch < 3; pitch++) {
        if (!field_and_return_ball_home(g, catch_pad, 1200)) return 0;
        if (!tick_until_pitch_ready(g, 1200)) return 0;
        int strikes_before = r->half_inning.strikes;
        if (!play_human_pitch(g, bat_pad, catch_pad, 0, 0)) return 0;

        int resolved = 0;
        for (int i = 0; i < 400 && !resolved; i++) {
            if (!scripted_tick(g)) return 0;
            if (r->half_inning.strikes > strikes_before || r->half_inning.outs > start_outs) resolved = 1;
        }
        if (!resolved) return 0;
        // On strike 3, the batter must advance (§18(1)): pick up the ball and throw to 1st base to burn him.
        if (pitch == 2 && r->half_inning.outs == start_outs) {
            if (!pick_up_loose_ball(g, catch_pad, 600)) return 0;
            if (!throw_ball_to_base_key(g, catch_pad, KEY_LEFT, 14)) return 0;
            for (int i = 0; i < 600 && r->half_inning.outs == start_outs; i++) {
                if (!scripted_tick(g)) return 0;
            }
        }
    }
    return r->half_inning.outs > start_outs;
}

int test_scripted_super_inning_walkoff(void)
{
    GameSetup setup;
    fixture_create_super_inning(&setup, 0, 1, TEAM0_PAD, TEAM1_PAD);
    ScriptedGame* g = scripted_create_from_setup(&setup, 0x50500001u);
    ASSERT_NOT_NULL(g, "scripted_create_from_setup returned NULL");

    WalkoffObserver obs = {0};
    scripted_attach(g, walkoff_observer_hook, &obs);

    StateInfo* st = scripted_state(g);
    MatchSession* m = scripted_match(g);
    GameRulesState* r = st->rules;

    ASSERT_EQ(2, r->scoreboard.period, "super inning must boot at period 2");
    ASSERT_EQ(8, r->scoreboard.inning, "super inning must start at inning 8 (top half)");
    ASSERT_EQ(0, get_batting_team_index(&r->scoreboard), "team 0 must bat in the top of the super inning");

    // Top half (inning 8): team 0 strikes out three times for a scoreless top half.
    for (int out = 0; out < 3; out++) {
        ASSERT(strike_out_batter(g, TEAM0_PAD, TEAM1_PAD), "failed to record out in top of super inning");
    }

    // Tick across the real side-change handshake into inning 9 (bottom of the super inning).
    int in_bottom = 0;
    for (int i = 0; i < 600 && !in_bottom; i++) {
        ASSERT(scripted_tick(g), "super inning exited unexpectedly at top->bottom side change");
        if (r->scoreboard.inning == 9 && r->referee.end_of_inning_state == END_INNING_STATE_NONE) in_bottom = 1;
    }
    ASSERT(in_bottom, "super inning must transition cleanly into inning 9 (bottom half)");
    ASSERT_EQ(1, get_batting_team_index(&r->scoreboard), "team 1 must bat in the bottom of the super inning");
    ASSERT_EQ(0, r->scoreboard.teams[0].runs, "top half must have ended scoreless");
    ASSERT_EQ(0, r->scoreboard.teams[1].runs, "bottom half must start tied 0-0");

    // Bottom half (inning 9): team 1 selects a batter, hits a clean drive into the field, and circles the
    // bases via real human base-running keys to score the walk-off run with zero outs.
    ASSERT(tick_until_batter_ready(g, TEAM1_PAD, 1500), "team 1 batter never stepped up in bottom of super inning");
    ASSERT(tick_until_pitch_ready(g, 1200), "team 0 pitcher never became pitch-ready in bottom of super inning");

    // Arm the batter before contact so he breaks for first as soon as the bat meets the ball.
    scripted_tap(g, TEAM1_PAD, KEY_DOWN);
    ASSERT(play_human_pitch(g, TEAM1_PAD, TEAM0_PAD, 1, 26), "human pitch/swing dance failed in bottom half");

    // Drive the runner forward from first -> second -> third (kunniajuoksu walk-off) via real double-taps.
    // The human catching pad deliberately holds off throwing so the clean drive carries all the way.
    int game_ended = 0;
    int pushed_first = 0;
    int pushed_second = 0;
    for (int i = 0; i < 2000; i++) {
        if (!scripted_tick(g)) {
            game_ended = (st->screen == SCREEN_MAIN_MENU);
            break;
        }
        // Once our batter settles on 1st or 2nd, double-tap the base key to keep advancing.
        if (!pushed_first && get_base_controller(m, &r->referee, BASE_FIRST) != -1) {
            scripted_tap(g, TEAM1_PAD, KEY_LEFT);
            scripted_tap(g, TEAM1_PAD, KEY_LEFT);
            pushed_first = 1;
        }
        if (!pushed_second && get_base_controller(m, &r->referee, BASE_SECOND) != -1) {
            scripted_tap(g, TEAM1_PAD, KEY_RIGHT);
            scripted_tap(g, TEAM1_PAD, KEY_RIGHT);
            pushed_second = 1;
        }
    }

    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    MenuMode end_menu = g->sim->menu.mode;
    int winner = st->game_conclusion->winner;
    int p2_t0 = st->game_conclusion->period2_runs[0];
    int p2_t1 = st->game_conclusion->period2_runs[1];
    scripted_destroy(g);

    ASSERT(!failed, reason);
    ASSERT(obs.saw_end_period, "walk-off run must raise half_inning.end_period");
    ASSERT(obs.outs_when_end_period_set < 3, "walk-off must end the super inning before three outs");
    ASSERT(game_ended, "walk-off must leave SCREEN_GAME cleanly");
    ASSERT_EQ((int)MENU_ENTRY_GAME_OVER, (int)end_menu, "super-inning walk-off must open MENU_ENTRY_GAME_OVER");
    ASSERT_EQ(1, winner, "team 1 must be recorded as the winner");
    ASSERT_EQ(0, p2_t0, "team 0 super-inning runs must be 0");
    ASSERT_EQ(1, p2_t1, "team 1 super-inning walk-off runs must be 1");
    return TEST_PASSED;
}
