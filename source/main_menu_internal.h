#ifndef MAIN_MENU_INTERNAL_H
#define MAIN_MENU_INTERNAL_H

#if defined(__wii__)
#include "arrow_tpl.h"
#define arrow 0
#include "catcher_tpl.h"
#define catcher 0
#include "batter_tpl.h"
#define batter 0
#include "team1_huttu_tpl.h"
#define team1_huttu 0
#include "team2_huttu_tpl.h"
#define team2_huttu 0
#include "team3_huttu_tpl.h"
#define team3_huttu 0
#include "team4_huttu_tpl.h"
#define team4_huttu 0
#include "team5_huttu_tpl.h"
#define team5_huttu 0
#include "team6_huttu_tpl.h"
#define team6_huttu 0
#include "team7_huttu_tpl.h"
#define team7_huttu 0
#include "team8_huttu_tpl.h"
#define team8_huttu 0
#include "slot_tpl.h"
#define slot_texture 0
#include "menu_trophy_tpl.h"
#define menu_trophy_texture 0

#endif

#define LOADING_MODELS_HEIGHT -0.15f
#define LOADING_APPRECIATED_HEIGHT 0.0f
#define LOADING_AUTHOR_HEIGHT 0.45f

#define ARROW_SCALE 0.05f
#define ARROW_SMALLER_SCALE 0.03f
#define FIGURE_SCALE 0.4f
#define FRONT_ARROW_POS 0.15f
#define SELECTION_ARROW_LEFT -0.05f
#define SELECTION_ARROW_RIGHT 0.4f
#define PLAYER_LIST_ARROW_CONTINUE_POS 0.35f
#define PLAYER_LIST_ARROW_POS 0.42f

#define PLAY_TEXT_HEIGHT -0.1f
#define CUP_TEXT_HEIGHT 0.0f
#define HELP_TEXT_HEIGHT 0.1f
#define QUIT_TEXT_HEIGHT 0.2f

#define HELP_LEFT -0.75f

#define NEW_CUP_TEXT_HEIGHT -0.1f
#define LOAD_CUP_TEXT_HEIGHT 0.0f
#define SELECTION_CUP_ALT_1_HEIGHT -0.15f
#define SELECTION_CUP_LEFT -0.3f
#define SELECTION_CUP_ARROW_LEFT 0.2f
#define SELECTION_CUP_MENU_OFFSET 0.1f

#define SELECTION_TEXT_LEFT -0.35f
#define SELECTION_TEXT_RIGHT 0.1f
#define SELECTION_ALT_1_HEIGHT -0.1f
#define SELECTION_ALT_OFFSET 0.06f
#define SELECTION_TEAM_TEXT_HEIGHT -0.2f
#define PLAYER_LIST_TEAM_TEXT_HEIGHT -0.4f
#define PLAYER_LIST_TEAM_TEXT_POS -0.1f
#define PLAYER_LIST_INFO_HEIGHT -0.3f
#define PLAYER_LIST_INFO_OFFSET 0.22f
#define PLAYER_LIST_INFO_NAME_OFFSET 0.1f
#define PLAYER_LIST_INFO_FIRST -0.4f
#define PLAYER_LIST_FIRST_PLAYER_HEIGHT -0.2f
#define PLAYER_LIST_PLAYER_OFFSET 0.05f
#define PLAYER_LIST_NUMBER_OFFSET 0.1f
#define PLAYER_LIST_CONTINUE_HEIGHT 0.45f

#define DEFAULT_CONTROLLED_1 0
#define DEFAULT_CONTROLLED_2 2
#define DEFAULT_TEAM_1 0
#define DEFAULT_TEAM_2 1

#define BAT_DEFAULT_HEIGHT -1.0f
#define LEFT_HAND_DEFAULT_HEIGHT 0.1f
#define RIGHT_HAND_DEFAULT_HEIGHT 0.22f
#define SCALE_FACTOR 0.005f
#define POSITION_SCALE_ADDITION 0.00065f
#define BAT_MOVING_SPEED 0.0025f
#define BAT_DROP_MOVING_SPEED 0.03f
#define SCALE_LIMIT 30
#define HAND_WIDTH 0.12f
#define CAM_HEIGHT 2.3f
#define BAT_HEIGHT_CONSTANT 0.58f
#define MOVING_AWAY_SPEED 0.002f
#define HUTUNKEITTO_TEAM_TEXT_HEIGHT 0.45f
#define HUTUNKEITTO_TEAM_1_TEXT_POSITION 0.2f
#define HUTUNKEITTO_TEAM_2_TEXT_POSITION 0.55f

#define SLOT_COUNT 14

#if defined(__wii__)

extern Mtx view;
static Mtx model, modelview, rot;

extern guVector lightPos;
extern guVector light;
extern GXLightObj lo;

static MeshObject* planeMesh;
static void *planeDisplayList;
static u32 planeListSize;

static MeshObject* batMesh;
static void *batDisplayList;
static u32 batListSize;

static MeshObject* handMesh;
static void *handDisplayList;
static u32 handListSize;

static GXTexObj arrowTexture;
static TPLFile arrowTPL;

static GXTexObj catcherTexture;
static TPLFile catcherTPL;

static GXTexObj batterTexture;
static TPLFile batterTPL;

static GXTexObj team1Texture;
static TPLFile team1TPL;

static GXTexObj team2Texture;
static TPLFile team2TPL;

static GXTexObj team3Texture;
static TPLFile team3TPL;

static GXTexObj team4Texture;
static TPLFile team4TPL;

static GXTexObj team5Texture;
static TPLFile team5TPL;

static GXTexObj team6Texture;
static TPLFile team6TPL;

static GXTexObj team7Texture;
static TPLFile team7TPL;

static GXTexObj team8Texture;
static TPLFile team8TPL;

static GXTexObj slotTexture;
static TPLFile slotTPL;

static GXTexObj trophyTexture;
static TPLFile trophyTPL;

static guVector cam, look, up;

static guVector rotXAxis = {1,0,0};
static guVector rotZAxis = {0,0,1};
#else

static GLuint arrowTexture;
static GLuint catcherTexture;
static GLuint batterTexture;
static GLuint slotTexture;
static GLuint trophyTexture;
static GLuint team1Texture;
static GLuint team2Texture;
static GLuint team3Texture;
static GLuint team4Texture;
static GLuint team5Texture;
static GLuint team6Texture;
static GLuint team7Texture;
static GLuint team8Texture;

static MeshObject* planeMesh;
static GLuint planeDisplayList;

static MeshObject* handMesh;
static GLuint handDisplayList;

static MeshObject* batMesh;
static GLuint batDisplayList;

static Vector3D cam, look, up;
extern float lightPos[4];
#endif

static void loadMenuScreenSettings();
static void drawFront();
static void drawSelection();
static void drawGameOverTexts();
static void drawPlayerList();
static void drawHutunkeitto();
static void drawCup();
static void drawHelp();
static void moveToGame();
static void drawLoadingTexts();
static void initHutunkeitto();

extern StateInfo stateInfo;

static MenuInfo menuInfo;
static CupInfo cupInfo;

static int cupGame;
static int stage; // 0 front, 1 team selection, 2 player order for team 1, 3 player order for team 2
static int pointer; 
static int rem;
static int mark;
static int stage_1_state;
static int stage_4_state;
static int stage_8_state;
static int stage_9_state;
static int stage_8_state_1_level;
static int batTimer;
static int batTimerLimit;
static int batTimerCount;
static int updatingCanStart; 
static int team1;
static int team2;
static int team1_control;
static int team2_control;
static int inningsInPeriod;
static int team1_batting_order[PLAYERS_IN_TEAM + JOKER_COUNT];
static int team2_batting_order[PLAYERS_IN_TEAM + JOKER_COUNT];
static int batting_order[PLAYERS_IN_TEAM + JOKER_COUNT];

static int team_1_choices[2][5];
static int team_2_choices[2][5];
static int choiceCounter;
static int choiceCount;

static int leftReady;
static int rightReady;
static int turnCount;
static float batHeight;
static float batPosition;
static float leftHandHeight;
static float leftHandPosition;
static float rightHandHeight;
static float rightHandPosition;
static float tempLeftHeight;
static float handsZ;
static float refereeHandHeight;
static int leftScaleCount;
static int rightScaleCount;
static int playsFirst;

typedef struct _TreeCoordinates
{
	float x;
	float y;
} TreeCoordinates;



static TreeCoordinates treeCoordinates[SLOT_COUNT];
static int teamSelection;

static CupInfo saveData[5];

/*
	loads data from data-file and stores it to saveData-structure.
	return 0 if everything went fine, 1 otherwise
*/
static int refreshLoadCups()
{
    FILE *file;
	char *content = (char*)malloc(10000 * sizeof(char));
	char *p = content;
	int counter = 0;
	int ok = 0;
	int slot = 0;
	int i = 0;
	int j;
	int valid = 1;
   
    /* open the file */
    file = fopen("saves.dat", "r");
    if (file == NULL) {
        printf("I couldn't open saves.dat for reading.\n");
		free(content);
        return 1;
    }
    // read file to char array
	do 
	{
		i = fgetc(file);
		*p = (char)i;
		if(*p == '*') ok = 1;
		p++;
	} while(i != EOF);
	*p = '\0';
   
    /* close the file */
    fclose(file);
	// if we found correct type of end of file
	if(ok == 1)
	{
		while(content[counter] != '*')
		{
			if(content[counter] == 'd')
			{
				int i = content[counter + 2] - '0';
				char index[3] = "00";
				saveData[slot].inningCount = i;
				i = content[counter + 4] - '0';
				saveData[slot].gameStructure = i;
				index[0] = content[counter + 6];
				index[1] = content[counter + 7];
				saveData[slot].userTeamIndexInTree = atoi(index);
				i = content[counter + 9] - '0';
				saveData[slot].dayCount = i;
			}
			else if(content[counter] == 'i')
			{
				int i;
				for(i = 0; i < SLOT_COUNT; i++)
				{
					char str[3] = "  "; 
					int index;
					str[1] = content[counter + i*3 + 3];
					str[0] = content[counter + i*3 + 3 - 1];
					index = atoi(str);
					saveData[slot].cupTeamIndexTree[i] = index;
				}
			}
			else if(content[counter] == 'w')
			{
				int i;
				for(i = 0; i < SLOT_COUNT; i++)
				{
					int wins = content[counter + i*2 + 2] - '0';
					saveData[slot].slotWins[i] = wins;
				}
				slot++;
			}
			else if(content[counter] == '^')
			{
				saveData[slot].userTeamIndexInTree = -1;
				slot++;
			}

			counter++;
		}
	}
	else
	{
		free(content);
		printf("Something wrong with the save file");
		return 1;
	}
	free(content);

	// go through the saveData-structure and figure out if its good.
	for(i = 0; i < 5; i++)
	{
		if(saveData[i].userTeamIndexInTree != -1)
		{
			if(saveData[i].dayCount < 0) valid = 0;
			if(saveData[i].gameStructure != 0 && saveData[i].gameStructure != 1) valid = 0;
			if(saveData[i].inningCount != 2 && saveData[i].inningCount != 4 && saveData[i].inningCount != 8) valid = 0;
			if(saveData[i].winnerIndex >= TEAM_COUNT) valid = 0;
			if(saveData[i].userTeamIndexInTree >= 14) valid = 0;
			for(j = 0; j < SLOT_COUNT; j++)
			{
				if(saveData[i].slotWins[j] < 0 || saveData[i].slotWins[j] > 3) valid = 0;
				if(saveData[i].cupTeamIndexTree[j] > TEAM_COUNT) valid = 0;
			}
		}
	}
	if(valid == 0)
	{
		printf("Something wrong with the save file.");
		return 1;
	}
	return 0;
}
/*
	stores information in cupInfo-structure to saveData-structure's specified slot and 
	then writes saveData-info to data-file.
*/
static void saveCup(int slot)
{
	FILE *fp;
	char* data = (char*)malloc(10000 * sizeof(char));
	int i;
	int counter;
	CupInfo* saveDataPtr;
   
	fp = fopen("saves.dat", "w");
	if (fp == NULL) {
		printf("I couldn't open saves.dat for writing.\n");
		return;
	}
	counter = 0;
	for(i = 0; i < 5; i++)
	{
		int j;
		if(i != slot)
		{
			saveDataPtr = &saveData[i];
		}
		else
		{
			saveDataPtr = &cupInfo;
		}
		if(saveDataPtr->userTeamIndexInTree != -1)
		{
			data[counter] = 'd'; counter++;
			data[counter] = ' '; counter++;
			data[counter] = (char)(((int)'0')+(saveDataPtr->inningCount)); counter++;
			data[counter] = ' '; counter++;
			data[counter] = (char)(((int)'0')+(saveDataPtr->gameStructure)); counter++;
			data[counter] = ' '; counter++;
			data[counter] = (char)(((int)'0')+(saveDataPtr->userTeamIndexInTree)/10); counter++;
			data[counter] = (char)(((int)'0')+(saveDataPtr->userTeamIndexInTree)%10); counter++;
			data[counter] = ' '; counter++;
			data[counter] = (char)(((int)'0')+(saveDataPtr->dayCount)); counter++;
			data[counter] = '\n'; counter++;
			data[counter] = 'i'; counter++;
			for(j = 0; j < SLOT_COUNT; j++)
			{
				int index = saveDataPtr->cupTeamIndexTree[j];
				char first;
				char second;
				if(index < 0)
				{
					first = '-';
					second = (char)(((int)'0')+abs(index));
				}
				else
				{
					first = ' ';
					second = (char)(((int)'0')+index);
				}
				data[counter] = ' '; counter++;
				data[counter] = first; counter++;
				data[counter] = second; counter++;
			}
			data[counter] = '\n'; counter++;
			data[counter] = 'w'; counter++;
			for(j = 0; j < SLOT_COUNT; j++)
			{
				int wins = saveDataPtr->slotWins[j];
				data[counter] = ' '; counter++;
				data[counter] = (char)(((int)'0')+wins); counter++;
			}
			data[counter] = '\n'; counter++;
		}
		else
		{
			data[counter] = '^'; counter++;
			data[counter] = '\n'; counter++;
		}
	}
	data[counter] = '*'; counter++;
	data[counter] = '\0';
	fputs(data, fp);
	fclose(fp);
	free(data);

	if(refreshLoadCups() != 0)
	{
		printf("Something wrong with the save file.");
	}
}

static void updateSchedule()
{
	int j;
	int counter = 0;
	for(j = 0; j < SLOT_COUNT/2; j++)
	{
		if(stateInfo.cupInfo->gameStructure == 0)
		{
			if(stateInfo.cupInfo->slotWins[j*2] < 3 && stateInfo.cupInfo->slotWins[j*2+1] < 3)
			{
				if(j < 4 || (j < 6 && stateInfo.cupInfo->dayCount >= 5) || 
					(j == 6 && stateInfo.cupInfo->dayCount >= 10))
				{
					stateInfo.cupInfo->schedule[counter][0] = j*2;
					stateInfo.cupInfo->schedule[counter][1] = j*2+1;
					counter++;
				}
			}
		}
		else
		{
			if(stateInfo.cupInfo->slotWins[j*2] < 1 && stateInfo.cupInfo->slotWins[j*2+1] < 1)
			{
				if(j < 4 || (j < 6 && stateInfo.cupInfo->dayCount >= 1) || 
					(j == 6 && stateInfo.cupInfo->dayCount >= 2))
				{
					stateInfo.cupInfo->schedule[counter][0] = j*2;
					stateInfo.cupInfo->schedule[counter][1] = j*2+1;
					counter++;
				}
			}
		}
	}
	for(j = counter; j < 4; j++)
	{
		stateInfo.cupInfo->schedule[j][0] = -1;
		stateInfo.cupInfo->schedule[j][1] = -1;
	}
}

/*
	loads data from saveData-structure to cupInfo-structure
*/
static void loadCup(int slot)
{
	int i;
	stateInfo.cupInfo->inningCount = saveData[slot].inningCount;
	stateInfo.cupInfo->gameStructure = saveData[slot].gameStructure;
	stateInfo.cupInfo->userTeamIndexInTree = saveData[slot].userTeamIndexInTree;
	stateInfo.cupInfo->dayCount = saveData[slot].dayCount;

	for(i = 0; i < SLOT_COUNT; i++)
	{
		stateInfo.cupInfo->cupTeamIndexTree[i] = saveData[slot].cupTeamIndexTree[i];
		stateInfo.cupInfo->slotWins[i] = saveData[slot].slotWins[i];
	}
	stateInfo.cupInfo->winnerIndex = -1;
	if(stateInfo.cupInfo->gameStructure == 1)
	{
		if(stateInfo.cupInfo->slotWins[12] == 1)
		{
			stateInfo.cupInfo->winnerIndex = stateInfo.cupInfo->cupTeamIndexTree[12];
		}
		else if(stateInfo.cupInfo->slotWins[13] == 1)
		{
			stateInfo.cupInfo->winnerIndex = stateInfo.cupInfo->cupTeamIndexTree[13];
		}		
	}
	else if(stateInfo.cupInfo->gameStructure == 0)
	{
		if(stateInfo.cupInfo->slotWins[12] == 3)
		{
			stateInfo.cupInfo->winnerIndex = stateInfo.cupInfo->cupTeamIndexTree[12];
		}
		else if(stateInfo.cupInfo->slotWins[13] == 3)
		{
			stateInfo.cupInfo->winnerIndex = stateInfo.cupInfo->cupTeamIndexTree[13];
		}		
	}

	// here we should fill the schedule array
	updateSchedule();
}

static void updateCupTreeAfterDay(int scheduleSlot, int winningSlot)
{
	int i;
	int counter = 0;
	int done = 0;
	while(done == 0 && counter < 4)
	{
		if(stateInfo.cupInfo->schedule[counter][0] != -1)
		{
			int team1Index = stateInfo.cupInfo->cupTeamIndexTree[(stateInfo.cupInfo->schedule[counter][0])];
			int team2Index = stateInfo.cupInfo->cupTeamIndexTree[(stateInfo.cupInfo->schedule[counter][1])];
			int index = 8 + stateInfo.cupInfo->schedule[counter][0] / 2;
			int team1Points = 0;
			int team2Points = 0;
			int random = rand()%100;
			int difference;
			int winningTeam;
			for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++)
			{
				team1Points += stateInfo.teamData[team1Index].players[i].speed;
				team1Points += stateInfo.teamData[team1Index].players[i].power;
				team2Points += stateInfo.teamData[team2Index].players[i].speed;
				team2Points += stateInfo.teamData[team2Index].players[i].power;
			}
			difference = team2Points - team1Points;
			// update schedule and cup tree
			if(counter != scheduleSlot)
			{
				if(random + difference*3 >= 50)
				{
					winningTeam = 1;
				}
				else
				{
					winningTeam = 0;
				}
			}
			else
			{
				winningTeam = winningSlot;
			}
			stateInfo.cupInfo->slotWins[stateInfo.cupInfo->schedule[counter][winningTeam]] += 1;
			if(stateInfo.cupInfo->gameStructure == 0)
			{
				if(stateInfo.cupInfo->slotWins[stateInfo.cupInfo->schedule[counter][winningTeam]] == 3)
				{
					if(index < 14)
					{
						stateInfo.cupInfo->cupTeamIndexTree[index] =
							stateInfo.cupInfo->cupTeamIndexTree[stateInfo.cupInfo->schedule[counter][winningTeam]];
						if(stateInfo.cupInfo->schedule[counter][winningTeam] == stateInfo.cupInfo->userTeamIndexInTree)
						{
							stateInfo.cupInfo->userTeamIndexInTree = index;
						}
					}
					else
					{
						stateInfo.cupInfo->winnerIndex = stateInfo.cupInfo->cupTeamIndexTree[stateInfo.cupInfo->schedule[counter][winningTeam]];
					}
				}
			}
			else
			{
				if(stateInfo.cupInfo->slotWins[stateInfo.cupInfo->schedule[counter][winningTeam]] == 1)
				{
					if(index < 14)
					{
						stateInfo.cupInfo->cupTeamIndexTree[index] =
							stateInfo.cupInfo->cupTeamIndexTree[stateInfo.cupInfo->schedule[counter][winningTeam]];
						if(stateInfo.cupInfo->schedule[counter][winningTeam] == stateInfo.cupInfo->userTeamIndexInTree)
						{
							stateInfo.cupInfo->userTeamIndexInTree = index;
						}
					}
					else
					{
						stateInfo.cupInfo->winnerIndex = stateInfo.cupInfo->cupTeamIndexTree[stateInfo.cupInfo->schedule[counter][winningTeam]];
					}
				}
			}			
			counter++;
		}
		else
		{
			done = 1;
		}
	}
}
#endif /* MAIN_MENU_INTERNAL_H */