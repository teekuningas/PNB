#include "globals.h"
#include "render.h"

#include "player.h"
#include "player_internal.h"

/*
	here we are just gonna render our players. draw function is the key part, it will take all the information about players and use that to draw 
	them. it means that all the data about how the players look like in the scene will be in the playerInfo-structure.
*/

int initPlayer()
{
	#if defined(__wii__)
	int result;
	#endif
	// here as we can see, there are a lot of .obj files. Not particularly efficient to do animations this way
	// but i guess its ok. it shouldnt be too slow.
	char bare_hands_walking_string[68] = "data/player_bare_hands_walking/player_bare_hands_walking_000001.obj";
	char bare_hands_running_string[68] = "data/player_bare_hands_running/player_bare_hands_running_000001.obj";
	char without_ball_walking_string[84] = "data/player_glove_without_ball_walking/player_glove_without_ball_walking_000001.obj";
	char with_ball_walking_string[78] = "data/player_glove_with_ball_walking/player_glove_with_ball_walking_000001.obj";
	char without_ball_running_string[84] = "data/player_glove_without_ball_running/player_glove_without_ball_running_000001.obj";
	char with_ball_running_string[78] = "data/player_glove_with_ball_running/player_glove_with_ball_running_000001.obj";
	char pitch_down_string[33] = "data/pitch/pitch_down_000001.obj";
	char pitch_up_string[31] = "data/pitch/pitch_up_000001.obj";
	char throw_load_string[33] = "data/throw/throw_load_000001.obj";
	char throw_release_string[36] = "data/throw/throw_release_000001.obj";
	char swing_string[30] = "data/batting/swing_000001.obj";
	char bunt_string[29] = "data/batting/bunt_000001.obj";
	char batting_stop_string[37] = "data/batting/batting_stop_000001.obj";
	int i;	
	// 8, 16
	for(i = 0; i < 16; i++)
	{
		bare_hands_walking_string[62] = (char)(((int)'0')+(i+1)%10);
		bare_hands_walking_string[61] = (char)(((int)'0')+((i+1)/10));
		#if defined(__wii__)
		playerBareHandsWalkingMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGX(bare_hands_walking_string, "Sphere", 
			playerBareHandsWalkingMesh[i], &playerBareHandsWalkingListSize[i], &playerBareHandsWalkingDisplayList[i]) != 0) return -1;
		#else
		playerBareHandsWalkingMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGL(bare_hands_walking_string, "Sphere", playerBareHandsWalkingMesh[i], &(playerBareHandsWalkingDisplayList[i])) != 0) return -1;
		#endif
	}

	for(i = 0; i < 20; i++)
	{
		bare_hands_running_string[62] = (char)(((int)'0')+(i+1)%10);
		bare_hands_running_string[61] = (char)(((int)'0')+((i+1)/10));
		#if defined(__wii__)
		playerBareHandsRunningMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGX(bare_hands_running_string, "Sphere", 
			playerBareHandsRunningMesh[i], &playerBareHandsRunningListSize[i], &playerBareHandsRunningDisplayList[i]) != 0) return -1;
		#else
		playerBareHandsRunningMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGL(bare_hands_running_string, "Sphere", playerBareHandsRunningMesh[i], &playerBareHandsRunningDisplayList[i]) != 0) return -1; 
		#endif
	}

	for(i = 0; i < 16; i++)
	{
		without_ball_walking_string[78] = (char)(((int)'0')+(i+1)%10);
		without_ball_walking_string[77] = (char)(((int)'0')+((i+1)/10));
		#if defined(__wii__)
		playerGloveWithoutBallWalkingMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGX(without_ball_walking_string, "Cube", 
			playerGloveWithoutBallWalkingMesh[i], &playerGloveWithoutBallWalkingListSize[i], &playerGloveWithoutBallWalkingDisplayList[i]) != 0) return -1;
		#else
		playerGloveWithoutBallWalkingMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGL(without_ball_walking_string, "Cube", playerGloveWithoutBallWalkingMesh[i], &playerGloveWithoutBallWalkingDisplayList[i]) != 0) return -1; 
		#endif
	}

	for(i = 0; i < 16; i++)
	{
		with_ball_walking_string[72] = (char)(((int)'0')+(i+1)%10);
		with_ball_walking_string[71] = (char)(((int)'0')+((i+1)/10));
		#if defined(__wii__)
		playerGloveWithBallWalkingMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGX(with_ball_walking_string, "Cube", 
			playerGloveWithBallWalkingMesh[i], &playerGloveWithBallWalkingListSize[i], &playerGloveWithBallWalkingDisplayList[i]) != 0) return -1;
		#else
		playerGloveWithBallWalkingMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGL(with_ball_walking_string, "Cube", playerGloveWithBallWalkingMesh[i], &playerGloveWithBallWalkingDisplayList[i]) != 0) return -1; 
		#endif
	}

	for(i = 0; i < 20; i++)
	{
		without_ball_running_string[78] = (char)(((int)'0')+(i+1)%10);
		without_ball_running_string[77] = (char)(((int)'0')+((i+1)/10));
		#if defined(__wii__)
		playerGloveWithoutBallRunningMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGX(without_ball_running_string, "Cube", 
			playerGloveWithoutBallRunningMesh[i], &playerGloveWithoutBallRunningListSize[i], &playerGloveWithoutBallRunningDisplayList[i]) != 0) return -1;
		#else
		playerGloveWithoutBallRunningMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGL(without_ball_running_string, "Cube", playerGloveWithoutBallRunningMesh[i], &playerGloveWithoutBallRunningDisplayList[i]) != 0) return -1; 
		#endif
	}
	for(i = 0; i < 20; i++)
	{
		with_ball_running_string[72] = (char)(((int)'0')+(i+1)%10);
		with_ball_running_string[71] = (char)(((int)'0')+((i+1)/10));
		#if defined(__wii__)
		playerGloveWithBallRunningMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGX(with_ball_running_string, "Cube", 
			playerGloveWithBallRunningMesh[i], &playerGloveWithBallRunningListSize[i], &playerGloveWithBallRunningDisplayList[i]) != 0) return -1;
		#else
		playerGloveWithBallRunningMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGL(with_ball_running_string, "Cube", playerGloveWithBallRunningMesh[i], &playerGloveWithBallRunningDisplayList[i]) != 0) return -1; 
		#endif
	}
	for(i = 0; i < 9; i++)
	{

		pitch_down_string[27] = (char)(((int)'0')+(i+1)%10);
		pitch_down_string[26] = (char)(((int)'0')+((i+1)/10));
		#if defined(__wii__)
		pitchDownMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGX(pitch_down_string, "Cube", 
			pitchDownMesh[i], &pitchDownListSize[i], &pitchDownDisplayList[i]) != 0) return -1;
		#else
		pitchDownMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGL(pitch_down_string, "Cube", pitchDownMesh[i], &pitchDownDisplayList[i]) != 0) return -1; 
		#endif
	}
	for(i = 0; i < 13; i++)
	{
		pitch_up_string[25] = (char)(((int)'0')+(i+1)%10);
		pitch_up_string[24] = (char)(((int)'0')+((i+1)/10));
		#if defined(__wii__)
		pitchUpMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGX(pitch_up_string, "Cube", 
			pitchUpMesh[i], &pitchUpListSize[i], &pitchUpDisplayList[i]) != 0) return -1;
		#else
		pitchUpMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGL(pitch_up_string, "Cube", pitchUpMesh[i], &pitchUpDisplayList[i]) != 0) return -1; 
		#endif
	}


	for(i = 0; i < 11; i++)
	{
		throw_load_string[27] = (char)(((int)'0')+(i+1)%10);
		throw_load_string[26] = (char)(((int)'0')+((i+1)/10));
		#if defined(__wii__)
		throwLoadMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGX(throw_load_string, "Cube", 
			throwLoadMesh[i], &throwLoadListSize[i], &throwLoadDisplayList[i]) != 0) return -1;
		#else
		throwLoadMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGL(throw_load_string, "Cube", throwLoadMesh[i], &throwLoadDisplayList[i]) != 0) return -1; 
		#endif
	}
	for(i = 0; i < 21; i++)
	{
		throw_release_string[30] = (char)(((int)'0')+(i+1)%10);
		throw_release_string[29] = (char)(((int)'0')+((i+1)/10));
		#if defined(__wii__)
		throwReleaseMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGX(throw_release_string, "Cube", 
			throwReleaseMesh[i], &throwReleaseListSize[i], &throwReleaseDisplayList[i]) != 0) return -1;
		#else
		throwReleaseMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGL(throw_release_string, "Cube", throwReleaseMesh[i], &throwReleaseDisplayList[i]) != 0) return -1; 
		#endif
	}
	for(i = 0; i < 34; i++)
	{
		swing_string[24] = (char)(((int)'0')+(i+1)%10);
		swing_string[23] = (char)(((int)'0')+((i+1)/10));
		#if defined(__wii__)
		swingMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGX(swing_string, "Sphere", 
			swingMesh[i], &swingListSize[i], &swingDisplayList[i]) != 0) return -1;
		#else
		swingMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGL(swing_string, "Sphere", swingMesh[i], &swingDisplayList[i]) != 0) return -1; 
		#endif
	}
	for(i = 0; i < 34; i++)
	{
		bunt_string[23] = (char)(((int)'0')+(i+1)%10);
		bunt_string[22] = (char)(((int)'0')+((i+1)/10));
		#if defined(__wii__)
		buntMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGX(bunt_string, "Sphere", 
			buntMesh[i], &buntListSize[i], &buntDisplayList[i]) != 0) return -1;
		#else
		buntMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGL(bunt_string, "Sphere", buntMesh[i], &buntDisplayList[i]) != 0) return -1; 
		#endif
	}
	for(i = 0; i < 34; i++)
	{
		batting_stop_string[31] = (char)(((int)'0')+(i+1)%10);
		batting_stop_string[30] = (char)(((int)'0')+((i+1)/10));
		#if defined(__wii__)
		battingStopMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGX(batting_stop_string, "Sphere", 
			battingStopMesh[i], &battingStopListSize[i], &battingStopDisplayList[i]) != 0) return -1;
		#else
		battingStopMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGL(batting_stop_string, "Sphere", battingStopMesh[i], &battingStopDisplayList[i]) != 0) return -1; 
		#endif
	}

	// and the rest of meshes and textures:
	#if defined(__wii__)
	TPL_OpenTPLFromMemory(&team1TPL, (void *)team1_tpl, team1_tpl_size);
	TPL_GetTexture(&team1TPL, team1, &team1Texture);
	TPL_OpenTPLFromMemory(&team2TPL, (void *)team2_tpl, team2_tpl_size);
	TPL_GetTexture(&team2TPL, team2, &team2Texture);	
	TPL_OpenTPLFromMemory(&team3TPL, (void *)team3_tpl, team3_tpl_size);
	TPL_GetTexture(&team3TPL, team3, &team3Texture);	
	TPL_OpenTPLFromMemory(&team4TPL, (void *)team4_tpl, team4_tpl_size);
	TPL_GetTexture(&team4TPL, team4, &team4Texture);	
	TPL_OpenTPLFromMemory(&team5TPL, (void *)team5_tpl, team5_tpl_size);
	TPL_GetTexture(&team5TPL, team5, &team5Texture);
	TPL_OpenTPLFromMemory(&team6TPL, (void *)team6_tpl, team6_tpl_size);
	TPL_GetTexture(&team6TPL, team6, &team6Texture);	
	TPL_OpenTPLFromMemory(&team7TPL, (void *)team7_tpl, team7_tpl_size);
	TPL_GetTexture(&team7TPL, team7, &team7Texture);	
	TPL_OpenTPLFromMemory(&team8TPL, (void *)team8_tpl, team8_tpl_size);
	TPL_GetTexture(&team8TPL, team8, &team8Texture);	
	TPL_OpenTPLFromMemory(&team1JokerTPL, (void *)team1_joker_tpl, team1_joker_tpl_size);
	TPL_GetTexture(&team1JokerTPL, team1Joker, &team1JokerTexture);
	TPL_OpenTPLFromMemory(&team2JokerTPL, (void *)team2_joker_tpl, team2_joker_tpl_size);
	TPL_GetTexture(&team2JokerTPL, team2Joker, &team2JokerTexture);	
	TPL_OpenTPLFromMemory(&team3JokerTPL, (void *)team3_joker_tpl, team3_joker_tpl_size);
	TPL_GetTexture(&team3JokerTPL, team3Joker, &team3JokerTexture);	
	TPL_OpenTPLFromMemory(&team4JokerTPL, (void *)team4_joker_tpl, team4_joker_tpl_size);
	TPL_GetTexture(&team4JokerTPL, team4Joker, &team4JokerTexture);	
	TPL_OpenTPLFromMemory(&team5JokerTPL, (void *)team5_joker_tpl, team5_joker_tpl_size);
	TPL_GetTexture(&team5JokerTPL, team5Joker, &team5JokerTexture);
	TPL_OpenTPLFromMemory(&team6JokerTPL, (void *)team6_joker_tpl, team6_joker_tpl_size);
	TPL_GetTexture(&team6JokerTPL, team6Joker, &team6JokerTexture);	
	TPL_OpenTPLFromMemory(&team7JokerTPL, (void *)team7_joker_tpl, team7_joker_tpl_size);
	TPL_GetTexture(&team7JokerTPL, team7Joker, &team7JokerTexture);	
	TPL_OpenTPLFromMemory(&team8JokerTPL, (void *)team8_joker_tpl, team8_joker_tpl_size);
	TPL_GetTexture(&team8JokerTPL, team8Joker, &team8JokerTexture);	
	TPL_OpenTPLFromMemory(&selection1TPL, (void *)selectionBall1_tpl, selectionBall1_tpl_size);
	TPL_GetTexture(&selection1TPL, selectionBall1, &selection1Texture);
	TPL_OpenTPLFromMemory(&selection2TPL, (void *)selectionBall2_tpl, selectionBall2_tpl_size);
	TPL_GetTexture(&selection2TPL, selectionBall2, &selection2Texture);
	TPL_OpenTPLFromMemory(&selection3TPL, (void *)selectionBall3_tpl, selectionBall3_tpl_size);
	TPL_GetTexture(&selection3TPL, selectionBall3, &selection3Texture);	
	// call to our obj-loader.
	playerBareHandsStandingMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGX("data/player_bare_hands_standing.obj", "Sphere", 
		playerBareHandsStandingMesh, &playerBareHandsStandingListSize, &playerBareHandsStandingDisplayList) != 0) return -1;
	playerGloveWithBallStandingMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGX("data/player_glove_with_ball_standing.obj", "Cube", 
		playerGloveWithBallStandingMesh, &playerGloveWithBallStandingListSize, &playerGloveWithBallStandingDisplayList) != 0) return -1;
	playerGloveWithoutBallStandingMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGX("data/player_glove_without_ball_standing.obj", "Cube", 
		playerGloveWithoutBallStandingMesh, &playerGloveWithoutBallStandingListSize, &playerGloveWithoutBallStandingDisplayList) != 0) return -1;
	markerMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGX("data/pallo.obj", "Icosphere", markerMesh, 
		&markerListSize, &markerDisplayList) != 0) return -1;		
		
	// shadow needs a bit special treatment
	// we want to use our normal loading function LoadObj, but that initializes color to just be 255, 255, 255, 255 so 
	// we want to change that then afterwards.
	shadowMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	result = LoadObj("data/shadow.obj", "Circle", shadowMesh);
	if(result != 0)
	{
		printf("\nError with LoadObj. Error code: %d\n", result);
		return -1;
	}	
	free(shadowMesh->uColorIndex);
	shadowColors = memalign( 32, ( sizeof(u8) * 4 ) );
	shadowColors[0] = 255; shadowColors[1] = 255;
	shadowColors[2] = 255; shadowColors[3] = 127;
	shadowMesh->uColorIndex = shadowColors;			
	
	shadowListSize = prepareMesh(shadowMesh, &shadowDisplayList);			

	#else
	if(tryLoadingTextureGL(&team1Texture, "../../pc_textures/team1.tga", "team1") != 0) return -1;
	if(tryLoadingTextureGL(&team2Texture, "../../pc_textures/team2.tga", "team2") != 0) return -1;
	if(tryLoadingTextureGL(&team3Texture, "../../pc_textures/team3.tga", "team3") != 0) return -1;
	if(tryLoadingTextureGL(&team4Texture, "../../pc_textures/team4.tga", "team4") != 0) return -1;
	if(tryLoadingTextureGL(&team5Texture, "../../pc_textures/team5.tga", "team5") != 0) return -1;
	if(tryLoadingTextureGL(&team6Texture, "../../pc_textures/team6.tga", "team6") != 0) return -1;
	if(tryLoadingTextureGL(&team7Texture, "../../pc_textures/team7.tga", "team7") != 0) return -1;
	if(tryLoadingTextureGL(&team8Texture, "../../pc_textures/team8.tga", "team8") != 0) return -1;
	if(tryLoadingTextureGL(&team1JokerTexture, "../../pc_textures/team1_joker.tga", "team1Joker") != 0) return -1;
	if(tryLoadingTextureGL(&team2JokerTexture, "../../pc_textures/team2_joker.tga", "team2Joker") != 0) return -1;
	if(tryLoadingTextureGL(&team3JokerTexture, "../../pc_textures/team3_joker.tga", "team3Joker") != 0) return -1;
	if(tryLoadingTextureGL(&team4JokerTexture, "../../pc_textures/team4_joker.tga", "team4Joker") != 0) return -1;
	if(tryLoadingTextureGL(&team5JokerTexture, "../../pc_textures/team5_joker.tga", "team5Joker") != 0) return -1;
	if(tryLoadingTextureGL(&team6JokerTexture, "../../pc_textures/team6_joker.tga", "team6Joker") != 0) return -1;
	if(tryLoadingTextureGL(&team7JokerTexture, "../../pc_textures/team7_joker.tga", "team7Joker") != 0) return -1;
	if(tryLoadingTextureGL(&team8JokerTexture, "../../pc_textures/team8_joker.tga", "team8Joker") != 0) return -1;
	if(tryLoadingTextureGL(&selection1Texture, "../../pc_textures/selectionBall1.tga", "selection1") != 0) return -1;
	if(tryLoadingTextureGL(&selection2Texture, "../../pc_textures/selectionBall2.tga", "selection2") != 0) return -1;
	if(tryLoadingTextureGL(&selection3Texture, "../../pc_textures/selectionBall3.tga", "selection3") != 0) return -1;
	playerBareHandsStandingMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/player_bare_hands_standing.obj", "Sphere", playerBareHandsStandingMesh, &playerBareHandsStandingDisplayList) != 0) return -1; 
	playerGloveWithBallStandingMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/player_glove_with_ball_standing.obj", "Cube", playerGloveWithBallStandingMesh, &playerGloveWithBallStandingDisplayList) != 0) return -1; 	
	playerGloveWithoutBallStandingMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/player_glove_without_ball_standing.obj", "Cube", playerGloveWithoutBallStandingMesh, &playerGloveWithoutBallStandingDisplayList) != 0) return -1; 	
	markerMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/pallo.obj", "Icosphere", markerMesh, &markerDisplayList) != 0) return -1; 
	shadowMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/shadow.obj", "Circle", shadowMesh, &shadowDisplayList) != 0) return -1; 
	#endif

	return 0;
}

void drawPlayer(double alpha, PlayerInfo *playerInfo)
{
	int i;	
	int j = 0;
	double angle;
	// so we draw every players
	for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT + PLAYERS_IN_TEAM; i++)
	{
		j++;
		// calculate the angle in which player is facing
		#if defined(__wii__)			
		angle = atan2(-playerInfo[i].tPI.orientation.z, playerInfo[i].tPI.orientation.x);
		#else
		angle = atan2(-playerInfo[i].tPI.orientation.z, playerInfo[i].tPI.orientation.x) * 180.0f / PI;
		#endif
		// for every player we also move and rotate the players to right places
		#if defined(__wii__)	
		guMtxIdentity(model);
		guMtxRotAxisRad(rot, &rotYAxis, angle + PI/2);
		guMtxConcat(rot, model, model);
		guMtxScaleApply(model, model, PLAYER_SCALE, PLAYER_SCALE, PLAYER_SCALE);
		guMtxTransApply(model, model, alpha*playerInfo[i].tPI.location.x + (1-alpha)*playerInfo[i].tPI.lastLocation.x, 
			playerInfo[i].tPI.location.y, alpha*playerInfo[i].tPI.location.z + (1-alpha)*playerInfo[i].tPI.lastLocation.z);			
		guMtxConcat(view,model,modelview);	
		GX_LoadPosMtxImm(modelview, GX_PNMTX0);
		guMtxInverse(modelview,mvi);
		guMtxTranspose(mvi,modelview);
		GX_LoadNrmMtxImm(modelview, GX_PNMTX0);
		#else
		glPushMatrix();
		glTranslatef((float)(alpha*playerInfo[i].tPI.location.x + (1-alpha)*playerInfo[i].tPI.lastLocation.x), 
			(float)playerInfo[i].tPI.location.y, (float)(alpha*playerInfo[i].tPI.location.z + (1-alpha)*playerInfo[i].tPI.lastLocation.z));
		glScalef(PLAYER_SCALE, PLAYER_SCALE, PLAYER_SCALE);
		glRotatef((float)(angle + 90), 0.0f, 1.0f, 0.0f);
		
		#endif
		
		// and we render. first texture is selected, as there are different teams and within teams there are jokers
		// and nonjokers. and then the model is selected and it also calls the display list.
		textureSelection(playerInfo[i].cPI.team, playerInfo[i].bTPI.joker, 0);
		modelSelection(i);
		// and then there is the shadow.
		// we add 0.001f*j to SHADOW_HEIGHT so that there will be no visual problems of meshes being exactly at the same height.
		#if defined(__wii__)
		
		GX_SetBlendMode(GX_BM_BLEND, GX_BL_ZERO, GX_BL_SRCALPHA, GX_LO_SET);
		guMtxIdentity(model);
		guMtxScaleApply(model, model, 1.0f, 1.0f, 0.6f);
		guMtxTransApply(model, model, alpha*playerInfo[i].tPI.location.x + (1-alpha)*playerInfo[i].tPI.lastLocation.x - 0.2f, 
			SHADOW_HEIGHT + 0.001f*j, alpha*playerInfo[i].tPI.location.z + (1-alpha)*playerInfo[i].tPI.lastLocation.z);		
		guMtxConcat(view,model,modelview);	
		GX_LoadPosMtxImm(modelview, GX_PNMTX0);
		guMtxInverse(modelview,mvi);
		guMtxTranspose(mvi,modelview);
		GX_LoadNrmMtxImm(modelview, GX_PNMTX0);	
		GX_CallDispList(shadowDisplayList, shadowListSize);	
		GX_SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_SET);				
		#else
		glPopMatrix();	
		glEnable(GL_BLEND);
		glDisable(GL_LIGHTING);
		glPushMatrix();
		glTranslatef((float)(alpha*playerInfo[i].tPI.location.x + (1-alpha)*playerInfo[i].tPI.lastLocation.x - 0.2f), 
			SHADOW_HEIGHT + 0.001f*j, (float)(alpha*playerInfo[i].tPI.location.z + (1-alpha)*playerInfo[i].tPI.lastLocation.z));
		glScalef(1.0f, 1.0f, 0.6f);
		glBindTexture(GL_TEXTURE_2D, 0);  
		glCallList(shadowDisplayList); 
		glPopMatrix();	
		glDisable(GL_BLEND);
		glEnable(GL_LIGHTING);
		#endif
		// and then there is the marker at the top of player who is controlled
		#if defined(__wii__)
		if(i == stateInfo.localGameInfo->pII.controlIndex)
		{
			guMtxIdentity(model);
			guMtxScaleApply(model, model, SELECTION_BALL_SCALE, SELECTION_BALL_SCALE, SELECTION_BALL_SCALE);
			guMtxTransApply(model, model, (alpha*playerInfo[i].tPI.location.x + (1-alpha)*playerInfo[i].tPI.lastLocation.x), 
			playerInfo[i].tPI.location.y + 2.0f, (alpha*playerInfo[i].tPI.location.z + (1-alpha)*playerInfo[i].tPI.lastLocation.z));
			guMtxConcat(view,model,modelview);	
			GX_LoadPosMtxImm(modelview, GX_PNMTX0);
			guMtxInverse(modelview,mvi);
			guMtxTranspose(mvi,modelview);
			GX_LoadNrmMtxImm(modelview, GX_PNMTX0);	
			textureSelection(playerInfo[i].cPI.team, 0, 1);
			GX_CallDispList(markerDisplayList, markerListSize);			
	
		}	
		#else
		if(i == stateInfo.localGameInfo->pII.controlIndex)
		{
			textureSelection(playerInfo[i].cPI.team, 0, 1);
			glPushMatrix();
			glTranslatef((float)(alpha*playerInfo[i].tPI.location.x + (1-alpha)*playerInfo[i].tPI.lastLocation.x), 
			(float)playerInfo[i].tPI.location.y + 2.0f, (float)(alpha*playerInfo[i].tPI.location.z + (1-alpha)*playerInfo[i].tPI.lastLocation.z));
			glScalef(SELECTION_BALL_SCALE, SELECTION_BALL_SCALE, SELECTION_BALL_SCALE);
			glCallList(markerDisplayList); 
			glPopMatrix();
		}
		#endif
	}
}

static void textureSelection(int team, int joker, int type)
{
	// theres two types of texture selections, texture selection for player and texture selection for marker.
	if(type == 0)
	{
		// here value means which team, like ankkurit or lippo
		if((stateInfo.globalGameInfo)->teams[team].value == 1)
		{
			if(joker == 0)
			{
				#if defined(__wii__)	
				GX_LoadTexObj(&team1Texture, GX_TEXMAP0);
				#else
				glBindTexture(GL_TEXTURE_2D, team1Texture);  
				#endif
			}
			else
			{
				#if defined(__wii__)	
				GX_LoadTexObj(&team1JokerTexture, GX_TEXMAP0);
				#else
				glBindTexture(GL_TEXTURE_2D, team1JokerTexture);  
				#endif
			}
		}	
		else if((stateInfo.globalGameInfo)->teams[team].value == 2)
		{
			if(joker == 0)
			{
				#if defined(__wii__)	
				GX_LoadTexObj(&team2Texture, GX_TEXMAP0);
				#else
				glBindTexture(GL_TEXTURE_2D, team2Texture);  
				#endif
			}
			else
			{
				#if defined(__wii__)	
				GX_LoadTexObj(&team2JokerTexture, GX_TEXMAP0);
				#else
				glBindTexture(GL_TEXTURE_2D, team2JokerTexture);  
				#endif
			}
		}	
		else if((stateInfo.globalGameInfo)->teams[team].value == 3)
		{
			if(joker == 0)
			{
				#if defined(__wii__)	
				GX_LoadTexObj(&team3Texture, GX_TEXMAP0);
				#else
				glBindTexture(GL_TEXTURE_2D, team3Texture);  
				#endif
			}
			else
			{
				#if defined(__wii__)	
				GX_LoadTexObj(&team3JokerTexture, GX_TEXMAP0);
				#else
				glBindTexture(GL_TEXTURE_2D, team3JokerTexture);  
				#endif
			}
		}
		else if((stateInfo.globalGameInfo)->teams[team].value == 4)
		{
			if(joker == 0)
			{
				#if defined(__wii__)	
				GX_LoadTexObj(&team4Texture, GX_TEXMAP0);
				#else
				glBindTexture(GL_TEXTURE_2D, team4Texture);  
				#endif
			}
			else
			{
				#if defined(__wii__)	
				GX_LoadTexObj(&team4JokerTexture, GX_TEXMAP0);
				#else
				glBindTexture(GL_TEXTURE_2D, team4JokerTexture);  
				#endif
			}
		}
		else if((stateInfo.globalGameInfo)->teams[team].value == 5)
		{
			if(joker == 0)
			{
				#if defined(__wii__)	
				GX_LoadTexObj(&team5Texture, GX_TEXMAP0);
				#else
				glBindTexture(GL_TEXTURE_2D, team5Texture);  
				#endif
			}
			else
			{
				#if defined(__wii__)	
				GX_LoadTexObj(&team5JokerTexture, GX_TEXMAP0);
				#else
				glBindTexture(GL_TEXTURE_2D, team5JokerTexture);  
				#endif
			}
		}
		else if((stateInfo.globalGameInfo)->teams[team].value == 6)
		{
			if(joker == 0)
			{
				#if defined(__wii__)	
				GX_LoadTexObj(&team6Texture, GX_TEXMAP0);
				#else
				glBindTexture(GL_TEXTURE_2D, team6Texture);  
				#endif
			}
			else
			{
				#if defined(__wii__)	
				GX_LoadTexObj(&team6JokerTexture, GX_TEXMAP0);
				#else
				glBindTexture(GL_TEXTURE_2D, team6JokerTexture);  
				#endif
			}
		}
		else if((stateInfo.globalGameInfo)->teams[team].value == 7)
		{
			if(joker == 0)
			{
				#if defined(__wii__)	
				GX_LoadTexObj(&team7Texture, GX_TEXMAP0);
				#else
				glBindTexture(GL_TEXTURE_2D, team7Texture);  
				#endif
			}
			else
			{
				#if defined(__wii__)	
				GX_LoadTexObj(&team7JokerTexture, GX_TEXMAP0);
				#else
				glBindTexture(GL_TEXTURE_2D, team7JokerTexture);  
				#endif
			}
		}
		else if((stateInfo.globalGameInfo)->teams[team].value == 8)
		{
			if(joker == 0)
			{
				#if defined(__wii__)	
				GX_LoadTexObj(&team8Texture, GX_TEXMAP0);
				#else
				glBindTexture(GL_TEXTURE_2D, team8Texture);  
				#endif
			}
			else
			{
				#if defined(__wii__)	
				GX_LoadTexObj(&team8JokerTexture, GX_TEXMAP0);
				#else
				glBindTexture(GL_TEXTURE_2D, team8JokerTexture);  
				#endif
			}
		}
	}
	// and here, should we use green, blue or red ball on top of a player.
	// depends on who controls.
	else if(type == 1)
	{
		if((stateInfo.globalGameInfo)->teams[team].control == 0)
		{
			#if defined(__wii__)	
			GX_LoadTexObj(&selection1Texture, GX_TEXMAP0);
			#else
			glBindTexture(GL_TEXTURE_2D, selection1Texture);  
			#endif
		}	
		else if((stateInfo.globalGameInfo)->teams[team].control == 1)
		{
			#if defined(__wii__)
			GX_LoadTexObj(&selection2Texture, GX_TEXMAP0);
			#else
			glBindTexture(GL_TEXTURE_2D, selection2Texture);  
			#endif
		}	
		else if((stateInfo.globalGameInfo)->teams[team].control == 2)
		{
			#if defined(__wii__)
			GX_LoadTexObj(&selection3Texture, GX_TEXMAP0);
			#else
			glBindTexture(GL_TEXTURE_2D, selection3Texture);  
			#endif
		}
	}
}

static void modelSelection(int index)
{
	// and then we must select which mesh we use and call the corresponding display list.
	// for animations we just use the animationStage, that is being updated in game_manipulation, as index
	// in mesh arrays.
	int animIndex = stateInfo.localGameInfo->playerInfo[index].cPI.animationStage / stateInfo.localGameInfo->playerInfo[index].cPI.animationFrequency;
	switch(stateInfo.localGameInfo->playerInfo[index].cPI.model) {
		case 0:
			#if defined(__wii__)
			GX_CallDispList(playerGloveWithoutBallStandingDisplayList, playerGloveWithoutBallStandingListSize);
			#else
			glCallList(playerGloveWithoutBallStandingDisplayList); 
			#endif
			break;
		case 1:
			#if defined(__wii__)
			GX_CallDispList(playerGloveWithBallStandingDisplayList, playerGloveWithBallStandingListSize);
			#else
			glCallList(playerGloveWithBallStandingDisplayList); 
			#endif
			break;
		case 2:
			#if defined(__wii__)
			GX_CallDispList(playerGloveWithoutBallWalkingDisplayList[animIndex], 
				playerGloveWithoutBallWalkingListSize[animIndex]);
			#else
			glCallList(playerGloveWithoutBallWalkingDisplayList[animIndex]); 
			#endif
			break;
		case 3:
			#if defined(__wii__)
			GX_CallDispList(playerGloveWithBallWalkingDisplayList[animIndex], 
				playerGloveWithBallWalkingListSize[animIndex]);
			#else
			glCallList(playerGloveWithBallWalkingDisplayList[animIndex]); 
			#endif
			break;
		case 4:
			#if defined(__wii__)
			GX_CallDispList(playerGloveWithoutBallRunningDisplayList[animIndex], 
				playerGloveWithoutBallRunningListSize[animIndex]);
			#else
			glCallList(playerGloveWithoutBallRunningDisplayList[animIndex]); 
			#endif
			break;
		case 5:
			#if defined(__wii__)
			GX_CallDispList(playerGloveWithBallRunningDisplayList[animIndex], 
				playerGloveWithBallRunningListSize[animIndex]);
			#else
			glCallList(playerGloveWithBallRunningDisplayList[animIndex]); 
			#endif
			break;
		case 6: 
			#if defined(__wii__)
			GX_CallDispList(pitchDownDisplayList[animIndex], 
				pitchDownListSize[animIndex]);
			#else
			glCallList(pitchDownDisplayList[animIndex]); 
			#endif
			break;
		case 7: 
			#if defined(__wii__)
			GX_CallDispList(pitchUpDisplayList[animIndex], 
				pitchUpListSize[animIndex]);
			#else
			glCallList(pitchUpDisplayList[animIndex]); 
			#endif
			break;
		case 8:
			#if defined(__wii__)
			GX_CallDispList(throwLoadDisplayList[animIndex], 
				throwLoadListSize[animIndex]);
			#else
			glCallList(throwLoadDisplayList[animIndex]); 
			#endif
			break;
		case 9:
			#if defined(__wii__)
			GX_CallDispList(throwReleaseDisplayList[animIndex], 
				throwReleaseListSize[animIndex]);
			#else
			glCallList(throwReleaseDisplayList[animIndex]); 
			#endif
			break;
		case 10: 
			#if defined(__wii__)
			GX_CallDispList(playerBareHandsStandingDisplayList, playerBareHandsStandingListSize);	
			#else
			glCallList(playerBareHandsStandingDisplayList); 
			#endif
			break;
		case 11:
			#if defined(__wii__)
			GX_CallDispList(playerBareHandsWalkingDisplayList[animIndex], 
				playerBareHandsWalkingListSize[animIndex]);
			#else
			glCallList(playerBareHandsWalkingDisplayList[animIndex]); 
			#endif
			break;
		case 12:
			#if defined(__wii__)
			GX_CallDispList(playerBareHandsRunningDisplayList[animIndex], 
				playerBareHandsRunningListSize[animIndex]);
			#else
			glCallList(playerBareHandsRunningDisplayList[animIndex]); 
			#endif
			break;
		case 13: 
			#if defined(__wii__)
			GX_CallDispList(swingDisplayList[0], 
				swingListSize[0]);
			#else
			glCallList(swingDisplayList[0]); 
			#endif
			break;
		case 14: 
			#if defined(__wii__)
			GX_CallDispList(swingDisplayList[animIndex], 
				swingListSize[animIndex]);
			#else
			glCallList(swingDisplayList[animIndex]); 
			#endif
			break;
		case 15:
			#if defined(__wii__)
			GX_CallDispList(buntDisplayList[animIndex], 
				buntListSize[animIndex]);
			#else
			glCallList(buntDisplayList[animIndex]); 
			#endif
			break;
		case 16:
			#if defined(__wii__)
			GX_CallDispList(battingStopDisplayList[animIndex], 
				battingStopListSize[animIndex]);
			#else
			glCallList(battingStopDisplayList[animIndex]); 
			#endif
			break;
		case 17:
			break;
		default:
			break;
	}
}
// cleaning keeps the house tidy
int cleanPlayer()
{
	int i;
	cleanMesh(playerBareHandsStandingMesh);
	cleanMesh(playerGloveWithBallStandingMesh);
	cleanMesh(playerGloveWithoutBallStandingMesh);
	cleanMesh(markerMesh);
	cleanMesh(shadowMesh);
	for(i = 0; i < 16; i++)
	{
		cleanMesh(playerGloveWithBallWalkingMesh[i]);
		cleanMesh(playerGloveWithoutBallWalkingMesh[i]);
		cleanMesh(playerBareHandsWalkingMesh[i]);
	}
	for(i = 0; i < 20; i++)
	{
		cleanMesh(playerGloveWithBallRunningMesh[i]);
		cleanMesh(playerGloveWithoutBallRunningMesh[i]);
		cleanMesh(playerBareHandsRunningMesh[i]);
	}
	for(i = 0; i < 9; i++)
	{
		cleanMesh(pitchDownMesh[i]);
	}
	for(i = 0; i < 13; i++)
	{
		cleanMesh(pitchUpMesh[i]);
	}
	for(i = 0; i < 11; i++)
	{
		cleanMesh(throwLoadMesh[i]);
	}
	for(i = 0; i < 21; i++)
	{
		cleanMesh(throwReleaseMesh[i]);
	}
	for(i = 0; i < 34; i++)
	{
		cleanMesh(swingMesh[i]);
		cleanMesh(buntMesh[i]);
		cleanMesh(battingStopMesh[i]);
	}
	#if defined(__wii__)
	free(playerBareHandsStandingDisplayList);
	free(playerGloveWithBallStandingDisplayList);
	free(playerGloveWithoutBallStandingDisplayList);
	free(markerDisplayList);
	for(i = 0; i < 16; i++)
	{
		free(playerGloveWithBallWalkingDisplayList[i]);
		free(playerGloveWithoutBallWalkingDisplayList[i]);
		free(playerBareHandsWalkingDisplayList[i]);
	}
	for(i = 0; i < 20; i++)
	{
		free(playerGloveWithBallRunningDisplayList[i]);
		free(playerGloveWithoutBallRunningDisplayList[i]);
		free(playerBareHandsRunningDisplayList[i]);
	}
	for(i = 0; i < 9; i++)
	{
		free(pitchDownDisplayList[i]);
	}
	for(i = 0; i < 13; i++)
	{
		free(pitchUpDisplayList[i]);
	}
	for(i = 0; i < 11; i++)
	{
		free(throwLoadDisplayList[i]);
	}
	for(i = 0; i < 21; i++)
	{
		free(throwReleaseDisplayList[i]);
	}
	for(i = 0; i < 34; i++)
	{
		free(swingDisplayList[i]);
		free(buntDisplayList[i]);
		free(battingStopDisplayList[i]);
	}
	#endif
	return 0;
}
