#ifndef PLAYER_VIEW_H
#define PLAYER_VIEW_H

#include "tick_outputs.h"
#include "world.h"

#define PITCH_DOWN_MAX 9
#define PITCH_UP_MAX 13
#define THROW_LOAD_FRAMES 11

typedef enum {
    PLAYER_ANIM_STAND_NO_BALL = 0,
    PLAYER_ANIM_STAND_WITH_BALL = 1,
    PLAYER_ANIM_WALK_NO_BALL = 2,
    PLAYER_ANIM_WALK_WITH_BALL = 3,
    PLAYER_ANIM_RUN_NO_BALL = 4,
    PLAYER_ANIM_RUN_WITH_BALL = 5,
    PLAYER_ANIM_PITCH_WINDUP = 6,
    PLAYER_ANIM_PITCH_THROW = 7,
    PLAYER_ANIM_THROW_WINDUP = 8,
    PLAYER_ANIM_THROW_RELEASE = 9,
    PLAYER_ANIM_STAND_BARE = 10,
    PLAYER_ANIM_WALK_BARE = 11,
    PLAYER_ANIM_RUN_BARE = 12,
    PLAYER_ANIM_BATTER_READY = 13,
    PLAYER_ANIM_BAT_SWING_1 = 14,
    PLAYER_ANIM_BAT_SWING_2 = 15,
    PLAYER_ANIM_BAT_SWING_3 = 16
} PlayerAnimationModel;

typedef struct _PlayerAnimation {
    PlayerAnimationModel model;
    int frame_tick;
    int frame_count;
    int ticks_per_frame;
} PlayerAnimation;

typedef struct _PlayerView {
    PlayerAnimation players[PLAYERS_IN_TEAM * 2 + JOKER_COUNT];
} PlayerView;

// Entry after physical setup; exact running clocks are reconstructed by replaying tick facts.
void player_view_init(PlayerView* view, const MatchSession* match);
void player_view_after_tick(PlayerView* view, const TickFacts* facts);

#endif
