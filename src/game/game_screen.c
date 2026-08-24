/*
    game divides into two sections, menus and the game. this is the game screen. only main.c is higher but lots of
   initialization code is still hidden to main.c. almost all visible 3d stuff is delegated to game_frame and
   immutable_world, but here we draw the skybox and some status information for user.
*/

#include "globals.h"
#include "immutable_world.h"
#include "game_frame.h"
#include "render.h"
#include "font.h"
#include "game_screen.h"
#include "common_logic.h"
#include "state_validator.h"
#include "referee.h"
#include "rules_pure/player_utils.h"
#include "game_reset.h"

#define STATISTICS_TEXT_HEIGHT -1.34f
#define OTHER_STATS_X -0.02f // Midpoint between original -0.12 and current 0.08
#define METER_X 0.32f
#define OUTS_X -0.69f // Reverted to original
#define INFO_X -0.60f // Reverted to original
#define BASES_X 0.50f
#define EVENT_TIMER_THRESHOLD (1.5 * (1 / (UPDATE_INTERVAL * 1.0f / 1000)))

static void draw_skybox(const StateInfo* stateInfo, ResourceManager* rm);
static void draw_statistics_2d(const StateInfo* stateInfo, double alpha, ResourceManager* rm, const RenderState* rs);
static int init_lights(StateInfo* stateInfo);
static void init_cam_settings(StateInfo* stateInfo);
static void load_game_screen_settings(StateInfo* stateInfo);

int init_game_screen(StateInfo* stateInfo, ResourceManager* rm)
{
    int result;

    resource_manager_load_all_game_assets(rm);

    init_cam_settings(stateInfo);

    stateInfo->match->uiState.lastMeterX = 0;
    stateInfo->match->uiState.lastSwingMeterX = 0;

    result = init_lights(stateInfo);
    if (result != 0) {
        printf("Could not init lights. Exiting.");
    }

    result = init_immutable_world(stateInfo, rm);
    if (result != 0) {
        printf("Could not init immutable world.");
        return -1;
    }

    result = init_game_frame(stateInfo, rm);
    if (result != 0) {
        printf("Could not init ball. Exiting.");
        return -1;
    }

    return 0;
}

void update_game_screen(StateInfo* stateInfo, MenuInfo* menuInfo)
{
    BallInfo* ballInfo = &(stateInfo->match->ballInfo);
    CameraState* cs = &(stateInfo->match->cameraState);
    // if we just changed here, load basic settings.
    if (stateInfo->changeScreen == 1) {
        stateInfo->changeScreen = 0;
        stateInfo->updated = 1;
        load_game_screen_settings(stateInfo);
    }
    // with home-key, one can return to main menu.
    if (((stateInfo->keyStates)->released[0][KEY_HOME] || (stateInfo->keyStates)->released[1][KEY_HOME])) {
        if (stateInfo->match->flowControl.pause == 0) {
            stateInfo->match->flowControl.pause = 1;
            state_validator_dump(stateInfo, "Manual Pause");
        } else if (stateInfo->match->flowControl.pause == 1) {
            stateInfo->changeScreen = 1;
            stateInfo->updated = 0;
            stateInfo->screen = SCREEN_MAIN_MENU;
            menuInfo->mode = MENU_ENTRY_NORMAL;
        }
    }
    if (stateInfo->match->flowControl.pause == 1) {
        if (((stateInfo->keyStates)->released[0][KEY_2] || (stateInfo->keyStates)->released[1][KEY_2])) {
            stateInfo->match->flowControl.pause = 0;
        }
    }
    // update camera
    cs->lastCamTargetLocation.x = cs->camTargetLocation.x;
    cs->lastCamTargetLocation.y = cs->camTargetLocation.y;
    cs->lastCamTargetLocation.z = cs->camTargetLocation.z;

    cs->lastCamLocation.y = cs->camLocation.y;
    cs->lastCamLocation.z = cs->camLocation.z;
    cs->lastCamLocation.x = cs->camLocation.x;
    // we follow the ball with our camera
    cs->camTargetLocation.x = ballInfo->location.x;
    cs->camTargetLocation.y = ballInfo->location.y * 0.5f;
    cs->camTargetLocation.z = ballInfo->location.z;

    // and our camera also moves a bit with the ball.

    cs->camLocation.x = 0.1f * cs->camTargetLocation.x;
    // if we are behind the homebase
    if (stateInfo->match->ballInfo.location.z > HOME_RADIUS) {
        cs->camLocation.y = cs->camTargetLocation.y - cs->camTargetLocation.z * 0.1f + 3.0f + 5.0f +
                            (float)fabs(cs->camTargetLocation.x) / 3;
        cs->camLocation.z = cs->camTargetLocation.z - 0.3f * cs->camTargetLocation.z + 12.0f + 15.0f +
                            (float)fabs(cs->camTargetLocation.x) / 2;
    }
    // if runner coming from third base
    else if (stateInfo->match->cameraState.homeRunCameraFlag == 0) {
        cs->camLocation.y = cs->camTargetLocation.y - cs->camTargetLocation.z * 0.1f + 3.0f;
        cs->camLocation.z = cs->camTargetLocation.z - 0.3f * cs->camTargetLocation.z + 12.0f;
    }
    // and the normal mode
    else {
        cs->camLocation.y = cs->camTargetLocation.y - cs->camTargetLocation.z * 0.1f + 3.0f + 5.0f;
        cs->camLocation.z = cs->camTargetLocation.z - 0.3f * cs->camTargetLocation.z + 12.0f + 8.0f;
    }
    // cant update this at the drawing-function as that doesnt happen synchroniusly.
    // so when info event happens, we set gameInfoEventTimer to 0 and this will start increasing it until
    // we hit the threshold.
    if (stateInfo->match->uiState.gameInfoEventTimer != -1) {
        stateInfo->match->uiState.gameInfoEventTimer += 1;

        if (stateInfo->match->uiState.gameInfoEventTimer > (int)EVENT_TIMER_THRESHOLD) {
            stateInfo->match->uiState.gameInfoEventTimer = -1;
        }
    }

    // and here will a lot of logic code.
    update_game_frame(stateInfo, menuInfo);
}

void draw_game_screen(const StateInfo* stateInfo, double alpha, ResourceManager* rm, const RenderState* rs)
{
    // Ensure the 3D rendering state is correctly set up before drawing the game screen.
    begin_3d_render(rs);
    CameraState* cs = (CameraState*)&(stateInfo->match->cameraState);
    Vector3D look, cam, skyBoxLook;

    look.x = (float)(alpha * cs->camTargetLocation.x + (1 - alpha) * cs->lastCamTargetLocation.x);
    look.y = (float)(alpha * cs->camTargetLocation.y + (1 - alpha) * cs->lastCamTargetLocation.y);
    look.z = (float)(alpha * cs->camTargetLocation.z + (1 - alpha) * cs->lastCamTargetLocation.z);
    cam.y = (float)(alpha * cs->camLocation.y + (1 - alpha) * cs->lastCamLocation.y);
    cam.z = (float)(alpha * cs->camLocation.z + (1 - alpha) * cs->lastCamLocation.z);
    cam.x = (float)(alpha * cs->camLocation.x + (1 - alpha) * cs->lastCamLocation.x);
    // with skybox we move to origin
    skyBoxLook.x = look.x - cam.x;
    skyBoxLook.y = look.y - cam.y;
    skyBoxLook.z = look.z - cam.z;
    // first we draw the skybox. It should not be lit or write to the depth buffer.
    glDisable(GL_LIGHTING);
    glDepthMask(GL_FALSE);
    glLoadIdentity();
    gluLookAt(
        cs->skyBoxCam.x, cs->skyBoxCam.y, cs->skyBoxCam.z, skyBoxLook.x, skyBoxLook.y, skyBoxLook.z, cs->up.x, cs->up.y,
        cs->up.z
    );

    draw_skybox(stateInfo, rm);

    // Re-enable depth writes for the rest of the scene.
    glDepthMask(GL_TRUE);

    glLoadIdentity();
    gluLookAt(cam.x, cam.y, cam.z, look.x, look.y, look.z, cs->up.x, cs->up.y, cs->up.z);
    glEnable(GL_LIGHTING);
    glLightfv(GL_LIGHT0, GL_POSITION, cs->lightPos);

    draw_immutable_world(stateInfo, alpha, rm);
    draw_game_frame(stateInfo, alpha, rm);

    // statistics - Switch to 2D
    begin_2d_render(rs);
    draw_statistics_2d(stateInfo, alpha, rm, rs);
}

// lights
static int init_lights(StateInfo* stateInfo)
{
    // our lighting is a point light that is so far away that its practically a directional light.
    //
    // some settings, quite random, but reasonable.
    float globalAmb[] = {0.8f, 0.8f, 0.8f, 1.0f};
    float diffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float ambient[] = {0.8f, 0.8f, 0.8f, 1.0f};
    float specular[] = {1.0f, 1.0f, 1.0f, 1.0f};
    CameraState* cs = &(stateInfo->match->cameraState);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    cs->lightPos[0] = LIGHT_SOURCE_POSITION_X;
    cs->lightPos[1] = LIGHT_SOURCE_POSITION_Y;
    cs->lightPos[2] = LIGHT_SOURCE_POSITION_Z;
    cs->lightPos[3] = 1.0f;
    glLightfv(GL_LIGHT0, GL_POSITION, cs->lightPos);

    return 0;
}

static void draw_skybox(const StateInfo* stateInfo, ResourceManager* rm)
{
    glBindTexture(GL_TEXTURE_2D, resource_manager_get_texture(rm, "data/textures/skybox.tga"));
    glPushMatrix();
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    glCallList(resource_manager_get_model(rm, "data/models/skybox.obj"));
    glPopMatrix();
}

static void draw_statistics_2d(const StateInfo* stateInfo, double alpha, ResourceManager* rm, const RenderState* rs)
{
    char* str4;
    char str5[2] = "x";
    float swingMeterX;
    float meterX;
    UIState* us = &(stateInfo->match->uiState);

    // Layout Constants
    const float BASE_Y = VIRTUAL_HEIGHT - 80.0f;
    const float TEXT_Y = BASE_Y + 20.0f;
    const float CENTER_X = VIRTUAL_WIDTH / 2.0f;
    // Direct pixel offsets from center (derived from previous SCALE_FACTOR 1350.0f)
    const float OUTS_OFFSET_X = -931.5f; // -0.69 * 1350
    const float INFO_OFFSET_X = -810.0f; // -0.60 * 1350
    const float STATS_OFFSET_X = -27.0f; // -0.02 * 1350
    const float METER_OFFSET_X = 432.0f; //  0.32 * 1350
    const float BASES_OFFSET_X = 675.0f; //  0.50 * 1350

    const float FONT_SIZE = 40.0f;

    // Background
    // Draw a dark strip at the bottom
    glBindTexture(GL_TEXTURE_2D, resource_manager_get_texture(rm, "data/textures/empty_background.tga"));
    // Using a simple quad logic here to avoid messing up with scaling of the plane model
    // Actually draw_texture_2d does exactly this.
    draw_texture_2d(
        resource_manager_get_texture(rm, "data/textures/empty_background.tga"), 0, BASE_Y, VIRTUAL_WIDTH, 80.0f
    ); // Height 80 (was 120)

    // OUTS (Far Left)
    float outs_x = CENTER_X + OUTS_OFFSET_X;
    if (stateInfo->rules->scoreboard.period < 4) {
        switch (stateInfo->rules->halfInningState.outs) {
        case 3:
            print_text_2d("XXX", 3, outs_x, TEXT_Y, FONT_SIZE);
            break;
        case 2:
            print_text_2d("XX", 2, outs_x, TEXT_Y, FONT_SIZE);
            break;
        case 1:
            print_text_2d("X", 1, outs_x, TEXT_Y, FONT_SIZE);
            break;
        case 0:
        default:
            break;
        }
    }

    // INFO AREA (Left-Center)
    float info_x = CENTER_X + INFO_OFFSET_X;

    if (stateInfo->rules->halfInningState.event != EVENT_NONE) {
        us->gameInfoEventTimer = 0;
        us->gameInfoEvent = (int)stateInfo->rules->halfInningState.event;
        stateInfo->rules->halfInningState.event = EVENT_NONE;
    }

    if (us->gameInfoEventTimer != -1) {
        const char* msg = "";
        switch (us->gameInfoEvent) {
        case EVENT_OUT:
            msg = "OUT";
            break;
        case EVENT_WOUNDED:
            msg = "WOUNDED";
            break;
        case EVENT_RUN_SCORED:
            msg = "RUN";
            break;
        case EVENT_OUT_OF_BOUNDS:
            msg = "OUT OF BOUNDS";
            break;
        case EVENT_STRIKE:
            msg = "STRIKE";
            break;
        case EVENT_BALL:
            msg = "BALL";
            break;
        case EVENT_INNING_ENDING:
            msg = "HALF-INNING ENDS";
            break;
        case EVENT_NEXT_PAIR:
            msg = "NEXT PAIR";
            break;
        case EVENT_TWO_RUNS_SCORED:
            msg = "TWO RUNS";
            break;
        }
        print_text_2d(msg, (unsigned int)strlen(msg), info_x, TEXT_Y, FONT_SIZE);
    } else if (stateInfo->match->flowControl.waitingForFreeWalkDecision == 1) {
        print_text_2d("Take a walk", 11, info_x, TEXT_Y, FONT_SIZE);
    } else {
        // Player selection info
        int index;
        int shouldContinue = 1;
        if (stateInfo->rules->scoreboard.period < 4) {
            if (stateInfo->match->pII.batterSelectionIndex == -1)
                shouldContinue = 0;
            else
                index = stateInfo->match->pII.batterSelectionIndex;
        } else {
            int battingTeamIndex = get_batting_team_index(&stateInfo->rules->scoreboard);
            int pairIdx = stateInfo->rules->homeRunContestState.runnerBatterPairCounter;
            // pairIdx reaches pairCount at end-of-inning; clamp so we never read past the
            // current pair row of batterRunnerIndices.
            if (pairIdx < 0 || pairIdx >= stateInfo->rules->scoreboard.pairCount) {
                shouldContinue = 0;
            } else {
                index = stateInfo->rules->scoreboard.teams[battingTeamIndex].batterRunnerIndices[0][pairIdx];
                if (index == -1) shouldContinue = 0;
            }
        }
        if (shouldContinue == 1) {
            int speed = stateInfo->match->playerInfo[index].bTPI.speed;
            int power = stateInfo->match->playerInfo[index].bTPI.power;
            char p_str[32];
            sprintf(p_str, "S%d P%d", speed, power);
            print_text_2d(p_str, (unsigned int)strlen(p_str), info_x, TEXT_Y, FONT_SIZE);

            if (stateInfo->match->playerInfo[index].bTPI.joker != JOKER_REGULAR &&
                stateInfo->rules->scoreboard.period < 4) {
                str5[0] = 'J';
            } else {
                str5[0] = (char)(((int)'0') + stateInfo->match->playerInfo[index].bTPI.number);
            }
            // Number at +180 (gap 40 from S/P), Name at +250 (gap 42 from Number)
            print_text_2d(str5, 1, info_x + 180.0f, TEXT_Y, FONT_SIZE);

            str4 = stateInfo->match->playerInfo[index].bTPI.name;
            print_text_2d(str4, (unsigned int)strlen(str4), info_x + 250.0f, TEXT_Y, FONT_SIZE);
        }
    }

    // STATS (Center)
    float stats_x = CENTER_X + STATS_OFFSET_X;

    // Inning
    char inn_str[16] = "I  ";
    if (stateInfo->rules->scoreboard.period < 4) {
        inn_str[2] = (char)(((int)'0') + stateInfo->rules->scoreboard.inning / 2 + 1);
    } else {
        inn_str[2] = '0';
    }
    print_text_2d(inn_str, 3, stats_x - 140.0f, TEXT_Y, FONT_SIZE); // Moved Left

    // Balls
    char ball_str[16] = "B  ";
    if (stateInfo->rules->halfInningState.balls < 10) {
        ball_str[2] = (char)(((int)'0') + stateInfo->rules->halfInningState.balls);
    } else {
        sprintf(ball_str, "B%d", stateInfo->rules->halfInningState.balls);
    }
    print_text_2d(ball_str, (unsigned int)strlen(ball_str), stats_x - 20.0f, TEXT_Y, FONT_SIZE); // Evenly spaced

    // Strikes
    char strike_str[16] = "S  ";
    strike_str[2] = (char)(((int)'0') + stateInfo->rules->halfInningState.strikes);
    print_text_2d(strike_str, 3, stats_x + 100.0f, TEXT_Y, FONT_SIZE); // Evenly spaced

    // Runs
    char runs_str[32];
    sprintf(
        runs_str, "R %d - %d", stateInfo->rules->scoreboard.teams[0].runs, stateInfo->rules->scoreboard.teams[1].runs
    );
    print_text_2d(runs_str, (unsigned int)strlen(runs_str), stats_x + 220.0f, TEXT_Y, FONT_SIZE); // Moved Right

    // METER (Right-Center)
    float meter_screen_x = CENTER_X + METER_OFFSET_X;
    float meter_y = BASE_Y + 17.0f; // Shifted 12px down relative to old (Old was BASE_Y(-120)+25 = -95)
    float meter_w = 200.0f;
    float meter_h = 40.0f;

    draw_texture_2d(
        resource_manager_get_texture(rm, "data/textures/meter.tga"), meter_screen_x, meter_y, meter_w, meter_h
    );

    // Meter Markers. The cursor value is in [0,1]; the marker travels the full bar so its edges sit flush
    // with the bar's left/right ends (no gap) — its travel range is (meter_w − marker_w), not meter_w.
    float marker_w = 5.0f;
    float marker_travel = meter_w - marker_w;
    meterX = 0.16f * stateInfo->match->pRAI.meter_value;
    swingMeterX = 0.16f * stateInfo->match->pRAI.swing_meter_value;

    float field_marker_x = meter_screen_x + (alpha * meterX + (1 - alpha) * us->lastMeterX) / 0.16f * marker_travel;
    float swing_marker_x =
        meter_screen_x + (alpha * swingMeterX + (1 - alpha) * us->lastSwingMeterX) / 0.16f * marker_travel;

    us->lastMeterX = meterX;
    us->lastSwingMeterX = swingMeterX;

    draw_texture_2d(
        resource_manager_get_texture(rm, "data/textures/selectionBall3.tga"), field_marker_x, meter_y, marker_w, 40.0f
    );
    draw_texture_2d(
        resource_manager_get_texture(rm, "data/textures/selectionBall1.tga"), swing_marker_x, meter_y, marker_w, 40.0f
    );

    // Pitch dance — a SECOND catching-team marker. While the human aims (power already locked, the aim
    // cursor descending = field_marker above), also show a STATIC marker at the locked power, in the same
    // catching-team marker colour. This removes the old visual "jump" (the single cursor used to teleport
    // to the far right on power-lock) and is the first instance of the dance's multi-marker meter (each
    // team will eventually show its own power + aim markers, same colour within a team). Shown only during
    // the human aim phase; the AI drives no widget, so an AI pitch never shows a marker. Render-only —
    // reads the client-local widget phase + the declared (synced) power.
    if (stateInfo->clientInput->pitchWidget.mode == WIDGET_DESCENT) {
        float locked_power = stateInfo->match->pendingActionState.pitchDeclaration.power;
        float power_marker_x = meter_screen_x + locked_power * marker_travel;
        draw_texture_2d(
            resource_manager_get_texture(rm, "data/textures/selectionBall3.tga"), power_marker_x, meter_y, marker_w,
            40.0f
        );
    }

    // BASES (Far Right)
    float bases_screen_x = CENTER_X + BASES_OFFSET_X;
    float bases_y = BASE_Y + 17.0f; // Shifted 12px down relative to old (Old was BASE_Y(-120)+25 = -95)
    float bases_w = 200.0f;
    float bases_h = 40.0f;

    draw_texture_2d(
        resource_manager_get_texture(rm, "data/textures/bases.tga"), bases_screen_x, bases_y, bases_w, bases_h
    );

    // Base Runners
    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        int index = i;
        if (stateInfo->match->playerInfo[index].bTPI.baseId != BASE_NONE) {
            float phase = 0.0f;
            BaseID base = BASE_HOME;
            float distance;
            // Logic copied from original
            if (stateInfo->match->playerInfo[index].bTPI.baseId == BASE_HOME) {
                if (stateInfo->match->playerInfo[index].tPI.location.z - HOME_LINE_Z > 0) {
                    phase = 0.0f;
                } else {
                    distance = stateInfo->fieldPositions->firstBaseRun.z - HOME_LINE_Z;
                    phase = (stateInfo->match->playerInfo[index].tPI.location.z - HOME_LINE_Z) / distance;
                }
                base = BASE_HOME;
            } else if (stateInfo->match->playerInfo[index].bTPI.baseId == BASE_FIRST) {
                distance = stateInfo->fieldPositions->secondBaseRun.x - stateInfo->fieldPositions->firstBaseRun.x;
                phase =
                    (stateInfo->match->playerInfo[index].tPI.location.x - stateInfo->fieldPositions->firstBaseRun.x) /
                    distance;
                base = BASE_FIRST;
            } else if (stateInfo->match->playerInfo[index].bTPI.baseId == BASE_SECOND) {
                distance = stateInfo->fieldPositions->thirdBaseRun.x - stateInfo->fieldPositions->secondBaseRun.x;
                phase =
                    (stateInfo->match->playerInfo[index].tPI.location.x - stateInfo->fieldPositions->secondBaseRun.x) /
                    distance;
                base = BASE_SECOND;
            } else if (stateInfo->match->playerInfo[index].bTPI.baseId == BASE_THIRD) {
                distance = HOME_LINE_Z - stateInfo->fieldPositions->thirdBaseRun.z;
                phase = (stateInfo->match->playerInfo[index].tPI.location.z - stateInfo->fieldPositions->thirdBase.z) /
                        distance;
                base = BASE_THIRD;
            } else if (stateInfo->match->playerInfo[index].bTPI.baseId == BASE_HOME_SCORED) {
                phase = 0.0f;
                base = BASE_HOME_SCORED;
            }

            // Map to screen
            // width is bases_w
            float interval_px = bases_w / 4.0f;
            float runner_x = bases_screen_x + ((int)base * interval_px) + (phase * interval_px) -
                             6.0f; // -6.0f offset to center on image spots

            draw_texture_2d(
                resource_manager_get_texture(rm, "data/textures/basesMarker.tga"), runner_x, bases_y + 10.0f, 15.0f,
                15.0f
            );
        }
    }
}

static void load_game_screen_settings(StateInfo* stateInfo)
{
    stateInfo->match->uiState.gameInfoEventTimer = -1;
    // initialize cam
    init_cam_settings(stateInfo);
    // Physical world + flow + team setup
    reset_for_new_half_inning(stateInfo->match, stateInfo->fieldPositions, stateInfo->teamData, stateInfo->rules);
    // Referee initialization (from-menu only — no state machine active, full clean slate)
    referee_reset_for_new_inning(
        &stateInfo->rules->referee, &stateInfo->rules->halfInningState, &stateInfo->rules->betweenPitchState
    );
    // Scan physical world to establish initial legal tracking
    initialize_referee(stateInfo, &stateInfo->rules->referee);
}

static void init_cam_settings(StateInfo* stateInfo)
{
    CameraState* cs = &(stateInfo->match->cameraState);

    cs->lastCamTargetLocation.x = 0.0f;
    cs->lastCamTargetLocation.y = 0.0f;
    cs->lastCamTargetLocation.z = 0.0f;
    cs->camTargetLocation.x = 0.0f;
    cs->camTargetLocation.y = 0.0f;
    cs->camTargetLocation.z = -1.0f;
    cs->lastCamLocation.x = 0.0f;
    cs->lastCamLocation.y = 0.0f;
    cs->lastCamLocation.z = 0.0f;
    cs->camLocation.x = 0.0f;
    cs->camLocation.y = 0.0f;
    cs->camLocation.z = 0.0f;

    cs->cam.x = cs->camLocation.x;
    cs->cam.y = 0.0f;
    cs->cam.z = 0.0f;
    cs->up.x = 0.0f;
    cs->up.y = 1.0f;
    cs->up.z = 0.0f;
    cs->look.x = 0.0f;
    cs->look.y = 0.0f;
    cs->look.z = 0.0f;

    cs->skyBoxCam.x = 0.0f;
    cs->skyBoxCam.y = 0.0f;
    cs->skyBoxCam.z = 0.0f;
    cs->skyBoxLook.x = 0.0f;
    cs->skyBoxLook.y = 0.0f;
    cs->skyBoxLook.z = -1.0f;
    // these values are kinda trial and error values to move the statistics window to right place. i think proper
    // deriving is unnecessary as we use perspective projection at the same time.

    // to move statistics to safe zone of tv
    cs->statCam.x = 0.0f;
    cs->statCam.y = 1.9f;
    cs->statCam.z = -1.69f;
    cs->statUp.x = 0.0f;
    cs->statUp.y = 0.0f;
    cs->statUp.z = -1.0f;
    cs->statLook.x = 0.0f;
    cs->statLook.y = 0.0f;
    cs->statLook.z = -1.69f;
}

int clean_game_screen(StateInfo* stateInfo)
{
    int result;

    result = clean_immutable_world(stateInfo);
    if (result != 0) {
        printf("Could not clean immutable world properly.\n");
        return -1;
    }

    result = clean_game_frame(stateInfo);
    if (result != 0) {
        printf("Could not clean mutable world properly.\n");
        return -1;
    }

    return 0;
}