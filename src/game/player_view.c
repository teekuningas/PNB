#include "player_view.h"

static void apply_activity(PlayerAnimation* animation, const PlayerTickFacts* facts, bool batting)
{
    switch (facts->activity.started) {
    case PLAYER_ACTIVITY_RESET:
        animation->frame_tick = 0;
        animation->frame_count = 0;
        animation->ticks_per_frame = 1;
        break;
    case PLAYER_ACTIVITY_WALKING_TO_TARGET:
        *animation = (PlayerAnimation){.frame_count = 16, .ticks_per_frame = 3};
        break;
    case PLAYER_ACTIVITY_RUNNING_ROUTE:
    case PLAYER_ACTIVITY_CONTROLLED_RUNNING:
        *animation = (PlayerAnimation){.frame_count = 20, .ticks_per_frame = 3};
        break;
    case PLAYER_ACTIVITY_PITCH_BEGUN:
        *animation = (PlayerAnimation){.frame_count = PITCH_DOWN_MAX, .ticks_per_frame = 3};
        break;
    case PLAYER_ACTIVITY_PITCH_RELEASED:
        *animation =
            (PlayerAnimation){.frame_tick = PITCH_UP_MAX * 3 - 1, .frame_count = PITCH_UP_MAX, .ticks_per_frame = 3};
        break;
    case PLAYER_ACTIVITY_THROW_BEGUN:
        *animation = (PlayerAnimation){.frame_count = THROW_LOAD_FRAMES, .ticks_per_frame = 3};
        break;
    case PLAYER_ACTIVITY_THROW_RELEASED:
        *animation = (PlayerAnimation){.frame_count = 21, .ticks_per_frame = 2};
        break;
    case PLAYER_ACTIVITY_SWINGING:
    case PLAYER_ACTIVITY_BUNTING:
    case PLAYER_ACTIVITY_WITHDRAWING:
        *animation = (PlayerAnimation){.frame_count = 34, .ticks_per_frame = 3};
        break;
    default:
        break;
    }

    switch (facts->activity.latest) {
    case PLAYER_ACTIVITY_RESET:
    case PLAYER_ACTIVITY_TARGET_REACHED:
        animation->model = batting ? PLAYER_ANIM_STAND_BARE : PLAYER_ANIM_STAND_NO_BALL;
        break;
    case PLAYER_ACTIVITY_MOVEMENT_CANCELLED:
    case PLAYER_ACTIVITY_BALL_DROPPED:
        animation->model = PLAYER_ANIM_STAND_NO_BALL;
        break;
    case PLAYER_ACTIVITY_WALKING_TO_TARGET:
        animation->model = batting ? PLAYER_ANIM_WALK_BARE : PLAYER_ANIM_WALK_NO_BALL;
        break;
    case PLAYER_ACTIVITY_RUNNING_ROUTE:
        animation->model = PLAYER_ANIM_RUN_BARE;
        break;
    case PLAYER_ACTIVITY_CONTROLLED_RUNNING:
        animation->model = facts->holds_ball ? PLAYER_ANIM_RUN_WITH_BALL : PLAYER_ANIM_RUN_NO_BALL;
        break;
    case PLAYER_ACTIVITY_CONTROLLED_ARRIVAL:
        animation->model = facts->holds_ball ? PLAYER_ANIM_STAND_WITH_BALL : PLAYER_ANIM_STAND_NO_BALL;
        break;
    case PLAYER_ACTIVITY_PITCH_BEGUN:
        animation->model = PLAYER_ANIM_PITCH_WINDUP;
        break;
    case PLAYER_ACTIVITY_PITCH_RELEASED:
        animation->model = PLAYER_ANIM_PITCH_THROW;
        break;
    case PLAYER_ACTIVITY_PITCH_FAKED:
    case PLAYER_ACTIVITY_BALL_CAUGHT:
        animation->model = PLAYER_ANIM_STAND_WITH_BALL;
        break;
    case PLAYER_ACTIVITY_THROW_BEGUN:
        animation->model = PLAYER_ANIM_THROW_WINDUP;
        break;
    case PLAYER_ACTIVITY_THROW_RELEASED:
        animation->model = PLAYER_ANIM_THROW_RELEASE;
        break;
    case PLAYER_ACTIVITY_BATTER_READY:
        animation->model = PLAYER_ANIM_BATTER_READY;
        break;
    case PLAYER_ACTIVITY_SWINGING:
        animation->model = PLAYER_ANIM_BAT_SWING_1;
        break;
    case PLAYER_ACTIVITY_BUNTING:
        animation->model = PLAYER_ANIM_BAT_SWING_2;
        break;
    case PLAYER_ACTIVITY_WITHDRAWING:
        animation->model = PLAYER_ANIM_BAT_SWING_3;
        break;
    case PLAYER_ACTIVITY_NONE:
        break;
    }
}

static void advance_animation(PlayerAnimation* animation, bool throw_recovering)
{
    switch (animation->model) {
    case PLAYER_ANIM_WALK_NO_BALL:
    case PLAYER_ANIM_WALK_WITH_BALL:
    case PLAYER_ANIM_RUN_NO_BALL:
    case PLAYER_ANIM_RUN_WITH_BALL:
    case PLAYER_ANIM_WALK_BARE:
    case PLAYER_ANIM_RUN_BARE:
        animation->frame_tick = (animation->frame_tick + 1) % (animation->frame_count * animation->ticks_per_frame);
        break;
    case PLAYER_ANIM_THROW_RELEASE:
        if (!throw_recovering) {
            animation->model = PLAYER_ANIM_STAND_NO_BALL;
            break;
        }
        // Fall through: a release holds its final mesh until physical recovery ends.
    case PLAYER_ANIM_PITCH_WINDUP:
    case PLAYER_ANIM_PITCH_THROW:
    case PLAYER_ANIM_BAT_SWING_1:
    case PLAYER_ANIM_BAT_SWING_2:
    case PLAYER_ANIM_BAT_SWING_3:
        if (animation->frame_tick != animation->frame_count * animation->ticks_per_frame - 1) {
            animation->frame_tick++;
        }
        break;
    default:
        break;
    }
}

void player_view_init(PlayerView* view, const MatchSession* match)
{
    for (int i = 0; i < PLAYERS_IN_TEAM * 2 + JOKER_COUNT; i++) {
        bool batting = i < PLAYERS_IN_TEAM + JOKER_COUNT;
        view->players[i] = (PlayerAnimation){.model = batting ? PLAYER_ANIM_STAND_BARE : PLAYER_ANIM_STAND_NO_BALL,
                                             .ticks_per_frame = 1};
        PlayerTickFacts facts = {.activity = match->playerInfo[i].events, .holds_ball = match->ballInfo.holder == i};
        apply_activity(&view->players[i], &facts, batting);
    }
}

void player_view_after_tick(PlayerView* view, const TickFacts* facts)
{
    for (int i = 0; i < PLAYERS_IN_TEAM * 2 + JOKER_COUNT; i++) {
        bool batting = i < PLAYERS_IN_TEAM + JOKER_COUNT;
        apply_activity(&view->players[i], &facts->simulated_players[i], batting);
        advance_animation(&view->players[i], facts->simulated_players[i].throw_recovering);
        apply_activity(&view->players[i], &facts->enforced_players[i], batting);
    }
}
