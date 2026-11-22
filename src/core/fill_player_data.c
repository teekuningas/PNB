#include <mxml.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "team_data.h"
#include "session.h"
#include "fill_player_data.h"

static int freeTeam(TeamData* teamData);
static int readTeamsFromFile(StateInfo* stateInfo, const char* filename);
static int mxmlCountNodes(mxml_node_t* tree, const char* elementName);

int fill_player_data(StateInfo* stateInfo, const char* filename)
{
    stateInfo->num_teams = 0;
    stateInfo->team_data = NULL;
    return readTeamsFromFile(stateInfo, filename);
}

int clean_player_data(StateInfo* stateInfo)
{
    for (int i = 0; i < stateInfo->num_teams; i++) {
        freeTeam(&(stateInfo->team_data[i]));
    }
    free(stateInfo->team_data);
    return 0;
}

static int freeTeam(TeamData* teamData)
{
    for (int i = 0; i < teamData->num_players; i++) {
        free(teamData->players[i].id);
        free(teamData->players[i].name);
    }
    free(teamData->players);
    free(teamData->id);
    free(teamData->name);
    return 0;
}

static int readTeamsFromFile(StateInfo* stateInfo, const char* filename)
{
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Failed to open file: %s\n", filename);
        return -1;
    }

    mxml_node_t* tree = mxmlLoadFile(NULL, file, MXML_OPAQUE_CALLBACK);
    fclose(file);
    if (tree == NULL) {
        printf("Failed to parse XML file: %s\n", filename);
        return -1;
    }

    mxml_node_t* root = mxmlFindElement(tree, tree, "teams", NULL, NULL, MXML_DESCEND);
    if (root == NULL) {
        printf("No 'teams' element found in XML file: %s\n", filename);
        mxmlDelete(tree);
        return -1;
    }

    // Count number of teams and allocate memory for team_data array
    stateInfo->num_teams = mxmlCountNodes(root, "team");
    stateInfo->team_data = calloc(stateInfo->num_teams, sizeof(TeamData));
    if (stateInfo->team_data == NULL) {
        printf("Failed to allocate memory for teams\n");
        mxmlDelete(tree);
        return -1;
    }

    // Populate team_data array
    int teamIndex = 0;
    mxml_node_t* teamNode = mxmlFindElement(root, root, "team", NULL, NULL, MXML_DESCEND_FIRST);
    while (teamNode != NULL) {

        // Get team id and name, and allocate memory for them
        const char* teamId = mxmlElementGetAttr(teamNode, "id");
        mxml_node_t* teamNameNode = mxmlFindElement(teamNode, tree, "team_name", NULL, NULL, MXML_DESCEND);
        if (teamId == NULL || teamNameNode == NULL) {
            printf("Failed to read team id or name\n");
            mxmlDelete(tree);
            return -1;
        }
        stateInfo->team_data[teamIndex].id = strdup(teamId);
        stateInfo->team_data[teamIndex].name = strdup(mxmlGetOpaque(teamNameNode));

        // Read players and fill the players array in the same way...
        mxml_node_t* playersNode = mxmlFindElement(teamNode, tree, "players", NULL, NULL, MXML_DESCEND);
        if (playersNode == NULL) {
            printf("No 'players' element found in team: %s\n", stateInfo->team_data[teamIndex].id);
            mxmlDelete(tree);
            return -1;
        }

        // Count number of players and allocate memory for players array
        stateInfo->team_data[teamIndex].num_players = mxmlCountNodes(playersNode, "player");
        stateInfo->team_data[teamIndex].players =
            calloc(stateInfo->team_data[teamIndex].num_players, sizeof(PlayerData));
        if (stateInfo->team_data[teamIndex].players == NULL) {
            printf("Failed to allocate memory for players in team: %s\n", stateInfo->team_data[teamIndex].id);
            mxmlDelete(tree);
            return -1;
        }

        // Populate players array
        int playerIndex = 0;
        mxml_node_t* playerNode = mxmlFindElement(playersNode, playersNode, "player", NULL, NULL, MXML_DESCEND_FIRST);
        while (playerNode != NULL) {
            // Get player id, name, speed, and power, and allocate memory for id and name
            const char* playerId = mxmlElementGetAttr(playerNode, "id");
            mxml_node_t* playerNameNode = mxmlFindElement(playerNode, tree, "name", NULL, NULL, MXML_DESCEND);
            mxml_node_t* playerSpeedNode = mxmlFindElement(playerNode, tree, "speed", NULL, NULL, MXML_DESCEND);
            mxml_node_t* playerPowerNode = mxmlFindElement(playerNode, tree, "power", NULL, NULL, MXML_DESCEND);
            if (playerId == NULL || playerNameNode == NULL || playerSpeedNode == NULL || playerPowerNode == NULL) {
                printf("Failed to read player data\n");
                mxmlDelete(tree);
                return -1;
            }
            stateInfo->team_data[teamIndex].players[playerIndex].id = strdup(playerId);
            stateInfo->team_data[teamIndex].players[playerIndex].name = strdup(mxmlGetOpaque(playerNameNode));
            stateInfo->team_data[teamIndex].players[playerIndex].speed = atoi(mxmlGetOpaque(playerSpeedNode));
            stateInfo->team_data[teamIndex].players[playerIndex].power = atoi(mxmlGetOpaque(playerPowerNode));
            playerNode = mxmlFindElement(playerNode, playersNode, "player", NULL, NULL, MXML_NO_DESCEND);
            playerIndex++;
        }

        teamNode = mxmlFindElement(teamNode, tree, "team", NULL, NULL, MXML_NO_DESCEND);
        teamIndex++;
    }

    mxmlDelete(tree);

    return 0;
}

static int mxmlCountNodes(mxml_node_t* tree, const char* elementName)
{
    int count = 0;
    for (mxml_node_t* node = mxmlFindElement(tree, tree, elementName, NULL, NULL, MXML_DESCEND); node != NULL;
         node = mxmlFindElement(node, tree, elementName, NULL, NULL, MXML_DESCEND)) {
        count++;
    }
    return count;
}
