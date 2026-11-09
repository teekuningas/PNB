#include "all_contracts.h"
#include "body_step.h"
#include "fielder_behaviours.h"
#include "game_reset.h"
#include "player_motion.h"
#include "rules_pure/player_utils.h"
#include "scenario_builder.h"
#include "test_helpers.h"

int test_player_facing_preserves_planar_displacement(void)
{
    PlayerBody body = {.location = {2.0f, 4.0f, 3.0f}, .orientation = {99.0f, 42.0f, -97.0f}};
    Vector3D ball_location = {5.0f, 17.0f, -1.0f};

    set_orientation(&body, &ball_location);
    ASSERT_FLOAT_EQ(3.0f, body.orientation.x, 1e-6f, "facing retains displacement, not a unit vector");
    ASSERT_FLOAT_EQ(-4.0f, body.orientation.z, 1e-6f, "facing follows the ball in the ground plane");
    ASSERT_FLOAT_EQ(42.0f, body.orientation.y, 1e-6f, "planar facing does not overwrite the vertical pose");

    ball_location = body.location;
    set_orientation(&body, &ball_location);
    ASSERT_FLOAT_EQ(0.0f, body.orientation.x, 1e-6f, "coincident positions need no invented direction");
    ASSERT_FLOAT_EQ(0.0f, body.orientation.z, 1e-6f, "coincident positions stay finite");
    ASSERT_FLOAT_EQ(42.0f, body.orientation.y, 1e-6f, "coincidence still preserves the vertical pose");
    return TEST_PASSED;
}

int test_player_stops_preserve_distinct_facing_and_sync(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* m = ctx->state->match;
    m->ballInfo.location = (Vector3D){5.0f, 17.0f, -1.0f};
    m->ballInfo.moving = 1;
    m->ballInfo.holder = -1;
    for (int i = 15; i <= 16; i++) {
        PlayerInfo* p = &m->playerInfo[i];
        p->body.location = (Vector3D){2.0f, 4.0f, 3.0f};
        p->body.previous_location = (Vector3D){-30.0f, 37.0f, -40.0f};
        p->body.orientation = (Vector3D){99.0f, 42.0f, -97.0f};
        p->body.target_location = (Vector3D){11.0f, 12.0f, 13.0f};
        p->body.velocity = (Vector3D){0.2f, 0.3f, 0.4f};
        p->motion.moving = 1;
        p->motion.running = 1;
        p->motion.following_target = 1;
    }

    stop_movement(m->playerInfo, 15);
    stop_target_looking_player(&m->playerInfo[16].motion);
    player_location_orientation_and_targets(m, ctx->state->fieldPositions);

    ASSERT_FLOAT_EQ(99.0f, m->playerInfo[15].body.orientation.x, 1e-6f, "a stopped runner keeps its facing lock");
    ASSERT_FLOAT_EQ(
        -97.0f, m->playerInfo[15].body.orientation.z, 1e-6f, "stopping movement does not stop running mode"
    );
    ASSERT_FLOAT_EQ(3.0f, m->playerInfo[16].body.orientation.x, 1e-6f, "stopping a target walk releases facing");
    ASSERT_FLOAT_EQ(-4.0f, m->playerInfo[16].body.orientation.z, 1e-6f, "the next body step can face the moving ball");
    for (int i = 15; i <= 16; i++) {
        PlayerInfo* p = &m->playerInfo[i];
        ASSERT_FLOAT_EQ(2.0f, p->body.location.x, 1e-6f, "a stopped body does not integrate its retained velocity");
        ASSERT_FLOAT_EQ(3.0f, p->body.location.z, 1e-6f, "neither stop drifts toward the old target");
        ASSERT_FLOAT_EQ(0.2f, p->body.velocity.x, 1e-6f, "stopping gates integration rather than erasing velocity");
        ASSERT_FLOAT_EQ(0.4f, p->body.velocity.z, 1e-6f, "both stops retain the old velocity");
        ASSERT_FLOAT_EQ(11.0f, p->body.target_location.x, 1e-6f, "neither stop rewrites the destination");
        ASSERT_FLOAT_EQ(13.0f, p->body.target_location.z, 1e-6f, "the old destination is ignored, not moved");
        ASSERT_FLOAT_EQ(
            p->body.location.x, p->body.previous_location.x, 1e-6f, "the next step catches interpolation up"
        );
        ASSERT_FLOAT_EQ(
            p->body.location.z, p->body.previous_location.z, 1e-6f, "both stops request previous-position sync"
        );
        ASSERT_FLOAT_EQ(37.0f, p->body.previous_location.y, 1e-6f, "the interpolation sync is planar");
        ASSERT_EQ(0, p->motion.sync_previous_location, "the body step consumes the sync request");
    }
    cleanup_scenario(ctx);
    return TEST_PASSED;
}

int test_player_reset_recipes_rebuild_home_geometry(void)
{
    for (int recipe = 0; recipe < 3; recipe++) {
        ScenarioContext* ctx = create_scenario();
        MatchSession* m = ctx->state->match;
        FieldPositions positions = *ctx->state->fieldPositions;
        positions.pitchPlate.x += 13.0f;
        positions.pitchPlate.z -= 17.0f;
        positions.middleLeftCatcher = (Vector3D){21.0f, 2.0f, -31.0f};
        const Vector3D posts[] = {
            positions.pitcher,
            positions.firstBase,
            positions.secondBase,
            positions.thirdBase,
            positions.bottomRightCatcher,
            positions.middleLeftCatcher,
            positions.middleRightCatcher,
            positions.backLeftCatcher,
            positions.backRightCatcher
        };
        for (int i = 0; i < PLAYERS_IN_TEAM * 2 + JOKER_COUNT; i++) {
            m->playerInfo[i].body.home_location = (Vector3D){1000.0f, 1000.0f, 1000.0f};
        }
        if (recipe == 0) {
            reset_for_new_half_inning(m, &positions, ctx->state->teamData, ctx->state->rules);
        } else if (recipe == 1) {
            reset_for_foul_play(m, &positions, ctx->state->rules);
        } else {
            Scoreboard* scoreboard = &ctx->state->rules->scoreboard;
            scoreboard->period = 4;
            scoreboard->pairCount = 1;
            int side = get_batting_team_index(scoreboard);
            scoreboard->teams[side].batterRunnerIndices[0][0] = 0;
            scoreboard->teams[side].batterRunnerIndices[1][0] = 1;
            ctx->state->rules->homeRunContestState.runnerBatterPairCounter = 0;
            reset_for_next_pair(m, &positions, scoreboard, &ctx->state->rules->homeRunContestState);
        }

        for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
            const Vector3D* home = &m->playerInfo[i].body.home_location;
            float dx = home->x - positions.pitchPlate.x;
            float dz = home->z - positions.pitchPlate.z;
            float radius = sqrtf(dx * dx + dz * dz);
            ASSERT(
                radius >= HOME_RADIUS - 1e-5f && radius <= HOME_RADIUS * 1.3f, "batters rest around the current plate"
            );
            ASSERT_FLOAT_EQ(BALL_HEIGHT_WITH_PLAYER, home->y, 1e-6f, "batting homes retain their physical height");
            for (int j = 0; j < i; j++) {
                const Vector3D* other = &m->playerInfo[j].body.home_location;
                ASSERT(
                    fabsf(home->x - other->x) + fabsf(home->z - other->z) > 0.1f, "batters have distinct home places"
                );
            }
        }
        for (int i = 12; i < PLAYERS_IN_TEAM * 2 + JOKER_COUNT; i++) {
            const Vector3D* home = &m->playerInfo[i].body.home_location;
            ASSERT_FLOAT_EQ(posts[i - 12].x, home->x, 1e-6f, "each fielder's home is its current formation post");
            ASSERT_FLOAT_EQ(posts[i - 12].y, home->y, 1e-6f, "formation height is not discarded");
            ASSERT_FLOAT_EQ(posts[i - 12].z, home->z, 1e-6f, "all reset recipes rebuild every fielding home");
        }
        cleanup_scenario(ctx);
    }
    return TEST_PASSED;
}

int test_idle_fielder_returns_to_its_home_geometry(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* m = ctx->state->match;
    PlayerInfo* p = &m->playerInfo[17];
    Vector3D home = p->body.home_location;
    p->body.location = (Vector3D){home.x - 3.0f, home.y, home.z - 4.0f};
    p->motion.moving = 0;
    p->cTPI.isNearHomeLocation = 0;
    p->cTPI.replacingStage = REPLACEMENT_IDLE;
    p->cTPI.busyCatching = 0;
    p->cTPI.throw_recovery_ticks = 0;
    m->catchingState.controlled_fielder = -1;

    move_idling_players_to_home_location(m);
    ASSERT_FLOAT_EQ(home.x, p->body.target_location.x, 1e-6f, "an idle fielder returns to its own formation post");
    ASSERT_FLOAT_EQ(home.z, p->body.target_location.z, 1e-6f, "the return uses home geometry, not the stale target");
    ASSERT_FLOAT_EQ(WALK_SPEED * 3.0f / 5.0f, p->body.velocity.x, 1e-6f, "the homeward walk has the right direction");
    ASSERT_FLOAT_EQ(WALK_SPEED * 4.0f / 5.0f, p->body.velocity.z, 1e-6f, "the homeward walk has walking speed");
    player_location_orientation_and_targets(m, ctx->state->fieldPositions);
    ASSERT(p->body.location.x > home.x - 3.0f && p->body.location.z > home.z - 4.0f, "the next body step walks home");
    cleanup_scenario(ctx);
    return TEST_PASSED;
}
