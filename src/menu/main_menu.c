#include "globals.h"
#include "render.h"
#include "font.h"
#include "rng.h"

#include "main_menu.h"
#include "menu_types.h"
#include "team_selection_menu.h"
#include "batting_order_menu.h"
#include "hutunkeitto_menu.h"
#include "homerun_contest_menu.h"
#include "front_menu.h"
#include "game_over_menu.h"
#include "help_menu.h"
#include "menu_helpers.h"
#include "loading_screen_menu.h"
#include "cup_menu.h"
#include "cup.h"
#include "input.h"

#define CAM_HEIGHT 2.3f

int init_main_menu(StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo, ResourceManager* rm, RenderState* rs)
{
    menuInfo->mode = MENU_ENTRY_NORMAL;

    resource_manager_load_all_menu_assets(rm);
    init_front_menu_state(&menuData->front_menu);

    return 0;
}

void update_main_menu(
    StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo, KeyStates* keyStates, unsigned int* rng_seed
)
{
    MenuStage nextStage;
    if (stateInfo->changeScreen == 1) {
        stateInfo->changeScreen = 0;
        stateInfo->updated = 1;
        int i;
        switch (menuInfo->mode) {
        case MENU_ENTRY_NORMAL:
            reset_menu_for_new_game(menuData, stateInfo);
            break;
        case MENU_ENTRY_INTER_PERIOD:
            // When returning to a game, team info is in scoreboard
            init_batting_order_state(
                &menuData->batting_order, stateInfo->rules->scoreboard.teams[0].value - 1,
                stateInfo->rules->scoreboard.teams[0].control, stateInfo
            );
            menuData->stage = MENU_STAGE_BATTING_ORDER_1;
            for (i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
                menuData->pendingGameSetup.team1_batting_order[i] = i;
                menuData->pendingGameSetup.team2_batting_order[i] = i;
            }
            break;
        case MENU_ENTRY_SUPER_INNING:
            // When returning to a game, team info is in scoreboard
            init_batting_order_state(
                &menuData->batting_order, stateInfo->rules->scoreboard.teams[0].value - 1,
                stateInfo->rules->scoreboard.teams[0].control, stateInfo
            );
            menuData->stage = MENU_STAGE_BATTING_ORDER_1;
            for (i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
                menuData->pendingGameSetup.team1_batting_order[i] = i;
                menuData->pendingGameSetup.team2_batting_order[i] = i;
            }
            break;
        case MENU_ENTRY_HOMERUN_CONTEST: {
            int totalPicks = (stateInfo->rules->scoreboard.period == 4) ? 10 : 6;
            int team1Index = stateInfo->rules->scoreboard.teams[0].value - 1;
            int team2Index = stateInfo->rules->scoreboard.teams[1].value - 1;
            init_homerun_contest_state(
                &menuData->homerun1, team1Index, stateInfo->rules->scoreboard.teams[0].control, totalPicks
            );
            init_homerun_contest_state(
                &menuData->homerun2, team2Index, stateInfo->rules->scoreboard.teams[1].control, totalPicks
            );
            menuData->stage = MENU_STAGE_HOMERUN_CONTEST_1;
        } break;
        case MENU_ENTRY_GAME_OVER:
            menuData->stage = MENU_STAGE_GAME_OVER;
            break;
        }
        // No sound on game over screen, it's played on exit
        if (menuInfo->mode != MENU_ENTRY_GAME_OVER) {
            stateInfo->playSoundEffect = SOUND_MENU;
        }
    }
    switch (menuData->stage) {
    case MENU_STAGE_FRONT: {
        nextStage = update_front_menu(&menuData->front_menu, keyStates, stateInfo);
        if (nextStage == MENU_STAGE_CUP) {
            init_cup_menu(&menuData->cup_menu, stateInfo, rng_seed);
        } else if (nextStage == MENU_STAGE_TEAM_SELECTION) {
            stateInfo->rules->scoreboard.isCupGame = 0;
            init_team_selection_state(&menuData->team_selection, stateInfo->numTeams);
        } else if (nextStage == MENU_STAGE_HELP) {
            init_help_menu(&menuData->help_menu);
        } else if (nextStage == MENU_STAGE_QUIT) {
            stateInfo->screen = SCREEN_LOADING;
        }
        menuData->stage = nextStage;
        break;
    }
    case MENU_STAGE_TEAM_SELECTION: {
        nextStage = update_team_selection_menu(&menuData->team_selection, keyStates, &menuData->pendingGameSetup);
        if (nextStage != menuData->stage) {
            if (nextStage == MENU_STAGE_BATTING_ORDER_1) {
                init_batting_order_state(
                    &menuData->batting_order, menuData->pendingGameSetup.team1,
                    menuData->pendingGameSetup.team1_control, stateInfo
                );
            } else if (nextStage == MENU_STAGE_FRONT) {
                init_front_menu_state(&menuData->front_menu);
            }
        }
        menuData->stage = nextStage;
        break;
    }
    case MENU_STAGE_BATTING_ORDER_1: {
        nextStage = update_batting_order_menu(
            &menuData->batting_order, keyStates, menuData->stage, menuInfo->mode, &menuData->pendingGameSetup
        );
        if (nextStage != menuData->stage) {
            if (nextStage == MENU_STAGE_BATTING_ORDER_2) {
                init_batting_order_state(
                    &menuData->batting_order, menuData->pendingGameSetup.team2,
                    menuData->pendingGameSetup.team2_control, stateInfo
                );
            } else if (nextStage == MENU_STAGE_HUTUNKEITTO) {
                init_hutunkeitto_state(&menuData->hutunkeitto);
            }
        }
        menuData->stage = nextStage;
        break;
    }
    case MENU_STAGE_BATTING_ORDER_2: {
        nextStage = update_batting_order_menu(
            &menuData->batting_order, keyStates, menuData->stage, menuInfo->mode, &menuData->pendingGameSetup
        );
        if (nextStage != menuData->stage) {
            if (nextStage == MENU_STAGE_HUTUNKEITTO) {
                init_hutunkeitto_state(&menuData->hutunkeitto);
            }
            if (nextStage == MENU_STAGE_GO_TO_GAME) {
                if (menuInfo->mode == MENU_ENTRY_INTER_PERIOD || menuInfo->mode == MENU_ENTRY_SUPER_INNING) {
                    menuData->pendingGameSetup.launchType = GAME_LAUNCH_RETURN_INTER_PERIOD;
                    launch_game_from_menu(stateInfo, &menuData->pendingGameSetup, rng_seed);
                } else {
                    menuData->pendingGameSetup.gameMode = GAME_MODE_NORMAL;
                    menuData->pendingGameSetup.launchType = GAME_LAUNCH_NEW;
                    launch_game_from_menu(stateInfo, &menuData->pendingGameSetup, rng_seed);
                }
            }
        }
        menuData->stage = nextStage;
        break;
    }
    case MENU_STAGE_HUTUNKEITTO: {
        nextStage = update_hutunkeitto_menu(
            &menuData->hutunkeitto, keyStates, menuData->pendingGameSetup.team1_control,
            menuData->pendingGameSetup.team2_control, &menuData->pendingGameSetup, rng_seed
        );
        if (nextStage != menuData->stage) {
            if (nextStage == MENU_STAGE_GO_TO_GAME) {
                if (menuInfo->mode == MENU_ENTRY_SUPER_INNING) {
                    // Hutunkeitto reached as part of the super inning: resume the
                    // existing game, don't start a fresh one (which would wipe
                    // period/inning/run state and loop the game forever).
                    menuData->pendingGameSetup.launchType = GAME_LAUNCH_RETURN_SUPER_INNING;
                } else {
                    menuData->pendingGameSetup.gameMode = GAME_MODE_NORMAL;
                    menuData->pendingGameSetup.launchType = GAME_LAUNCH_NEW;
                }
                launch_game_from_menu(stateInfo, &menuData->pendingGameSetup, rng_seed);
                menuInfo->mode = MENU_ENTRY_NORMAL;
            }
        }
        menuData->stage = nextStage;
        break;
    }
    case MENU_STAGE_GAME_OVER: {
        nextStage = updateGameOverMenu(stateInfo->gameConclusion, keyStates);
        if (nextStage != menuData->stage) {
            stateInfo->playSoundEffect = SOUND_MENU;
            if (nextStage == MENU_STAGE_CUP) {
                // Process the finished game to update the tournament state
                if (stateInfo->gameConclusion->isCupGame && stateInfo->cup != NULL) {
                    // 1. Record the user's game result
                    int winner_index = stateInfo->gameConclusion->winner;
                    TeamID winner_id = stateInfo->rules->scoreboard.teams[winner_index].value - 1;
                    cup_update_match_result(stateInfo->cup, stateInfo->currently_played_cup_match_index, winner_id);

                    // 2. Simulate remaining AI matches for the current day
                    int match_indices[8];
                    int match_count = 0;
                    cup_get_matches_for_day(stateInfo->cup, stateInfo->cup->current_day, match_indices, &match_count);

                    for (int i = 0; i < match_count; i++) {
                        const CupMatch* match = &stateInfo->cup->matches[match_indices[i]];
                        // Only simulate if neither team is the user
                        if (match->team_a_id != stateInfo->cup->user_team_id &&
                            match->team_b_id != stateInfo->cup->user_team_id) {
                            int team_a_wins = seeded_rand(rng_seed, 2);
                            TeamID winner = team_a_wins ? match->team_a_id : match->team_b_id;
                            cup_update_match_result(stateInfo->cup, match_indices[i], winner);
                        }
                    }

                    // 3. Advance to next match day
                    cup_advance_to_next_match_day(stateInfo->cup);
                }

                // Initialize the cup menu logic/coordinates before setting specific state
                init_cup_menu(&menuData->cup_menu, stateInfo, rng_seed);

                // 3. Set up the cup menu to show ongoing screen
                menuData->cup_menu.screen = CUP_MENU_SCREEN_ONGOING;
                menuData->cup_menu.ongoing.pointer = 0;
                if (stateInfo->cup != NULL && stateInfo->cup->matches[0].winner_id != CUP_MATCH_NO_WINNER) {
                    menuData->cup_menu.ongoing.rem = 3; // Cup finished menu
                } else {
                    menuData->cup_menu.ongoing.rem = 5; // Cup in progress menu
                }

                // 4. Check if the winner is human-controlled (Player 1 or Player 2) and the cup is finished
                int local_winner_index = stateInfo->gameConclusion->winner;
                int winner_control = stateInfo->rules->scoreboard.teams[local_winner_index].control;

                if (stateInfo->cup != NULL && stateInfo->cup->matches[0].winner_id != CUP_MATCH_NO_WINNER &&
                    winner_control != 2) { // 2 = AI
                    menuData->cup_menu.screen = CUP_MENU_SCREEN_END_CREDITS;
                    menuData->cup_menu.credits_menu.creditsScrollX = VIRTUAL_WIDTH;
                }
            } else {
                reset_menu_for_new_game(menuData, stateInfo);
            }
        }
        menuData->stage = nextStage;
        break;
    }
    case MENU_STAGE_HOMERUN_CONTEST_1: {
        nextStage = update_homerun_contest_menu(
            &menuData->homerun1, keyStates, menuData->stage, &menuData->pendingGameSetup,
            (const TeamData*)stateInfo->teamData
        );
        if (nextStage != menuData->stage) {
            menuData->stage = nextStage;
        }
        break;
    }
    case MENU_STAGE_HOMERUN_CONTEST_2: {
        nextStage = update_homerun_contest_menu(
            &menuData->homerun2, keyStates, menuData->stage, &menuData->pendingGameSetup,
            (const TeamData*)stateInfo->teamData
        );
        if (nextStage != menuData->stage) {
            if (nextStage == MENU_STAGE_GO_TO_GAME) {
                menuData->pendingGameSetup.launchType = GAME_LAUNCH_RETURN_HOMERUN_CONTEST;
                launch_game_from_menu(stateInfo, &menuData->pendingGameSetup, rng_seed);
            }
            menuData->stage = nextStage;
        }
        break;
    }
    case MENU_STAGE_CUP: {
        CupMenuOutput cup_output;
        nextStage = update_cup_menu(&menuData->cup_menu, stateInfo, keyStates, &cup_output, rng_seed);
        if (nextStage == MENU_STAGE_BATTING_ORDER_1) {
            // A game is starting, transfer data from cup output to pendingGameSetup
            menuData->pendingGameSetup.team1 = cup_output.team1;
            menuData->pendingGameSetup.team2 = cup_output.team2;
            menuData->pendingGameSetup.team1_control = cup_output.team1_control;
            menuData->pendingGameSetup.team2_control = cup_output.team2_control;
            menuData->pendingGameSetup.halfInningsInPeriod = cup_output.innings;
            stateInfo->rules->scoreboard.isCupGame = 1;
            init_batting_order_state(
                &menuData->batting_order, menuData->pendingGameSetup.team1, menuData->pendingGameSetup.team1_control,
                stateInfo
            );
        } else if (nextStage == MENU_STAGE_FRONT) {
            stateInfo->rules->scoreboard.isCupGame = 0;
            init_front_menu_state(&menuData->front_menu);
        }
        menuData->stage = nextStage;
        break;
    }
    case MENU_STAGE_HELP: {
        nextStage = update_help_menu(&menuData->help_menu, keyStates);
        if (nextStage != menuData->stage) {
            if (nextStage == MENU_STAGE_FRONT) {
                init_front_menu_state(&menuData->front_menu);
            }
        }
        menuData->stage = nextStage;
        break;
    }
    case MENU_STAGE_GO_TO_GAME:
        break;
    case MENU_STAGE_QUIT:
        // Quit handled in FRONT case, no update here
        break;
    }
    clear_released_keys(keyStates);
}

void draw_main_menu(
    const StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo, double alpha, ResourceManager* rm,
    RenderState* rs
)
{
    switch (menuData->stage) {
    case MENU_STAGE_FRONT:
        draw_front_menu(&menuData->front_menu, rs, rm);
        break;
    case MENU_STAGE_HELP:
        draw_help_menu(&menuData->help_menu, rs, rm);
        break;
    case MENU_STAGE_BATTING_ORDER_1:
    case MENU_STAGE_BATTING_ORDER_2:
        draw_batting_order_menu(&menuData->batting_order, menuData->stage, rs, rm);
        break;
    case MENU_STAGE_TEAM_SELECTION:
        draw_team_selection_menu(&menuData->team_selection, (const TeamData*)stateInfo->teamData, rs, rm);
        break;
    case MENU_STAGE_HUTUNKEITTO:
        draw_hutunkeitto_menu(
            &menuData->hutunkeitto, rs, rm, menuData->pendingGameSetup.team1, menuData->pendingGameSetup.team2
        );
        break;

    case MENU_STAGE_GAME_OVER:
        draw_game_over_menu(stateInfo->gameConclusion, (const TeamData*)stateInfo->teamData, rs, rm);
        break;
    case MENU_STAGE_HOMERUN_CONTEST_1:
        draw_homerun_contest_menu(
            &menuData->homerun1, (const RenderState*)rs, rm, (const TeamData*)stateInfo->teamData
        );
        break;
    case MENU_STAGE_HOMERUN_CONTEST_2:
        draw_homerun_contest_menu(
            &menuData->homerun2, (const RenderState*)rs, rm, (const TeamData*)stateInfo->teamData
        );
        break;
    case MENU_STAGE_CUP:
        draw_cup_menu(&menuData->cup_menu, stateInfo, rs, rm);
        break;

    case MENU_STAGE_GO_TO_GAME:
    case MENU_STAGE_QUIT:
        break;
    }
}

int clean_main_menu(MenuData* menuData)
{
    // All resources are now cleaned up by the resource manager.
    return 0;
}
