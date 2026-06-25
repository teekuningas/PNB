#include <string.h>
#include "globals.h"
#include "common_logic.h"
#include "vector_math.h"
#include "geometry.h"
#include "rng.h"
#include "base_logic.h"
#include "game_consolidation.h"
#include "game_manipulation.h"
#include "base_control.h"
#include "referee.h"
#include "rules_pure/player_utils.h"

// Wrapper functions for backward compatibility
// These now call the pure vector_math functions
int is_vector_small_enough_sphere(Vector3D* vector, float limit)
{
    return vec3_is_small_enough_sphere(vector, limit);
}

int is_vector_small_enough_circle_xzv(Vector3D* vector, float limit)
{
    return vec3_is_small_enough_circle_xz_v(vector, limit);
}

int is_vector_small_enough_circle_xz(float dx, float dz, float limit)
{
    return vec3_is_small_enough_circle_xz(dx, dz, limit);
}

void set_vector_xyz(Vector3D* vector, float x, float y, float z)
{
    vec3_set_xyz(vector, x, y, z);
}

void set_vector_v(Vector3D* vector1, Vector3D* vector2)
{
    vec3_set_from_vector(vector1, vector2);
}

void set_vector_xz(Vector3D* vector, float x, float z)
{
    vec3_set_xz(vector, x, z);
}

void add_to_vector_xz(Vector3D* vector, float x, float z)
{
    vec3_add_xz(vector, x, z);
}

void add_to_vector_v(Vector3D* vector1, Vector3D* vector2)
{
    vec3_add_vector(vector1, vector2);
}
/*
    Index is index of the player in the playerInfo-array.
    stop_movement stops arrow key initiated movement. Many situations
    where the change of controlled player will leave the previously controlled
    player moving so this is commonly used to stop these ones.
*/
void stop_movement(PlayerInfo* playerInfo, int index)
{
    int j;
    if (index != -1) {
        for (j = 0; j < DIRECTION_COUNT; j++) {
            playerInfo[index].cTPI.movesToDirection[j] = 0;
        }
        // and after stopping movement, also ensure that no animation stays.
        if (playerInfo[index].cTPI.throwRecoil == 0) {
            playerInfo[index].cPI.model = PLAYER_ANIM_STAND_NO_BALL;
        }
        playerInfo[index].cPI.looksForTarget = 0;
        playerInfo[index].cPI.moving = 0;

        playerInfo[index].cPI.lastLastLocationUpdate = 1;
    }
}
// sometimes for example after a catch, we stop the the player, so that it wouldnt continue
// on its own. but it should still continue, as if player has the key presse down all the time.
// then we call this to start the movement again if there has been no release of the key inbetween
void smooth_out_movement(MatchSession* match)
{
    int j;
    for (j = 0; j < DIRECTION_COUNT; j++) {
        if (match->aF.cTAF.move[j] == 2) {
            match->aF.cTAF.move[j] = 1;
        }
    }
}
// this is for batting team players
void stop_target_looking_player(PlayerInfo* playerInfo, PlayerRuntimeState* playerRuntime, int index)
{
    playerInfo[index].cPI.moving = 0;
    playerInfo[index].cPI.running = 0;
    playerInfo[index].cPI.looksForTarget = 0;
    playerInfo[index].cPI.lastLastLocationUpdate = 1;
}

void set_orientation(PlayerInfo* playerInfo, BallInfo* ballInfo, int i)
{
    // simply set player to orient towards the ball
    if (i != -1) {
        float dx = ballInfo->location.x - playerInfo[i].tPI.location.x;
        float dz = ballInfo->location.z - playerInfo[i].tPI.location.z;
        playerInfo[i].tPI.orientation.x = dx;
        playerInfo[i].tPI.orientation.z = dz;
    }
}

void run_to_target(PlayerInfo* playerInfo, int index, Vector3D* target)
{
    if (index != -1) {
        float dx;
        float dz;
        float speed;
        float norm;
        // so set target location
        playerInfo[index].tPI.targetLocation.x = target->x;
        playerInfo[index].tPI.targetLocation.z = target->z;
        // looking for target yeah
        playerInfo[index].cPI.looksForTarget = 1;
        // find the direction
        dx = playerInfo[index].tPI.targetLocation.x - playerInfo[index].tPI.location.x;
        dz = playerInfo[index].tPI.targetLocation.z - playerInfo[index].tPI.location.z;

        norm = geometry_distance_2d_xz(&playerInfo[index].tPI.targetLocation, &playerInfo[index].tPI.location);

        if (norm < EPSILON) norm = 1.0f;
        // set the velocity

        speed = BATTING_TEAM_RUN_FACTOR * RUN_SPEED + (RUN_SPEED / 16) * playerInfo[index].bTPI.speed;
        set_vector_xz(&playerInfo[index].tPI.velocity, dx * speed / norm, dz * speed / norm);
        // we are running now, ( so for example our orientation wont change now unless we stop running)
        playerInfo[index].cPI.running = 1;
        // we are moving too
        playerInfo[index].cPI.moving = 1;
        // orientation to our direction
        playerInfo[index].tPI.orientation.x = dx;
        playerInfo[index].tPI.orientation.z = dz;
        // and set the running animation
        playerInfo[index].cPI.model = PLAYER_ANIM_RUN_BARE;
        playerInfo[index].cPI.animationStage = 0;
        playerInfo[index].cPI.animationStageCount = 20;
        playerInfo[index].cPI.animationFrequency = 3;
    }
}
/*
    this function puts player with index in the argument moving to some specified
    target by walking. is used for both fielders and batting team.
*/
void move_to_target(PlayerInfo* playerInfo, int index, Vector3D* target)
{
    if (index != -1) {
        // cant start this if throw is going on. when ball is thrown the
        // control will often change automatically and we dont want the player to
        // start moving with walking animation before its throw animation has finished.
        if (playerInfo[index].cTPI.throwRecoil == 0) {
            float dx;
            float dz;
            float norm;
            playerInfo[index].tPI.targetLocation.x = target->x;
            playerInfo[index].tPI.targetLocation.z = target->z;
            // looksForTarget is important flag to avoid unnecessary
            // overhead of checking whether the player has
            // arrived to target location.
            playerInfo[index].cPI.looksForTarget = 1;
            // first find the unit vector for direction and then set player's
            // velocity to be the direction vector times the walk_speed.
            dx = playerInfo[index].tPI.targetLocation.x - playerInfo[index].tPI.location.x;
            dz = playerInfo[index].tPI.targetLocation.z - playerInfo[index].tPI.location.z;

            norm = geometry_distance_2d_xz(&playerInfo[index].tPI.targetLocation, &playerInfo[index].tPI.location);

            if (norm < EPSILON) norm = 1.0f;
            set_vector_xz(&playerInfo[index].tPI.velocity, dx * WALK_SPEED / norm, dz * WALK_SPEED / norm);
            // if the player for some reason was running before this, set that to 0.
            // could happen for example if baserunner gets out.
            playerInfo[index].cPI.running = 0;
            // and set moving to 1 so that player's location will be updated.
            playerInfo[index].cPI.moving = 1;

            // choose different walking animation for fielders and batting team.
            if (index < PLAYERS_IN_TEAM + JOKER_COUNT) {
                playerInfo[index].cPI.model = PLAYER_ANIM_WALK_BARE;
            } else {
                playerInfo[index].cPI.model = PLAYER_ANIM_WALK_NO_BALL;
            }
            playerInfo[index].cPI.animationStage = 0;
            playerInfo[index].cPI.animationStageCount = 16;
            playerInfo[index].cPI.animationFrequency = 3;
        }
    }
}
// so this function is called when outs happen but also when wounds happen. it just moves
// players out of the field and then to homebase.
void move_player_out(
    PlayerInfo* playerInfo, PlayerRuntimeState* playerRuntime, const FieldPositions* field_positions, int index
)
{
    Vector3D target;
    // we are walking
    playerInfo[index].cPI.running = 0;
    // left or right?
    if (playerInfo[index].tPI.location.x < 0) {
        target.x = field_positions->leftPoint.x - 5.0f;
        target.z = field_positions->leftPoint.z + 10.0f;
    } else {
        target.x = field_positions->rightPoint.x + 5.0f;
        target.z = field_positions->rightPoint.z + 10.0f;
    }
    // path point not passed yet.
    playerRuntime[index].passedPathPoint = 0;

    // and move to target takes care of the rest.
    move_to_target(playerInfo, index, &target);
}
// so we have the ranked fielders-array and those are players who are somewhat important in relation
// to ball's current location and velocity. so its natural that we have those players moving to catch
// the ball.
void move_ranked_to_catch(MatchSession* match)
{
    int i;

    for (i = 0; i < RANKED_FIELDERS_COUNT; i++) {
        int index = match->pII.fielderRankedIndices[i];
        // controlled player wont get the chance.
        if (index != match->pII.controlIndex && match->playerInfo[index].cTPI.replacingStage == REPLACEMENT_IDLE) {
            // if we are throwing ( towards a base ) we dont want the baseman there to start moving
            // as it would be nice that he is at the base when ball is caught if baserunner is going there.
            if (match->pRAI.throw_going_to_base == -1 ||
                (match->pII.catcherOnBaseIndex[match->pRAI.throw_going_to_base] != index)) {
                int k;
                int done = 0;
                // and we have special condition not to move any basemen
                // automatically at all.
                for (k = 0; k < BASE_COUNT; k++) {
                    if (match->pII.catcherOnBaseIndex[k] == index) {
                        done = 1;
                    }
                }
                if (done == 0) {
                    // set busycatching flag, and move player towards the target point
                    // that has been specified beforehand.
                    match->playerInfo[match->pII.fielderRankedIndices[i]].cTPI.busyCatching = 1;
                    move_to_target(match->playerInfo, index, &match->cameraState.targetPoint);
                }
            }
        }
    }
}

void run_to_next_base(MatchSession* match, const FieldPositions* field_positions, int index, BaseID base)
{
    if (index != -1) {
        Vector3D target;
        // first we select the target corresponding to base argument
        if (base == BASE_HOME) {
            if (match->pRAI.batter_can_advance == 0) return;
            target.x = field_positions->firstBaseRun.x;
            target.z = field_positions->firstBaseRun.z;
            // here as it is the batter, we'll also stop any batting to be able to run freely.
            match->pRAI.batter_ready = 0;
            match->pRAI.batting_going_on = 0;
        } else if (base == BASE_FIRST) {
            target.x = field_positions->secondBaseRun.x;
            target.z = field_positions->secondBaseRun.z;
        } else if (base == BASE_SECOND) {
            target.x = field_positions->thirdBaseRun.x;
            target.z = field_positions->thirdBaseRun.z;
        } else if (base == BASE_THIRD) {
            // if we are running home, there is the "flag" point, and we must change the direction there.
            // how it matters here is that if we have already passed the flag, we must run towards homebase,
            // if not, we must run towards flag.
            if (match->playerRuntime[index].passedPathPoint == 0) {
                target.x = field_positions->runLeftPoint.x;
                target.z = field_positions->runLeftPoint.z;

                match->cameraState.homeRunCameraFlag = 1;
            } else if (match->playerRuntime[index].passedPathPoint == 1) {
                target.x = field_positions->homeRunPoint.x;
                target.z = field_positions->homeRunPoint.z;
            } else {
                return;
            }

        } else {
            return;
        }
        // and set it so that next player has to have a will of his own to run
        match->pRAI.will_start_running[base] = 0;
        // set state to running, BUT only if we aren't already WOUNDED, OUT, or ADVANCING_FREELY
        // (which are terminal/override states that must not be downgraded)
        if (match->playerInfo[index].bTPI.state != PLAYER_STATE_WOUNDED &&
            match->playerInfo[index].bTPI.state != PLAYER_STATE_OUT &&
            match->playerInfo[index].bTPI.state != PLAYER_STATE_ADVANCING_FREELY) {
            match->playerInfo[index].bTPI.state = PLAYER_STATE_RUNNING;
        }
        // and we are moving forward
        match->playerRuntime[index].goingForward = 1;
        // and run_to_target can continue the job with index and the already set target.
        run_to_target(match->playerInfo, index, &target);
    }
}

void run_to_previous_base(MatchSession* match, const FieldPositions* field_positions, int index, BaseID base)
{
    if (index != -1) {
        Vector3D target;
        // run to previous base works similarly to run to next base.
        // starting point here is that we arent on any base, and the base variable is telling us
        // the previous base
        if (base == BASE_HOME) {
            // so when batter returns, he will go to his ready position again.
            target.x = (float)(field_positions->pitchPlate.x + cos(ZERO_BATTING_ANGLE) * BATTING_RADIUS);
            target.z = (float)(field_positions->pitchPlate.z - sin(ZERO_BATTING_ANGLE) * BATTING_RADIUS);
        } else if (base == BASE_FIRST) {
            target.x = field_positions->firstBaseRun.x;
            target.z = field_positions->firstBaseRun.z;
        } else if (base == BASE_SECOND) {
            target.x = field_positions->secondBaseRun.x;
            target.z = field_positions->secondBaseRun.z;
        } else if (base == BASE_THIRD) {
            // here we again select the target by our current location relative to flag
            if (match->playerRuntime[index].passedPathPoint == 0) {
                target.x = field_positions->thirdBaseRun.x;
                target.z = field_positions->thirdBaseRun.z;
            } else if (match->playerRuntime[index].passedPathPoint == 1) {
                target.x = field_positions->runLeftPoint.x;
                target.z = field_positions->runLeftPoint.z;
            } else {
                return;
            }

        } else {
            return;
        }

        // and set it so that next player has to have a will of his own to run
        match->pRAI.will_start_running[base] = 0;
        // we arent going forward
        match->playerRuntime[index].goingForward = 0;
        // set state to running, BUT only if we aren't already WOUNDED or OUT
        if (match->playerInfo[index].bTPI.state != PLAYER_STATE_WOUNDED &&
            match->playerInfo[index].bTPI.state != PLAYER_STATE_OUT) {
            match->playerInfo[index].bTPI.state = PLAYER_STATE_RUNNING;
        }
        // and run_to_target can handle the rest

        run_to_target(match->playerInfo, index, &target);
    }
}

void lead_from_base(
    PlayerInfo* playerInfo, PlayerRuntimeState* playerRuntime, const FieldPositions* field_positions, int index
)
{
    if (index != -1) {
        int done = 0;
        Vector3D target;
        // now to lead_from_base we must be either on first base or second base, as it doesnt make much sense in
        // third base nor in homebase.
        if (playerInfo[index].bTPI.baseId == BASE_FIRST) {
            // lead_from_base target is selected by adding a small step to current location to next bases' direction
            // using firstBase  instead of location in the difference is because we want the step size to stay
            // same
            target.x = playerInfo[index].tPI.location.x +
                       LEAD_STEP * (field_positions->secondBaseRun.x - field_positions->firstBaseRun.x);
            target.z = playerInfo[index].tPI.location.z +
                       LEAD_STEP * (field_positions->secondBaseRun.z - field_positions->firstBaseRun.z);
            // if we go over half way, we disallow any leading, you should just run from there.
            if (playerInfo[index].tPI.location.x >
                field_positions->firstBaseRun.x +
                    0.5f * (field_positions->secondBaseRun.x - field_positions->firstBaseRun.x))
                done = 1;
        } else if (playerInfo[index].bTPI.baseId == BASE_SECOND) {
            // same as in previous but from second to third base
            target.x = playerInfo[index].tPI.location.x +
                       LEAD_STEP * (field_positions->thirdBaseRun.x - field_positions->secondBaseRun.x);
            target.z = playerInfo[index].tPI.location.z +
                       LEAD_STEP * (field_positions->thirdBaseRun.z - field_positions->secondBaseRun.z);
            if (playerInfo[index].tPI.location.x <
                field_positions->secondBaseRun.x +
                    0.5f * (field_positions->thirdBaseRun.x - field_positions->secondBaseRun.x))
                done = 1;
        } else {
            done = 1;
        }
        // if our
        if (done == 0) {
            // walk to our target
            move_to_target(playerInfo, index, &target);
            // now we in fact are leading
            playerInfo[index].bTPI.state = PLAYER_STATE_LEADING;
            // but we dont set going forward flag.
            playerRuntime[index].goingForward = 0;
        }
    }
}

void change_player(MatchSession* match)
{
    // this is called by user explicitly and sometimes after updating change_player lists.
    // so cant change pitch if pitch is going on
    if (match->pRAI.pitch_state == PITCH_STAGE_NONE) {
        // player will start randomly floating after control changes to next player.
        if (match->pII.controlIndex != -1) {
            stop_movement(match->playerInfo, match->pII.controlIndex);
        }

        if (match->pII.fielderRankedIndices[match->pII.changePlayerArrayIndex] !=
            -1) { // set control to new index from the array
            match->pII.controlIndex = match->pII.fielderRankedIndices[match->pII.changePlayerArrayIndex];
            // and set him to run
            match->playerInfo[match->pII.controlIndex].cPI.running = 1;
            // logic about coming back from replacement if controlled
            if (match->playerInfo[match->pII.controlIndex].cTPI.replacingStage == REPLACEMENT_ACTIVE) {
                match->playerInfo[match->pII.controlIndex].cTPI.replacingStage = REPLACEMENT_IDLE;
            }
            // same for busyCatching.
            if (match->playerInfo[match->pII.controlIndex].cTPI.busyCatching == 1) {
                match->playerInfo[match->pII.controlIndex].cTPI.busyCatching = 0;
            }
            // move others to catch( so that the previous one for example doesnt just stop if its near the ball )
            move_ranked_to_catch(match);
            // stop player who has the selection now
            stop_movement(match->playerInfo, match->pII.controlIndex);
            // but start moving again if movement key being held at the same time. for smooth movement.
            // smooth_out_movement still needs StateInfo due to ActionFlags being in Local but also needing KeyStates?
            // Wait, smooth_out_movement implementation:
            /*
            void smooth_out_movement(StateInfo* stateInfo)
            {
                int j;
                for(j = 0; j < DIRECTION_COUNT; j++) {
                    if(stateInfo->match->aF.cTAF.move[j] == 2) {
                        stateInfo->match->aF.cTAF.move[j] = 1;
                    }
                }
            }
            */
            // It only uses MatchSession! I'll narrow it later.
        }
    }
}

void prepare_batter(MatchSession* match)
{
    int batterIndex = get_active_batter_index(match);
    if (batterIndex != -1) {
        // batter ready model
        match->playerInfo[batterIndex].cPI.model = PLAYER_ANIM_BATTER_READY;
        // can pitch now
        match->pRAI.batter_ready = 1;
        // waiting for pitch to go in air before starting the batting movement
        match->aF.bTAF.swing = 0;
        // batterIndex has been selected before calling this function

        // and initialize batter so that everything is ready to go.
        match->pRAI.init_batter = 1;
    }
}
// so here we calculate index and base of the player who is the leadrunner so that
// we can move him if thats the decision.
void calculate_free_walk(MatchSession* match, const RefereeState* referee)
{
    int i;
    BaseID maxBaseAtPitchStart = BASE_NONE;
    BaseID maxBaseId = BASE_NONE;
    int maxIndex = -1;
    // we go throush every (nonwounded) candidate and check who has the biggest base value
    // if there are many of those who have same base value, we will pick the one who has
    // the biggest baseAtPitchStart value. if both are same for some reason
    // then the selection will be quite random but shouldn't happen often and shouldn't be a big deal
    // either.
    for (i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        int index = i;
        if (match->playerInfo[index].bTPI.baseId != BASE_NONE &&
            match->playerInfo[index].bTPI.state != PLAYER_STATE_WOUNDED) {
            BaseID currentBaseId = match->playerInfo[index].bTPI.baseId;

            // SANITY CHECK: If player is at HOME base, they MUST be the active batter.
            int batterIndex = get_active_batter_index(match);
            if (currentBaseId == BASE_HOME && index != batterIndex) {
                printf(
                    "[CRITICAL LOGIC ERROR] Ghost Runner detected! Player %d is at BASE_HOME but batterIndex is %d. "
                    "Ignoring.\n",
                    index, batterIndex
                );
                continue;
            }

            // Use base_cmp to compare bases semantically (replaces >=)
            if (base_cmp(currentBaseId, maxBaseId) >= 0) {
                if (currentBaseId == maxBaseId) {
                    if (base_cmp(referee->battingPlayers[index].baseAtPitchStart, maxBaseAtPitchStart) > 0) {
                        maxBaseId = currentBaseId;
                        maxBaseAtPitchStart = referee->battingPlayers[index].baseAtPitchStart;
                        maxIndex = index;
                    }
                } else {
                    maxBaseId = currentBaseId;
                    maxBaseAtPitchStart = referee->battingPlayers[index].baseAtPitchStart;
                    maxIndex = index;
                }
            }
        }
    }
    match->flowControl.freeWalkIndex = maxIndex;
    if (maxIndex != -1)
        match->flowControl.freeWalkBase = match->playerInfo[maxIndex].bTPI.baseId;
    else
        match->flowControl.freeWalkBase = BASE_NONE;
}
/*
    Here we initialize all the locations and velocities so that players will be in their correct
    positions and orientations on the field and ball looks like its thrown to pitcher. Models
    are updated also. Before calling this the fielding team must have its position-attributes filled.
*/
void initialize_spatial_player_information(
    MatchSession* match, const FieldPositions* field_positions, unsigned int* rng_seed
)
{
    int i;
    const Vector3D* fieldPosition;
    int battingTeamPlacement[PLAYERS_IN_TEAM + JOKER_COUNT];
    // create array that has 0-11 in random order to give batting players in home
    // circle a nicer visual feelin'
    for (i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        battingTeamPlacement[i] = -1;
    }
    i = 0;
    while (i < PLAYERS_IN_TEAM + JOKER_COUNT) {
        int random = seeded_rand(rng_seed, PLAYERS_IN_TEAM + JOKER_COUNT);
        if (battingTeamPlacement[random] == -1) {
            battingTeamPlacement[random] = i;
            i++;
        }
    }
    // when out of bounds situation or inning ends, we swing the ball in the air and let the pitcher
    // catch it
    match->ballInfo.velocity.x = BALL_INIT_SPEED_X;
    match->ballInfo.velocity.y = BALL_INIT_SPEED_Y;
    match->ballInfo.velocity.z = BALL_INIT_SPEED_Z;
    match->ballInfo.location.x = BALL_INIT_LOCATION_X;
    match->ballInfo.location.y = BALL_INIT_LOCATION_Y;
    match->ballInfo.location.z = BALL_INIT_LOCATION_Z;

    match->ballInfo.lastLocation.x = match->ballInfo.location.x;
    match->ballInfo.lastLocation.y = match->ballInfo.location.y;
    match->ballInfo.lastLocation.z = match->ballInfo.location.z;
    // set locations and models for batting team players
    for (i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        float radiusFix = (float)(1.0f + fabs(5.5f - battingTeamPlacement[i]) / 20.0f);
        match->playerInfo[i].cPI.model = PLAYER_ANIM_STAND_BARE;

        match->playerInfo[i].tPI.homeLocation.x =
            (float)(field_positions->pitchPlate.x +
                    (HOME_RADIUS)*radiusFix *
                        cos(PI - (battingTeamPlacement[i] + 1) * PI / (PLAYERS_IN_TEAM + JOKER_COUNT + 1)));
        match->playerInfo[i].tPI.homeLocation.y = BALL_HEIGHT_WITH_PLAYER;
        match->playerInfo[i].tPI.homeLocation.z =
            (float)(field_positions->pitchPlate.z +
                    (HOME_RADIUS)*radiusFix *
                        sin(PI - (battingTeamPlacement[i] + 1) * PI / (PLAYERS_IN_TEAM + JOKER_COUNT + 1)));
        match->playerInfo[i].tPI.location.x = match->playerInfo[i].tPI.homeLocation.x;
        match->playerInfo[i].tPI.location.y = match->playerInfo[i].tPI.homeLocation.y;
        match->playerInfo[i].tPI.location.z = match->playerInfo[i].tPI.homeLocation.z;
        match->playerInfo[i].tPI.orientation.x = match->ballInfo.location.x - match->playerInfo[i].tPI.location.x;
        match->playerInfo[i].tPI.orientation.y = match->ballInfo.location.y - match->playerInfo[i].tPI.location.y;
        match->playerInfo[i].tPI.orientation.z = match->ballInfo.location.z - match->playerInfo[i].tPI.location.z;
        match->playerInfo[i].tPI.lastLocation.x = match->playerInfo[i].tPI.location.x;
        match->playerInfo[i].tPI.lastLocation.y = match->playerInfo[i].tPI.location.y;
        match->playerInfo[i].tPI.lastLocation.z = match->playerInfo[i].tPI.location.z;
    }
    // set locations and models for fielders. here we need positions set.
    for (i = 12; i < 2 * PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        match->playerInfo[i].cPI.model = PLAYER_ANIM_STAND_NO_BALL;

        switch (i - 12) {
        case 0:
            fieldPosition = &(field_positions->pitcher);
            break;
        case 1:
            fieldPosition = &(field_positions->firstBase);
            break;
        case 2:
            fieldPosition = &(field_positions->secondBase);
            break;
        case 3:
            fieldPosition = &(field_positions->thirdBase);
            break;
        case 4:
            fieldPosition = &(field_positions->bottomRightCatcher);
            break;
        case 5:
            fieldPosition = &(field_positions->middleLeftCatcher);
            break;
        case 6:
            fieldPosition = &(field_positions->middleRightCatcher);
            break;
        case 7:
            fieldPosition = &(field_positions->backLeftCatcher);
            break;
        case 8:
            fieldPosition = &(field_positions->backRightCatcher);
            break;
        default:
            fieldPosition = &(field_positions->pitchPlate);
            break;
        }
        match->playerInfo[i].tPI.homeLocation.x = fieldPosition->x;
        match->playerInfo[i].tPI.homeLocation.y = fieldPosition->y;
        match->playerInfo[i].tPI.homeLocation.z = fieldPosition->z;

        match->playerInfo[i].tPI.location.x = match->playerInfo[i].tPI.homeLocation.x;
        match->playerInfo[i].tPI.location.y = match->playerInfo[i].tPI.homeLocation.y;
        match->playerInfo[i].tPI.location.z = match->playerInfo[i].tPI.homeLocation.z;

        match->playerInfo[i].tPI.orientation.x = match->ballInfo.location.x - match->playerInfo[i].tPI.location.x;
        match->playerInfo[i].tPI.orientation.y = match->ballInfo.location.y - match->playerInfo[i].tPI.location.y;
        match->playerInfo[i].tPI.orientation.z = match->ballInfo.location.z - match->playerInfo[i].tPI.location.z;

        match->playerInfo[i].tPI.lastLocation.x = match->playerInfo[i].tPI.location.x;
        match->playerInfo[i].tPI.lastLocation.y = match->playerInfo[i].tPI.location.y;
        match->playerInfo[i].tPI.lastLocation.z = match->playerInfo[i].tPI.location.z;
    }
    // these are set for every player.
    for (i = 0; i < 2 * PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        match->playerInfo[i].tPI.velocity.x = 0.0f;
        match->playerInfo[i].tPI.velocity.y = 0.0f;
        match->playerInfo[i].tPI.velocity.z = 0.0f;
        match->playerInfo[i].tPI.targetLocation.x = 0.0f;
        match->playerInfo[i].tPI.targetLocation.y = 0.0f;
        match->playerInfo[i].tPI.targetLocation.z = 0.0f;
    }
}
// here we initialize players' stat information, and team information, whether they are joker or not,
// their number etc. all is information that is not going to be reinitialized when out of bounds -
// situation happens
void initialize_inning_permanent_player_information(
    MatchSession* match, const Scoreboard* scoreboard, const TeamData* team_data
)
{
    int battingTeamIndex = get_batting_team_index(scoreboard);
    int i;
    int jokerCounter = 0;

    // initialize batting team numbers and jokerness
    for (i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        if (i < PLAYERS_IN_TEAM) {
            match->playerInfo[scoreboard->teams[battingTeamIndex].batterOrder[i]].bTPI.number = i + 1;
            match->playerInfo[scoreboard->teams[battingTeamIndex].batterOrder[i]].bTPI.joker = JOKER_REGULAR;
        } else {
            match->playerInfo[scoreboard->teams[battingTeamIndex].batterOrder[i]].bTPI.number = 0;
            match->playerInfo[scoreboard->teams[battingTeamIndex].batterOrder[i]].bTPI.joker = JOKER_AVAILABLE;
        }
    }
    // initialize batting team
    for (i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        match->playerInfo[i].cPI.team = TEAM_BATTING;

        if (match->playerInfo[i].bTPI.joker == JOKER_AVAILABLE) {
            match->pII.jokerIndices[jokerCounter] = i;
            jokerCounter++;
        }
        match->playerInfo[i].bTPI.name = team_data[(scoreboard->teams[battingTeamIndex].value - 1)].players[i].name;
        match->playerInfo[i].bTPI.power = team_data[(scoreboard->teams[battingTeamIndex].value - 1)].players[i].power;
        match->playerInfo[i].bTPI.speed = team_data[(scoreboard->teams[battingTeamIndex].value - 1)].players[i].speed;
    }
    // initialize fielders
    for (i = 12; i < 2 * PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        match->playerInfo[i].cPI.team = TEAM_CATCHING;

        // here we set catcherOnBaseIndices and catcherReplacerOnBaseIndices.
        switch (i - 12) {
        case 0:
            match->pII.catcherOnBaseIndex[0] = i;
            break;
        case 1:
            match->pII.catcherOnBaseIndex[1] = i;
            break;
        case 2:
            match->pII.catcherOnBaseIndex[2] = i;
            break;
        case 3:
            match->pII.catcherOnBaseIndex[3] = i;
            break;
        case 4:
            match->pII.catcherReplacerOnBaseIndex[0] = i;
            match->pII.catcherReplacerOnBaseIndex[1] = i;
            break;
        case 5:
            match->pII.catcherReplacerOnBaseIndex[3] = i;
            break;
        case 6:
            match->pII.catcherReplacerOnBaseIndex[2] = i;
            break;
        }
    }
}
// information that can be flushed
void initialize_non_critical_player_information(MatchSession* match)
{
    int i, j;
    for (i = 0; i < 2 * PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        // MILESTONE 7.5: Initialize control state
        match->playerRuntime[i].arrivedToBase = 0;
        match->playerRuntime[i].passedPathPoint = 0;
        match->playerRuntime[i].goingForward = 0;
        match->playerRuntime[i].hasMadeRunOnThirdBase = 0;

        match->playerInfo[i].cPI.animationFrequency = 1;
        match->playerInfo[i].cPI.animationStage = 0;
        match->playerInfo[i].cPI.animationStageCount = 0;
        match->playerInfo[i].cPI.moving = 0;
        match->playerInfo[i].cPI.running = 0;
        match->playerInfo[i].cPI.looksForTarget = 0;
        match->playerInfo[i].cPI.lastLastLocationUpdate = 1;

        // Critical: throwRecoil must be 0 for all players so move_to_target doesn't block
        match->playerInfo[i].cTPI.throwRecoil = 0;

        if (i >= PLAYERS_IN_TEAM + JOKER_COUNT) {

            match->playerInfo[i].cTPI.isNearHomeLocation = 1;
            match->playerInfo[i].cTPI.replacingStage = REPLACEMENT_IDLE;
            match->playerInfo[i].cTPI.replacingBase = BASE_NONE;
            match->playerInfo[i].cTPI.busyCatching = 0;
            // throwRecoil already set above
            // initialize fielderRankedIndices with the indices of five first
            // players in positional order.
            if (i - 12 < RANKED_FIELDERS_COUNT) {
                match->pII.fielderRankedIndices[i - 12] = i;
            }

            for (j = 0; j < DIRECTION_COUNT; j++) {
                match->playerInfo[i].cTPI.movesToDirection[j] = 0;
            }
        } else {
            match->playerInfo[i].bTPI.state = PLAYER_STATE_IDLE;
            match->playerInfo[i].bTPI.baseId = BASE_NONE;
        }
    }
}
// ball flags
void initialize_ball_info(MatchSession* match)
{
    match->ballInfo.visible = 1;
    match->ballInfo.moving = 1;
    match->ballInfo.currentFlightHasHitGround = 0;
    match->ballInfo.onGround = 0;
    match->ballInfo.hitsGroundToUnWound = 0;
    match->ballInfo.needsMoveUpdate = 0;
    match->ballInfo.lastLastLocationUpdate = 0;
}
// action flag initialization
void initialize_action_info(MatchSession* match)
{
    int i;

    for (i = 0; i < BASE_COUNT; i++) {
        match->aF.bTAF.base_run[i] = 0;
    }
    match->aF.bTAF.choose_batter = 0;
    match->aF.bTAF.take_free_walk = 0;
    match->aF.bTAF.swing = 0;
    match->aF.bTAF.increase_batter_angle = 0;
    match->aF.bTAF.decrease_batter_angle = 0;

    for (i = 0; i < BASE_COUNT; i++) {
        match->aF.cTAF.move[i] = 0;
    }
    match->aF.cTAF.throw.target = BASE_NONE; // no throw declared
    match->aF.cTAF.change_player = 0;
    match->aF.cTAF.drop_ball = 0;
    match->aF.cTAF.pitch = 0;
    match->pendingActionState.pitch_phase = PITCH_PHASE_NONE;
}
// Resets flow control, camera, subsystems, and frame events for a clean restart.
// Does NOT touch referee-owned state (BPS, HIS, RefereeState).
void reset_flow_state(MatchSession* match, PlayerCounters* player_counters)
{
    // Flow control
    match->flowControl.pause = 0;
    match->flowControl.waitingForBatterDecision = 0;
    match->flowControl.waitingForFreeWalkDecision = 0;
    match->flowControl.freeWalkCalculationMade = 1;
    match->flowControl.freeWalkIndex = -1;
    match->flowControl.freeWalkBase = BASE_NONE;

    // Flow state
    player_counters->noMorePlayers = 0;
    match->gameFlowState.ballHome = 0;

    // Frame events (cleared every frame, but ensure clean start)
    clear_frame_events(&match->gameEvents);

    // Subsystem initialization
    consolidation_init(&(match->gameFlowState));
    init_game_manipulation(&(match->gameFlowState));

    // Action state
    match->pendingActionState.current_catching_action = CATCHING_ACTION_NONE;
    match->pendingActionState.pitch_phase = PITCH_PHASE_NONE;
    match->pendingActionState.throw_going_on = 0;
    match->pRAI.throw_going_to_base = -1;
    for (int b = 0; b < BASE_COUNT; b++) {
        match->pendingActionState.run_press_window[b] = 0; // drop any half-finished human double-press
    }

    // AI batting state: force re-planning on next pitch cycle.
    // Without this, after foul play the AI's planCalculated stays 1 (the batter_ready 0→1
    // transition happens within consolidation, AFTER AI has already run that frame, so the AI
    // never sees batter_ready==0 to trigger its own reset). The per-base run decisions need no
    // reset — the AI now derives them from live game state each frame (will_start_running +
    // player state), so there is no stale click-sim bookkeeping to clear.
    match->aiState.planCalculated = 0;

    // Camera
    match->cameraState.homeRunCameraFlag = 0;
    match->cameraState.targetPoint.x = 0.0f;
    match->cameraState.targetPoint.y = 0.0f;
    match->cameraState.targetPoint.z = 0.0f;
}

void clear_frame_events(GameEvents* events)
{
    events->catchMade = 0;
    events->playerArrivedAtBase = 0;
    events->pitchReleased = 0;
    events->ballHitByBat = 0;
    events->ballMissedByBat = 0;
    events->ballHitGround = 0;
    events->freeWalkAccepted = 0;
    events->freeWalkRejected = 0;
    events->batterEntered = 0;
    // gameInitialized removed in M18.0
}

// these should be kept when foul play
void initialize_critical_game_info(MatchSession* match, PlayerCounters* player_counters, const Scoreboard* scoreboard)
{
    int battingTeamIndex = get_batting_team_index(scoreboard);

    player_counters->nonJokerPlayersLeft = PLAYERS_IN_TEAM;
    player_counters->jokersLeft = 3;
    match->pII.batterSelectionIndex =
        scoreboard->teams[battingTeamIndex].batterOrder[scoreboard->teams[battingTeamIndex].batterOrderIndex];
}
// index information initialization, can be called when out of bounds
void initialize_index_information(MatchSession* match)
{
    match->pII.hasBallIndex = -1;
    match->pII.lastHadBallIndex = -1;
    match->pII.controlIndex = -1;
    match->pII.changePlayerArrayIndex = -1;
}
// player-related action information initialization, can be called when foul play.
void initialize_prai_information(MatchSession* match)
{
    int i;
    match->pRAI.pitch_state = PITCH_STAGE_NONE;
    match->pRAI.meter_value = 0.0f;
    match->pRAI.swing_meter_value = 0.0f;
    match->pRAI.batting_going_on = 0;
    match->pRAI.batter_can_advance = 0;
    match->pRAI.throw_going_to_base = -1;
    match->pRAI.batter_ready = 0;
    match->pRAI.refresh_catch_and_change = 0;
    match->pRAI.init_player_selection = 0;
    match->pRAI.init_batter = 0;
    for (i = 0; i < BASE_COUNT; i++) {
        match->pRAI.will_start_running[i] = 0;
    }
}

void setup_homerun_physical_state(
    MatchSession* match, const Scoreboard* scoreboard, const HomeRunContestState* hrcs,
    const FieldPositions* field_positions, int batterResumesInPlace
)
{
    int battingTeamIndex = get_batting_team_index(scoreboard);
    Vector3D target;
    int i;
    if (hrcs->runnerBatterPairCounter < scoreboard->pairCount) {
        int runnerIndex = scoreboard->teams[battingTeamIndex].batterRunnerIndices[1][hrcs->runnerBatterPairCounter];
        int batterIndex = scoreboard->teams[battingTeamIndex].batterRunnerIndices[0][hrcs->runnerBatterPairCounter];
        // batter
        if (batterIndex != -1) {
            match->playerInfo[batterIndex].bTPI.baseId = BASE_HOME;
            match->playerInfo[batterIndex].bTPI.state = PLAYER_STATE_AT_BAT;
            match->playerInfo[batterIndex].bTPI.number = hrcs->runnerBatterPairCounter + 1;
            // move player to default batter ready position
            target.x = (float)(field_positions->pitchPlate.x + cos(ZERO_BATTING_ANGLE) * BATTING_RADIUS);
            target.z = (float)(field_positions->pitchPlate.z - sin(ZERO_BATTING_ANGLE) * BATTING_RADIUS);
            // On a foul/out-of-bounds reset the same batter resumes, so start them already at
            // the plate — move_to_target then settles them straight into the ready stance
            // instead of walking them in from the reset position.
            if (batterResumesInPlace) {
                match->playerInfo[batterIndex].tPI.location.x = target.x;
                match->playerInfo[batterIndex].tPI.location.z = target.z;
                match->playerInfo[batterIndex].tPI.lastLocation = match->playerInfo[batterIndex].tPI.location;
            }
            move_to_target(match->playerInfo, batterIndex, &target);
        }
        // runner
        if (runnerIndex != -1) {
            match->playerInfo[runnerIndex].bTPI.baseId = BASE_THIRD;
            match->playerInfo[runnerIndex].bTPI.state = PLAYER_STATE_ON_BASE;

            match->playerInfo[runnerIndex].tPI.location.x = field_positions->thirdBaseRun.x;
            match->playerInfo[runnerIndex].tPI.location.y = field_positions->thirdBaseRun.y;
            match->playerInfo[runnerIndex].tPI.location.z = field_positions->thirdBaseRun.z;
            match->playerInfo[runnerIndex].tPI.lastLocation.x = match->playerInfo[runnerIndex].tPI.location.x;
            match->playerInfo[runnerIndex].tPI.lastLocation.y = match->playerInfo[runnerIndex].tPI.location.y;
            match->playerInfo[runnerIndex].tPI.lastLocation.z = match->playerInfo[runnerIndex].tPI.location.z;
            match->playerInfo[runnerIndex].tPI.orientation.x = -match->playerInfo[runnerIndex].tPI.location.x;
            match->playerInfo[runnerIndex].tPI.orientation.y = 0.0f;
            match->playerInfo[runnerIndex].tPI.orientation.z = -match->playerInfo[runnerIndex].tPI.location.x;
        }

        // set other runners next to the third base.        // set other runners next to the third base.
        for (i = hrcs->runnerBatterPairCounter + 1; i < scoreboard->pairCount; i++) {
            int index = scoreboard->teams[battingTeamIndex].batterRunnerIndices[1][i];
            if (index != -1) {
                match->playerInfo[index].tPI.location.x =
                    field_positions->thirdBaseRun.x - 2.0f - (i - (hrcs->runnerBatterPairCounter + 1)) * 1.5f;
                match->playerInfo[index].tPI.location.y = field_positions->thirdBaseRun.y;
                match->playerInfo[index].tPI.location.z = field_positions->thirdBaseRun.z;
                match->playerInfo[index].tPI.lastLocation.x = match->playerInfo[index].tPI.location.x;
                match->playerInfo[index].tPI.lastLocation.y = match->playerInfo[index].tPI.location.y;
                match->playerInfo[index].tPI.lastLocation.z = match->playerInfo[index].tPI.location.z;
                match->playerInfo[index].tPI.orientation.x = -match->playerInfo[index].tPI.location.x;
                match->playerInfo[index].tPI.orientation.y = 0.0f;
                match->playerInfo[index].tPI.orientation.z = -match->playerInfo[index].tPI.location.x;
            }
        }
    }
}
