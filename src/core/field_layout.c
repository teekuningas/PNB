#include "field_layout.h"

void field_init_positions(FieldPositions* positions)
{
	// y-coordinate here is adjusted to be height of a ball when player has it, just because thats basically only way it is used.
	// some of the values are hard coded, which is fine. cannot expect to calculate all players' positions relative to something.
	
	positions->pitchPlate.x = 0.0f;
	positions->pitchPlate.y = BALL_HEIGHT_WITH_PLAYER;
	positions->pitchPlate.z = 0.0f;
	
	positions->pitcher.x = 1.5f;
	positions->pitcher.y = BALL_HEIGHT_WITH_PLAYER;
	positions->pitcher.z = 0.0f;
	
	positions->firstBaseRun.x = -20.0f;
	positions->firstBaseRun.y = BALL_HEIGHT_WITH_PLAYER;
	positions->firstBaseRun.z = -23.5f;
	
	positions->firstBase.x = positions->firstBaseRun.x;
	positions->firstBase.y = BALL_HEIGHT_WITH_PLAYER;
	positions->firstBase.z = positions->firstBaseRun.z + 0.5f;
	
	positions->secondBaseRun.x = +30.0f;
	positions->secondBaseRun.y = BALL_HEIGHT_WITH_PLAYER;
	positions->secondBaseRun.z = -43.8f;
	
	positions->secondBase.x = positions->secondBaseRun.x;
	positions->secondBase.y = BALL_HEIGHT_WITH_PLAYER;
	positions->secondBase.z = positions->secondBaseRun.z + 1.0f;
	
	positions->thirdBaseRun.x = -30.0f;
	positions->thirdBaseRun.y = BALL_HEIGHT_WITH_PLAYER;
	positions->thirdBaseRun.z = -43.8f;
	
	positions->thirdBase.x = positions->thirdBaseRun.x;
	positions->thirdBase.y = BALL_HEIGHT_WITH_PLAYER;
	positions->thirdBase.z = positions->thirdBaseRun.z + 1.0f;
	
	positions->leftPoint.x = -31.2f;
	positions->leftPoint.y = BALL_HEIGHT_WITH_PLAYER;
	positions->leftPoint.z = -33.0f;
	
	positions->runLeftPoint.x = positions->leftPoint.x;
	positions->runLeftPoint.y = BALL_HEIGHT_WITH_PLAYER;
	positions->runLeftPoint.z = -25.0f;
	
	positions->backLeftPoint.x = positions->leftPoint.x;
	positions->backLeftPoint.y = BALL_HEIGHT_WITH_PLAYER;
	positions->backLeftPoint.z = -83.8f;
	
	positions->rightPoint.x = 31.5f;
	positions->rightPoint.y = BALL_HEIGHT_WITH_PLAYER;
	positions->rightPoint.z = -32.0f;
	
	positions->backRightPoint.x = positions->rightPoint.x;
	positions->backRightPoint.y = BALL_HEIGHT_WITH_PLAYER;
	positions->backRightPoint.z = positions->backLeftPoint.z;
	
	positions->bottomRightCatcher.x = positions->secondBase.x - 21.0f;
	positions->bottomRightCatcher.y = BALL_HEIGHT_WITH_PLAYER;
	positions->bottomRightCatcher.z = positions->secondBase.z + 22.0f;
	
	positions->middleLeftCatcher.x = positions->thirdBase.x + 18.0f;
	positions->middleLeftCatcher.y = BALL_HEIGHT_WITH_PLAYER;
	positions->middleLeftCatcher.z = positions->thirdBase.z + 6.0f;
	
	positions->middleRightCatcher.x = -positions->middleLeftCatcher.x - 9.0f;
	positions->middleRightCatcher.y = BALL_HEIGHT_WITH_PLAYER;
	positions->middleRightCatcher.z = positions->secondBase.z - 6.0f;
	
	positions->backLeftCatcher.x = positions->backLeftPoint.x + 20.0f;
	positions->backLeftCatcher.y = BALL_HEIGHT_WITH_PLAYER;
	positions->backLeftCatcher.z = positions->backLeftPoint.z + 10.0f;
	
	positions->backRightCatcher.x = -positions->backLeftCatcher.x;
	positions->backRightCatcher.y = BALL_HEIGHT_WITH_PLAYER;
	positions->backRightCatcher.z = positions->backLeftCatcher.z;
	
	positions->homeRunPoint.x = positions->pitchPlate.x - HOME_RADIUS;
	positions->homeRunPoint.y = BALL_HEIGHT_WITH_PLAYER;
	positions->homeRunPoint.z = HOME_LINE_Z;
}
