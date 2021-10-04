#include "globals.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "simplexml.h"
#include "fill_player_data.h"

/*
	whole purpose of this file is just to provide means to fill the data structure in one place and not mixed up in other code.
*/

static TeamData* teamDataPtr;
static int teamCounter = 0;
static int playerCounter = 0;

void* handler (SimpleXmlParser parser, SimpleXmlEvent event, 
	const char* szName, const char* szAttribute, const char* szValue);
void parse (char* sData, long nDataLen);
	
void trim (const char* szInput, char* szOutput);
char* getReadFileDataErrorDescription (int nError);
int readFileData (char* sFileName, char** sData, long *pnDataLen);

void* handler (SimpleXmlParser parser, SimpleXmlEvent event, 
	const char* szName, const char* szAttribute, const char* szValue)
{
	
	char szHandlerName[32], szHandlerAttribute[32], szHandlerValue[32];
	if (szName != NULL) {
		trim(szName, szHandlerName);
	}

	if (szAttribute != NULL) {
		trim(szAttribute, szHandlerAttribute);
	}

	if (szValue != NULL) {
		trim(szValue, szHandlerValue);
	}

	if (event == ADD_ATTRIBUTE) {
		if(szHandlerValue[0] == 't')
		{
			teamDataPtr[teamCounter].id = ownstrdup(szHandlerValue);
			teamCounter++;
			playerCounter = 0;
		}
		else if(szHandlerValue[0] == 'p')
		{
			teamDataPtr[teamCounter-1].players[playerCounter].id = ownstrdup(szHandlerValue);
			playerCounter++;
		}
		
	} else if (event == ADD_CONTENT) {	

		if(strcmp(szHandlerName, "player") == 0)
		{
			
		}
		else if(strcmp(szHandlerName, "players") == 0 || strcmp(szHandlerName, "teams") == 0
			|| strcmp(szHandlerName, "team") == 0)
		{
		}
		else
		{
	
			if(strcmp(szHandlerName, "name") == 0)
			{
				teamDataPtr[teamCounter-1].players[playerCounter-1].name = ownstrdup(szHandlerValue);
			}
			else if(strcmp(szHandlerName, "team_name") == 0)
			{

				teamDataPtr[teamCounter-1].name = ownstrdup(szHandlerValue);
			}
			else if(strcmp(szHandlerName, "speed") == 0)
			{
				int i = atoi (szHandlerValue);
				teamDataPtr[teamCounter-1].players[playerCounter-1].speed = i;
			}
			else if(strcmp(szHandlerName, "power") == 0)
			{
				int i = atoi (szHandlerValue);
				teamDataPtr[teamCounter-1].players[playerCounter-1].power = i;
			}
		}
		
	}
	
	return handler;
}

void parse (char* sData, long nDataLen) {
	SimpleXmlParser parser= simpleXmlCreateParser(sData, nDataLen);
	if (parser == NULL) {
		fprintf(stderr, "couldn't create parser");
		return;
	}
	
	if (simpleXmlParse(parser, handler) != 0) {
		fprintf(stderr, "parse error on line %li:\n%s\n", 
			simpleXmlGetLineNumber(parser), simpleXmlGetErrorDescription(parser));
	}
	
}

void trim (const char* szInput, char* szOutput) {
	int i= 0;
	while (szInput[i] != 0 && i < 32) {
		if (szInput[i] < ' ') {
			szOutput[i]= ' ';
		} else {
			szOutput[i]= szInput[i];
		}
		i++;
	}
	if (i < 32) {
		szOutput[i]= '\0';
	} else {
		szOutput[28]= '.';
		szOutput[29]= '.';
		szOutput[30]= '.';
		szOutput[31]= '\0';
	}
}


#define READ_FILE_NO_ERROR 0
#define READ_FILE_STAT_ERROR 1
#define READ_FILE_OPEN_ERROR 2
#define READ_FILE_OUT_OF_MEMORY 3
#define READ_FILE_READ_ERROR 4

int readFileData (char* sFileName, char** psData, long *pnDataLen) {
	struct stat fstat;
	*psData= NULL;
	*pnDataLen= 0;
	if (stat(sFileName, &fstat) == -1) {
		return READ_FILE_STAT_ERROR;
	} else {
		FILE *file= fopen(sFileName, "rb");
		if (file == NULL) {
			return READ_FILE_OPEN_ERROR;
		} else {
			*psData= (char*)malloc(fstat.st_size);
			if (*psData == NULL) {
				return READ_FILE_OUT_OF_MEMORY;
			} else {
				size_t len= fread(*psData, 1, fstat.st_size, file);
				fclose(file);
				if (len != fstat.st_size) {
					free(*psData);
					*psData= NULL;
					return READ_FILE_READ_ERROR;
				}
				*pnDataLen= len;
				return READ_FILE_NO_ERROR;
			}
		}
	}	
}

char* getReadFileDataErrorDescription (int nError) {
	switch (nError) {
		case READ_FILE_NO_ERROR: return "no error";
		case READ_FILE_STAT_ERROR: return "no such file";
		case READ_FILE_OPEN_ERROR: return "couldn't open file";
		case READ_FILE_OUT_OF_MEMORY: return "out of memory";
		case READ_FILE_READ_ERROR: return "couldn't read file";
	}
	return "unknown error";
}

int fillPlayerData(TeamData* teamData)
{
	char* name = "teams.xml";
	char* sData;
	long nDataLen;
	int nResult;
	int i, j;
	int valid = 1;
	teamDataPtr = teamData;
	nResult= readFileData(name, &sData, &nDataLen);
	if (nResult != 0) {
		fprintf(stderr, "couldn't read %s (%s).\n", name, 
			getReadFileDataErrorDescription(nResult));
		return -1;
	} 
	parse(sData, nDataLen);
	free(sData);
	
	// check if contents are "valid"
	for(i = 0; i < TEAM_COUNT; i++)
	{
		for(j = 0; j < PLAYERS_IN_TEAM + JOKER_COUNT; j++)
		{
			if(!(teamData[i].players[j].power >= 1 && teamData[i].players[j].power <= 5 &&
				teamData[i].players[j].speed >= 1 && teamData[i].players[j].speed <= 5))
			{
				valid = 0;
			}
		}
	}
	if(valid == 0)
	{
		printf("Invalid player data\n");
		return -1;
	}
	
	return 0;	
}

int cleanPlayerData()
{
	int i, j;
	// clean up
	for(i = 0; i < TEAM_COUNT; i++)
	{
		free(teamDataPtr[i].id);
		free(teamDataPtr[i].name);
		for(j = 0; j < PLAYERS_IN_TEAM + JOKER_COUNT; j++)
		{
			free(teamDataPtr[i].players[j].id);
			free(teamDataPtr[i].players[j].name);
		}
	}
	
	return 0;
}
