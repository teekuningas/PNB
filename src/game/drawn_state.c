/*
    drawn_state.c — what the World still stores because it is DRAWN, part of SIMULATE.

    The players' meshes and animation stages, and the home-run camera cue. Both are display values the
    engine keeps in the blittable World rather than letting the client derive them from what is
    happening — a named, fenced debt, not a design. They leave together, with the living-game work after
    the merge: update_models still ends a throw's recoil, which game logic reads, and the cue is raised
    and dropped by things that happen inside the tick, invisible in the world the tick leaves behind.
*/

#include "drawn_state.h"
#include "base_control.h"

// so for animations we need to change mesh after certain intervals.
void update_models(MatchSession* match)
{
    int i;
    // every player has some animations.
    for (i = 0; i < PLAYERS_IN_TEAM * 2 + JOKER_COUNT; i++) {
        switch (match->playerInfo[i].cPI.model) {
        case PLAYER_ANIM_WALK_NO_BALL:
        case PLAYER_ANIM_WALK_WITH_BALL:
        case PLAYER_ANIM_RUN_NO_BALL:
        case PLAYER_ANIM_RUN_WITH_BALL:
            // so first animations for walking and running. they are loops, the stage will be in range of 0 to
            // count*freq. reason this seemingly complicated way of doing this is just to provide possibility to have
            // animations with same meshcounts to have animations of different speeds.
            match->playerInfo[i].cPI.animationStage =
                (match->playerInfo[i].cPI.animationStage + 1) %
                (match->playerInfo[i].cPI.animationFrequency * match->playerInfo[i].cPI.animationStageCount);
            break;
        case PLAYER_ANIM_PITCH_WINDUP:
        case PLAYER_ANIM_PITCH_THROW:
            // throwing animations are not loops and they must end when the last stage is encountered.
            // (PLAYER_ANIM_THROW_WINDUP is no longer advanced here — its gather arc is driven directly by
            // the engine windup clock, ThrowActualization.timer, on the render side; timing dictates
            // animation. This retires the last throw animationStage writer for the windup.)
            if (match->playerInfo[i].cPI.animationStage !=
                match->playerInfo[i].cPI.animationStageCount * match->playerInfo[i].cPI.animationFrequency - 1) {
                match->playerInfo[i].cPI.animationStage = (match->playerInfo[i].cPI.animationStage + 1);
            }
            break;
        case PLAYER_ANIM_THROW_RELEASE:
            // here we do the same thing but set recoil to 0 after animation so that player can start moving and stuff
            // again.
            if (match->playerInfo[i].cPI.animationStage !=
                match->playerInfo[i].cPI.animationStageCount * match->playerInfo[i].cPI.animationFrequency - 1) {
                match->playerInfo[i].cPI.animationStage = (match->playerInfo[i].cPI.animationStage + 1);
            } else {
                match->playerInfo[i].cTPI.throwRecoil = 0;
                match->playerInfo[i].cPI.model = PLAYER_ANIM_STAND_NO_BALL;
            }
            break;
        case PLAYER_ANIM_WALK_BARE:
        case PLAYER_ANIM_RUN_BARE:
            // running and walking animations for batting team
            match->playerInfo[i].cPI.animationStage =
                (match->playerInfo[i].cPI.animationStage + 1) %
                (match->playerInfo[i].cPI.animationFrequency * match->playerInfo[i].cPI.animationStageCount);
            break;
        case PLAYER_ANIM_BAT_SWING_1:
        case PLAYER_ANIM_BAT_SWING_2:
        case PLAYER_ANIM_BAT_SWING_3:
            // batting animations are not loops, so theyll end after finishing. swinging, bunting, and spreading hands.
            if (match->playerInfo[i].cPI.animationStage !=
                match->playerInfo[i].cPI.animationStageCount * match->playerInfo[i].cPI.animationFrequency - 1) {
                match->playerInfo[i].cPI.animationStage = (match->playerInfo[i].cPI.animationStage + 1);
            }
            break;
        case PLAYER_ANIM_STAND_NO_BALL:
        case PLAYER_ANIM_STAND_WITH_BALL:
        case PLAYER_ANIM_STAND_BARE:
        case PLAYER_ANIM_BATTER_READY:
        default:
            break;
        }
    }
}

void update_home_run_camera(MatchSession* match, const FieldPositions* field_positions, const RefereeState* referee)
{
    if (ball_is_home(match, field_positions)) {
        if (match->home_run_camera.hold == -1 && match->home_run_camera.active == 1 &&
            (get_base_controller(match, referee, BASE_THIRD) == -1 ||
             match->playerInfo[get_base_controller(match, referee, BASE_THIRD)].bTPI.state == PLAYER_STATE_ON_BASE)) {
            match->home_run_camera.hold = 0;
        }
    }
}
