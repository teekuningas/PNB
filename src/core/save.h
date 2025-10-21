#ifndef SAVE_H
#define SAVE_H

#include "globals.h"

int readSaveData(CupInfo* saveData, int numSlots);
int writeSaveData(CupInfo* saveData, CupInfo* cupInfo, int currentCup, int numSlots);

#endif /* SAVE_H */
