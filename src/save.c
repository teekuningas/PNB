#include <stdio.h>
#include <stdlib.h>

#include "globals.h"

#ifdef _WIN32
#include <windows.h>
#define PATH_MAX MAX_PATH
#define SEPARATOR "\\"
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <errno.h>
#define SEPARATOR "/"
#endif

static int getSavePath(char* result);
static void initializeWithoutFile(CupInfo* saveData, int numSlots);

int readSaveData(CupInfo* saveData, int numSlots)
{

	FILE *file;
	char savePath[PATH_MAX];
	getSavePath(savePath);

	int counter = 0;
	int ok = 0;
	int slot = 0;
	int i = 0;

	char *content;
	char *p;

	/* open the file */
	file = fopen(savePath, "r");
	if (file == NULL) {
		printf("I couldn't open saves.dat for reading.\n");
		initializeWithoutFile(saveData, numSlots);
		return 0;
	}

	// Allocate enough
	content = (char*)malloc(10000 * sizeof(char));
	p = content;

	// read file to char array
	do {
		i = fgetc(file);
		*p = (char)i;
		if(*p == '*') ok = 1;
		p++;
	} while(i != EOF);
	*p = '\0';

	/* close the file */
	fclose(file);

	// if we found correct type of end of file
	if(ok == 1) {
		while(content[counter] != '*') {
			if(content[counter] == 'd') {
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
			} else if(content[counter] == 'i') {
				int i;
				for(i = 0; i < SLOT_COUNT; i++) {
					char str[3] = "  ";
					int index;
					str[1] = content[counter + i*3 + 3];
					str[0] = content[counter + i*3 + 3 - 1];
					index = atoi(str);
					saveData[slot].cupTeamIndexTree[i] = index;
				}
			} else if(content[counter] == 'w') {
				int i;
				for(i = 0; i < SLOT_COUNT; i++) {
					int wins = content[counter + i*2 + 2] - '0';
					saveData[slot].slotWins[i] = wins;
				}
				slot++;
			} else if(content[counter] == '^') {
				saveData[slot].userTeamIndexInTree = -1;
				slot++;
			}
			counter++;
		}
	} else {
		free(content);
		printf("Something wrong with the save file.\n");
		return 1;
	}
	free(content);
	return 0;
}

int writeSaveData(CupInfo* saveData, CupInfo* cupInfo, int currentSlot, int numSlots)
{

	FILE *fp;

	char savePath[PATH_MAX];
	getSavePath(savePath);

	char* data = (char*)malloc(10000 * sizeof(char));

	int i;
	int counter;
	CupInfo* saveDataPtr;

	fp = fopen(savePath, "w");
	if (fp == NULL) {
		printf("I couldn't open saves.dat for writing.\n");
		return 1;
	}

	counter = 0;
	for(i = 0; i < numSlots; i++) {
		int j;
		if(i != currentSlot) {
			saveDataPtr = &saveData[i];
		} else {
			saveDataPtr = cupInfo;
		}
		if(saveDataPtr->userTeamIndexInTree != -1) {
			data[counter] = 'd';
			counter++;
			data[counter] = ' ';
			counter++;
			data[counter] = (char)(((int)'0')+(saveDataPtr->inningCount));
			counter++;
			data[counter] = ' ';
			counter++;
			data[counter] = (char)(((int)'0')+(saveDataPtr->gameStructure));
			counter++;
			data[counter] = ' ';
			counter++;
			data[counter] = (char)(((int)'0')+(saveDataPtr->userTeamIndexInTree)/10);
			counter++;
			data[counter] = (char)(((int)'0')+(saveDataPtr->userTeamIndexInTree)%10);
			counter++;
			data[counter] = ' ';
			counter++;
			data[counter] = (char)(((int)'0')+(saveDataPtr->dayCount));
			counter++;
			data[counter] = '\n';
			counter++;
			data[counter] = 'i';
			counter++;
			for(j = 0; j < SLOT_COUNT; j++) {
				int index = saveDataPtr->cupTeamIndexTree[j];
				char first;
				char second;
				if(index < 0) {
					first = '-';
					second = (char)(((int)'0')+abs(index));
				} else {
					first = ' ';
					second = (char)(((int)'0')+index);
				}
				data[counter] = ' ';
				counter++;
				data[counter] = first;
				counter++;
				data[counter] = second;
				counter++;
			}
			data[counter] = '\n';
			counter++;
			data[counter] = 'w';
			counter++;
			for(j = 0; j < SLOT_COUNT; j++) {
				int wins = saveDataPtr->slotWins[j];
				data[counter] = ' ';
				counter++;
				data[counter] = (char)(((int)'0')+wins);
				counter++;
			}
			data[counter] = '\n';
			counter++;
		} else {
			data[counter] = '^';
			counter++;
			data[counter] = '\n';
			counter++;
		}
	}
	data[counter] = '*';
	counter++;
	data[counter] = '\0';
	fputs(data, fp);
	fclose(fp);
	free(data);

	return 0;
}

static void initializeWithoutFile(CupInfo* saveData, int numSlots)
{
	int i;
	for(i = 0; i < numSlots; i++) {
		saveData[i].userTeamIndexInTree = -1;
	}
}

static int getSavePath(char* result)
{
	char* homedir;
	char dirPath[PATH_MAX];
	// Add enough extra to contain saves.dat and separators and suppress warnings.
	char filePath[PATH_MAX + 20];

#ifdef _WIN32
	homedir = getenv("USERPROFILE");
	snprintf(dirPath, sizeof(dirPath), "%s%sAppData%sLocal%sPNB", homedir, SEPARATOR, SEPARATOR, SEPARATOR);
	if (!CreateDirectory(dirPath, NULL)) {
		DWORD err = GetLastError();
		if (err != ERROR_ALREADY_EXISTS) {
			fprintf(stderr, "Failed to create directory: %u\n", err);
			return 1;
		}
	}
	snprintf(filePath, sizeof(filePath), "%s%ssaves.dat", dirPath, SEPARATOR);
#else
	homedir = getenv("HOME");
	snprintf(dirPath, PATH_MAX, "%s%s.pnb", homedir, SEPARATOR);
	if (mkdir(dirPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
		if (errno != EEXIST) {
			perror("Failed to create directory");
			return 1;
		}
	}
	snprintf(filePath, sizeof(filePath), "%s%ssaves.dat", dirPath, SEPARATOR);
#endif

	strcpy(result, filePath);
	return 0;
}
