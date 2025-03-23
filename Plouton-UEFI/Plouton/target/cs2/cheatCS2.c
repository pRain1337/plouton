/*++
*
* Source file for the functions used for the cs2 cheat
*
--*/

// our includes
#include "cheatCS2.h"

// CS2 process and the needed modules structs
static WinProc   cs2;
static WinModule client;

// Cheat Settings

//	-	Aimbot settings

//		-	Aimbot keys
//			- 0x1	= Mouse 1 (left mouse button)
//			- 0x2	= Mouse 2 (right mouse button)
//			- 0x4	= Mouse 3 (middle mouse button)
//			- 0x8	= Mouse 4 (Additional mouse button #1)
//			- 0x10	= Mouse 5 (Additional mouse button #2)
const INT8 aimbotKey = 0x1;

//		-	Aimbot use vischeck
const BOOLEAN AimbotVisibleCheck = TRUE;

//		-	Aimbot weapontypes (Struct: WeaponType)
const INT8 aimbotWeaponTypes = Rifle | SMG;

//		-	Rifle settings
const INT8 RifleFOV = 10;
const INT8 RifleHeadFOV = 8;
const INT8 RifleSmooth = 13;

//		-	SMG Settings
const INT8 SMGFOV = 8;
const INT8 SMGHeadFOV = 8;
const INT8 SMGSmooth = 7;

//		-	Pistol settings
const INT8 PistolFOV = 12;
const INT8 PistolHeadFOV = 10;
const INT8 PistolSmooth = 10;

//		-	Rest 
const INT8 RestFOV = 10;
const INT8 RestHeadFOV = 8;
const INT8 RestSmooth = 13;

//	-	Sound ESP settings
const INT8 SoundFOV = 70;


// Sigscanned offsets
UINT64 ofsDwEntityList                         = 0;
UINT64 ofsDwLocalPlayerController              = 0;
UINT64 ofsDwCSInput                            = 0;

// Entitylist Data that persists among executions
CS2Player playersGlobal[16];    // Global entitylist
UINT16 arrayEnd = 0;            // End of the playersGlobal array
UINT16 readIndex = 1;           // Current entities to read
UINT8 readNumber = 2;           // Number of entities to read by execution
UINT64 resetCounter = 0;        // Counts the SMI to reset the entitylist after some time

// current status of the aimbot
#define NO_TARGET -1			// No aimbot target, search on next execution
#define INVALID_TARGET -2		// Current aimbot target is invalid, search on next execution

// Aimbot Data that persists among executions
aimbotTarget gAimTarget = { .found = FALSE, .index = NO_TARGET };
UINT8 gFovIncrease = 0;			// Increase fov the longer you hold mouse 1
QAngle gLastPunch;				// Save the last punch vector for perfect recoil control
	
// Localplayer Variables
UINT32 localplayerLastTeam = 0;
UINT64 localplayerIndex = 0;

// Player Variables used to calculate the hitboxes
UINT8 headBone = 6;
QVector headBoneMin;
QVector headBoneMax;
fix32_t headBoneRadius = 0x44cccd000; // 4.3f

UINT8 pelvisBone = 0;
QVector pelvisBoneMin;
QVector pelvisBoneMax;
fix32_t pelvisBoneRadius = 0x600000000; // 6.f

// Main Variables
UINT8 lastIndexOnRadar = 0;

// From NtKernelTools.c
extern WinCtx* winGlobal;


// Localplayer Functions

/*
* Function:  getLocalPlayer
* --------------------
*  Returns the localPlayer structure of CS2. Struct is passed via parameter to the function
*
*  localplayer:			Pointer to CS2Local structure. Function returns data via this pointer
*
*  returns:	BOOLEAN, whether the localPlayer structure was found
*
*/
BOOLEAN getLocalPlayer(PCS2Local localplayer)
{
	// Now get the controller
	if (vRead((EFI_PHYSICAL_ADDRESS)&localplayer->controller, client.baseAddress + ofsDwLocalPlayerController, sizeof(UINT64), cs2.dirBase) == FALSE)
	{
		return FALSE;
	}

	if (localplayer->controller == 0)
	{
		return FALSE;
	}

	// Now get the hPawn from the controller
	localplayer->pawn = getPlayerPawn(localplayer->controller);

	LOG_DBG("[CS2] LPA: %x\r\n", localplayer->pawn);

	// Read current team id
	if (vRead((EFI_PHYSICAL_ADDRESS)&localplayer->team, localplayer->controller + ofs_m_iTeamNum, sizeof(UINT32), cs2.dirBase) == FALSE)
	{
		return FALSE;
	}

	if (localplayer->team != 2 && localplayer->team != 3) // 2 T | 3 CT
	{
		return FALSE;
	}

	LOG_DBG("[CS2] LPTe: %d\r\n", localplayer->team);

	// Get the current csinput structure
	EFI_VIRTUAL_ADDRESS csinput = 0;
	if (vRead((EFI_PHYSICAL_ADDRESS)(&csinput), client.baseAddress + ofsDwCSInput, sizeof(EFI_VIRTUAL_ADDRESS), cs2.dirBase) == FALSE)
	{
		return FALSE;
	}

	// Get the local viewangles
	unsigned char tviewangles[12];
	if (vRead((EFI_PHYSICAL_ADDRESS)(tviewangles), csinput + ofs_viewAngles, sizeof(Vector), cs2.dirBase) == FALSE)
	{
		return FALSE;
	}

	// used multiple times later, pre convert to fix32
	localplayer->viewAngles = FloatVectorToFixedVector((PVector)tviewangles);

	LOG_DBG("[CS2] VA X: %d Y: %d Z: %d\r\n", fix32_to_int(localplayer->viewAngles.x), fix32_to_int(localplayer->viewAngles.y), fix32_to_int(localplayer->viewAngles.z));

	// Get further details about the localplayer if the pawn is valid
	if (localplayer->pawn != 0)
	{
		if (vRead((EFI_PHYSICAL_ADDRESS)&localplayer->gameScene, localplayer->pawn + ofs_m_pGameSceneNode, sizeof(UINT64), cs2.dirBase) == FALSE)
		{
			return FALSE;
		}

		// only used once later, no point in already converting to fix32
		if (vRead((EFI_PHYSICAL_ADDRESS)(&localplayer->origin), localplayer->gameScene + ofs_m_vecAbsOrigin, sizeof(Vector), cs2.dirBase) == FALSE)
		{
			return FALSE;
		}

		LOG_DBG("[CS2] PO X: %d Y: %d Z: %d\r\n", float2int(&localplayer->origin.x[0]), float2int(&localplayer->origin.y[0]), float2int(&localplayer->origin.z[0]));

		// Get the vecViewOffset (relative to the origin position)
		// only used once later, no point in already converting to fix32
		if (vRead((EFI_PHYSICAL_ADDRESS)(&localplayer->vecViewOffset), localplayer->pawn + ofs_m_vecViewOffset, sizeof(Vector), cs2.dirBase) == FALSE)
		{
			return FALSE;
		}

		LOG_DBG("[CS2] VO X: %d Y: %d Z: %d\r\n", float2int(&localplayer->vecViewOffset.x[0]), float2int(&localplayer->vecViewOffset.y[0]), float2int(&localplayer->vecViewOffset.z[0]));

		// Calculate the final cameraPosition by adding the 
		localplayer->cameraPosition.x = fix32_add(fix32_from_float(localplayer->origin.x), fix32_from_float(localplayer->vecViewOffset.x));
		localplayer->cameraPosition.y = fix32_add(fix32_from_float(localplayer->origin.y), fix32_from_float(localplayer->vecViewOffset.y));
		localplayer->cameraPosition.z = fix32_add(fix32_from_float(localplayer->origin.z), fix32_from_float(localplayer->vecViewOffset.z));

		LOG_DBG("[CS2] CPO X: %d Y: %d Z: %d \r\n", fix32_to_int(localplayer->cameraPosition.x), fix32_to_int(localplayer->cameraPosition.y), fix32_to_int(localplayer->cameraPosition.z));

		// Now get the current weapon of the localplayer
		if (vRead((EFI_PHYSICAL_ADDRESS)(&localplayer->activeWeapon), localplayer->pawn + ofs_m_pClippingWeapon, sizeof(EFI_VIRTUAL_ADDRESS), cs2.dirBase) == FALSE)
		{
			return FALSE;
		}

		if (localplayer->activeWeapon != 0)
		{
			// Get current ammo of the weapon
			if (vRead((EFI_PHYSICAL_ADDRESS)(&localplayer->weaponAmmo), localplayer->activeWeapon + ofs_m_iClip1, sizeof(UINT32), cs2.dirBase) == FALSE)
			{
				return FALSE;
			}

			// Get current id of the weapon
			if (vRead((EFI_PHYSICAL_ADDRESS)(&localplayer->weaponId), localplayer->activeWeapon + ofs_m_AttributeManager + ofs_m_Item + ofs_m_iItemDefinitionIndex, sizeof(UINT16), cs2.dirBase) == FALSE)
			{
				return FALSE;
			}

			LOG_DBG("[CS2] iWID: %d\r\n", localplayer->weaponId);

			// Get Weapon type
			localplayer->weaponType = 0;
			if (checkRifle(localplayer->weaponId) == TRUE)
			{
				localplayer->weaponType = Rifle;
			}
			else if (checkPistol(localplayer->weaponId) == TRUE)
			{
				localplayer->weaponType = Pistol;
			}
			else if (checkSniper(localplayer->weaponId) == TRUE)
			{
				localplayer->weaponType = Sniper;
			}
			else if (checkSMG(localplayer->weaponId) == TRUE)
			{
				localplayer->weaponType = SMG;
			}

			// Get pointer to the punch cache struct
			if (vRead((EFI_PHYSICAL_ADDRESS)(&localplayer->punchCache), localplayer->pawn + ofs_m_aimPunchCache, sizeof(CS2CUtlVector), cs2.dirBase) == FALSE)
			{
				return FALSE;
			}

			// Check if we got a valid punch cache struct
			if (localplayer->punchCache.size > 0 && localplayer->punchCache.size < 0xFFFF)
			{
				// Read the actual local aim punch
				// only used once later, no point in already converting to fix32
				if (vRead((EFI_PHYSICAL_ADDRESS)(&localplayer->punchAngles), localplayer->punchCache.data + (localplayer->punchCache.size - 1) * sizeof(Vector), sizeof(Vector), cs2.dirBase) == FALSE)
				{
					return FALSE;
				}

				LOG_DBG("[CS2] PA X: %d Y: %d Z: %d\r\n", float2int(&localplayer->punchAngles.x[0]), float2int(&localplayer->punchAngles.y[0]), float2int(&localplayer->punchAngles.z[0]));

				// Calculate the recoil angles now
				localplayer->recoilAngles.x = fix32_add(localplayer->viewAngles.x, fix32_mul(fix32_from_float(localplayer->punchAngles.x), fix32_two));
				localplayer->recoilAngles.y = fix32_add(localplayer->viewAngles.y, fix32_mul(fix32_from_float(localplayer->punchAngles.y), fix32_two));
				localplayer->recoilAngles.z = fix32_add(localplayer->viewAngles.z, fix32_mul(fix32_from_float(localplayer->punchAngles.z), fix32_two));

				LOG_DBG("[CS2] RA X: %d Y: %d Z: %d\r\n", fix32_to_int(localplayer->recoilAngles.x), fix32_to_int(localplayer->recoilAngles.y), fix32_to_int(localplayer->recoilAngles.z));
			}
		}
	}

	return TRUE;
}

/*
* Function:  checkRifle
* --------------------
*  Returns true if given weapon ID is a rifle
*
*  weaponid:			(INT) ID of weapon 
*
*  returns:	TRUE if the given ID is a weapon ID of a rifle.
*
*/
BOOLEAN checkRifle(int weaponid)
{
	for (int i = 0; i < sizeof(weaponList); i++)
	{
		if (weaponid == weaponList[i])
		{
			return TRUE;
		}
	}
	return FALSE;
}

/*
* Function:  checkPistol
* --------------------
*  Returns true if given weapon ID is a pistol
*
*  weaponid:			(INT) ID of weapon 
*
*  returns:	TRUE if the given ID is a weapon ID of a pistol.
*
*/
BOOLEAN checkPistol(int weaponid)
{
	for (int i = 0; i < sizeof(pistolList); i++)
	{
		if (weaponid == pistolList[i])
		{
			return TRUE;
		}
	}
	return FALSE;
}

/*
* Function:  checkSniper
* --------------------
*  Returns true if given weapon ID is a sniper
*
*  weaponid:			(INT) ID of weapon 
*
*  returns:	TRUE if the given ID is a weapon ID of a sniper.
*
*/
BOOLEAN checkSniper(int weaponid)
{
	for (int i = 0; i < sizeof(sniperList); i++)
	{
		if (weaponid == sniperList[i])
		{
			return TRUE;
		}
	}
	return FALSE;
}

/*
* Function:  checkSMG
* --------------------
*  Returns true if given weapon ID is an SMG
*
*  weaponid:			(INT) ID of weapon 
*
*  returns:	TRUE if the given ID is a weapon ID of an SMG.
*
*/
BOOLEAN checkSMG(int weaponid)
{
	for (int i = 0; i < sizeof(smgList); i++)
	{
		if (weaponid == smgList[i])
		{
			return TRUE;
		}
	}
	return FALSE;
}

// Player Functions

/*
* Function:  getPlayerController
* --------------------
*  Returns the memory address of the playerController with ID given as paramenter
*
*  index:				(INT) the index of the player whose playerController address we want to find
*
*  returns: The playerController address of given player ID as EFI_VIRTUAL_ADDRESS, or NULL if not found.
*
*/
EFI_VIRTUAL_ADDRESS getPlayerController(UINT16 index)
{
	// First get the entitylist pointer
	EFI_VIRTUAL_ADDRESS entityList = 0;
	if (vRead((EFI_PHYSICAL_ADDRESS)(&entityList), client.baseAddress + ofsDwEntityList, sizeof(EFI_VIRTUAL_ADDRESS), cs2.dirBase) == FALSE)
	{
		return 0;
	}

	// Check if we got a valid result
	if (entityList == 0)
	{
		// Failed reading entitylist
		return 0;
	}

	// Check if the supplied index is valid
	if (index > 0x7FFF)
	{
		return 0;
	}

	// Get the array entry from the list (VALVE CODE, the whole address calculations ends in entitylist + 0x10)
	EFI_VIRTUAL_ADDRESS array_entry = 0;
	if (vRead((EFI_PHYSICAL_ADDRESS)(&array_entry), entityList + (0x8 * (index & 0x7FFF) >> 9) + 0x10, sizeof(EFI_VIRTUAL_ADDRESS), cs2.dirBase) == FALSE)
	{
		return 0;
	}

	// Check if we got a valid entry
	if (array_entry == 0)
	{
		return 0;
	}

	// Now get the actual controller
	EFI_VIRTUAL_ADDRESS controller = 0;
	if (vRead((EFI_PHYSICAL_ADDRESS)(&controller), array_entry + ((index & 0x1FF) * 120), sizeof(EFI_VIRTUAL_ADDRESS), cs2.dirBase) == FALSE)
	{
		return 0;
	}

	return controller;
}

/*
* Function:  getPlayerPawn
* --------------------
*  Returns the memory address of the playerPawn with playerController given as paramenter
*
*  controller:			(EFI_VIRTUAL_ADDRESS) the address of playerController whose playerPawn address we want to find
*
*  returns: The playerPawn address as EFI_VIRTUAL_ADDRESS, or NULL if not found.
*
*/
EFI_VIRTUAL_ADDRESS getPlayerPawn(EFI_VIRTUAL_ADDRESS controller)
{
	// First get the handle to the pawn from the controller
	UINT32 pawn_handle = 0;
	if (vRead((EFI_PHYSICAL_ADDRESS)(&pawn_handle), controller + ofs_m_hPawn, sizeof(UINT32), cs2.dirBase) == FALSE)
	{
		return 0;
	}

	// Check if we got a valid handle
	if (pawn_handle == 0)
	{
		return 0;
	}

	// Now get the entitylist pointer
	EFI_VIRTUAL_ADDRESS entityList = 0;
	if (vRead((EFI_PHYSICAL_ADDRESS)(&entityList), client.baseAddress + ofsDwEntityList, sizeof(UINT64), cs2.dirBase) == FALSE)
	{
		return 0;
	}

	// Check if we got a valid result
	if (entityList == 0)
	{
		// Failed reading entitylist
		return 0;
	}

	// Now get the listentry from the entitylist (VALVE CODE, the whole address calculations ends in entitylist + 0x10)
	EFI_VIRTUAL_ADDRESS list_entry = 0;
	if (vRead((EFI_PHYSICAL_ADDRESS)(&list_entry), entityList + 8 * ((pawn_handle & 0x7FFF) >> 9) + 16, sizeof(UINT64), cs2.dirBase) == FALSE)
	{
		return 0;
	}

	// Check if we got a valid entry
	if (list_entry == 0)
	{
		return 0;
	}

	// Now get the actual player pawn from the list entry
	EFI_VIRTUAL_ADDRESS player_pawn = 0;
	if (vRead((EFI_PHYSICAL_ADDRESS)(&player_pawn), list_entry + 120 * (pawn_handle & 0x1FF), sizeof(UINT64), cs2.dirBase) == FALSE)
	{
		return 0;
	}

	return player_pawn;
}

/*
* Function:  getPlayerBoneTransform
* --------------------
*  Reads the given bone data into a local CTransform struct which is used to parse the hitbox
*
*  pCTransform:			Pointer to an CTransform struct where the data will be stored
*  transformPointer:	UINT64 (EFI_VIRTUAL_ADDRESS) that points to the bone struct in memory 
*
*  returns:	TRUE if it successfully read the bone struct, false otherwise
*
*/
BOOLEAN getPlayerBoneTransform(CTransform* pCTransform, EFI_VIRTUAL_ADDRESS transformPointer)
{
	// First read the CTransform into temporary variables
	Vector boneOrigin;
	if (vRead((EFI_PHYSICAL_ADDRESS)(&boneOrigin), transformPointer, sizeof(Vector), cs2.dirBase) == FALSE)
	{
		return FALSE;
	}

	// Now read the scale of the CTransform struct
	unsigned char boneScale[4];
	if (vRead((EFI_PHYSICAL_ADDRESS)(&boneScale[0]), transformPointer + 12, 4, cs2.dirBase) == FALSE)
	{
		return FALSE;
	}

	// Now read the rotation of the CTransform struct
	unsigned char boneRotation[16];
	if (vRead((EFI_PHYSICAL_ADDRESS)(&boneRotation[0]), transformPointer + 16, 16, cs2.dirBase) == FALSE)
	{
		return FALSE;
	}

	// Set the values in the CTransform struct
	pCTransform->origin.x = fix32_from_float(boneOrigin.x);
	pCTransform->origin.y = fix32_from_float(boneOrigin.y);
	pCTransform->origin.z = fix32_from_float(boneOrigin.z);

	pCTransform->scale = fix32_from_float(&boneScale[0]);

	pCTransform->rotation.x = fix32_from_float(&boneRotation[0]);
	pCTransform->rotation.y = fix32_from_float(&boneRotation[4]);
	pCTransform->rotation.z = fix32_from_float(&boneRotation[8]);
	pCTransform->rotation.r = fix32_from_float(&boneRotation[12]);

	return TRUE;
}

/*
* Function:  getPlayerByIndex
* --------------------
*  Reads the data of the player entity at the given index
*
*  player:				Pointer to the CS2Player struct where the data should be written to
*  index:				UINT16 Index of the player entity that should be read
*  localplayer:			Pointer to the CS2Local struct to check against team id
*  updateTeammates:		BOOLEAN to indicate if teammates should be dumped
*
*  returns:	TRUE if the index was successfully read, false otherwise
*
*/
BOOLEAN getPlayerByIndex(PCS2Player player, UINT16 index, PCS2Local localplayer, BOOLEAN updateTeammates)
{
	LOG_DBG("[CS2] Index: %d ", index);

	// Get controller from entitylist (thanks kidua)
	player->controller = getPlayerController(index);

	// Check if we got a valid controller
	if (player->controller == 0)
	{
		// Failed getting controller
		return FALSE;
	}

	// Get pawn from list (thanks kidua)
	player->pawn = getPlayerPawn(player->controller);

	// Check if we got a valid pawn
	if (player->pawn == 0)
	{
		// Failed getting pawn
		return FALSE;
	}

	player->index = index;

	// Check if it's the localplayer entity by comparing the controller pointer
	if (localplayer->controller == player->controller)
	{
		// Localplayer entity
		localplayerIndex = index - 1;
		return FALSE;
	}

	// Get health from entity
	if (vRead((EFI_PHYSICAL_ADDRESS)&player->health, player->pawn + ofs_m_iHealth, sizeof(UINT32), cs2.dirBase) == FALSE)
	{
		return FALSE;
	}

	LOG_DBG(" health: %d", player->health);

	// Check if the entity has a valid health
	if (player->health <= 0 || player->health > 100)
	{
		return FALSE;
	}

	// Get alive status from entity
	if (vRead((EFI_PHYSICAL_ADDRESS)&player->alive, player->controller + ofs_m_bPawnIsAlive, sizeof(BOOLEAN), cs2.dirBase) == FALSE)
	{
		return FALSE;
	}

	LOG_DBG(" alive: %d ", player->alive);

	// Check if the entity has a valid alive status
	if (player->alive == FALSE)
	{
		return FALSE;
	}

	// Get the immune status of the player
	if (vRead((EFI_PHYSICAL_ADDRESS)&player->immune, player->pawn + ofs_m_bGunGameImmunity, sizeof(BOOLEAN), cs2.dirBase) == FALSE)
	{
		return FALSE;
	}

	LOG_DBG(" immune: %d ", player->immune);

	// Get current team from the entity
	if (vRead((EFI_PHYSICAL_ADDRESS)&player->team, player->controller + ofs_m_iTeamNum, sizeof(UINT32), cs2.dirBase) == FALSE)
	{
		return FALSE;
	}

	// Check if it's a valid team
	if (player->team != 2 && player->team != 3)
	{
		return FALSE;
	}

	LOG_DBG(" team: %d ", player->team);

	// Check if the entity is same team as our localplayer
	if (player->team == localplayer->team)
	{
		// Check if we should ignore teammates
		if (updateTeammates == TRUE)
		{
			// Set them as spotted, so they're always visible
			player->spotted = TRUE;
		}
		else
		{
			// Ignore the entity
			return FALSE;
		}
	}
	else
	{
		// It's an enemy entity, get the spotted status
		UINT32 SpottedByMask = 0;
		if (vRead((EFI_PHYSICAL_ADDRESS)&SpottedByMask, player->pawn + ofs_m_entitySpottedState + ofs_m_bSpottedByMask, sizeof(UINT32), cs2.dirBase) == FALSE)
		{
			return FALSE;
		}

		// Only set it as spotted, if it's spotted by our localplayer
		player->spotted = SpottedByMask & (1 << (EFI_VIRTUAL_ADDRESS)localplayer->controller);

		LOG_DBG(" spotted: %d ", player->spotted);
	}

	// If we get here, it should be a valid enemy/teammate entity

	// Get the game scene pointer of the entity
	if (vRead((EFI_PHYSICAL_ADDRESS)&player->gameScene, player->pawn + ofs_m_pGameSceneNode, sizeof(EFI_VIRTUAL_ADDRESS), cs2.dirBase) == FALSE)
	{
		return FALSE;
	}

	// Check if we got a valid game scene
	if (player->gameScene != 0)
	{
		// Get the bone array pointer of the game scene
		if (vRead((EFI_PHYSICAL_ADDRESS)&player->boneArray, player->gameScene + ofs_m_modelState + 0x80, sizeof(UINT64), cs2.dirBase) == FALSE)
		{
			return FALSE;
		}

		// Check if we got a valid bone array
		if (player->boneArray != 0)
		{
			/* New hitbox code, thanks to kidua
			/  - Calculates the center of the hitbox using the whole transform of the bone
			*/
			CTransform headBone_transform;
			if (getPlayerBoneTransform(&headBone_transform, player->boneArray + (headBone * 32)) == FALSE)
			{
				// Failed reading transform
				LOG_ERROR("[CS2] Failed parsing head transform\r\n");

				// Abort, failed reading player bone
				return FALSE;
			}

			CTransform pelvisBone_transform;
			if (getPlayerBoneTransform(&pelvisBone_transform, player->boneArray + (pelvisBone * 32)) == FALSE)
			{
				// Failed reading transform
				LOG_ERROR("[CS2] Failed prasing pelvis transform\r\n");

				// Abort, failed reading player bone
				return FALSE;
			}

			// Calculate the center position for head and stomach
			player->bonePositionHead = GetHitboxCenter(headBone_transform, headBoneMin, headBoneMax);
			player->bonePositionPelvis = GetHitboxCenter(pelvisBone_transform, pelvisBoneMin, pelvisBoneMax);

			LOG_DBG("[CS2] X: %d Y: %d Z: %d\r\n", fix32_to_int(player->bonePositionHead.x), fix32_to_int(player->bonePositionHead.y), fix32_to_int(player->bonePositionHead.z));

		}
	}

	// Get velocity of the entity for aimbot interpolation
	unsigned char velocity[12];
	if (vRead((EFI_PHYSICAL_ADDRESS)(velocity), player->controller + ofs_m_vecVelocity, sizeof(Vector), cs2.dirBase) == FALSE)
	{
		return FALSE;
	}

	// used multiple times later, pre convert to fix32
	player->velocity = FloatVectorToFixedVector((PVector)velocity);

	return TRUE;
}

// Feature functions

/*
* Function:  extrapolatePosition
* --------------------
*  Calculates the extrapolated position of the entity based on it's current position and the velocity
*
*  position:			QVector of the current position
*  velocity:			QVector of the velocity of the entity
*
*  returns:	Extrapolated position as QVector
*
*/
QVector extrapolatePosition(QVector position, QVector velocity)
{
	// ivr is hardcoded to 0.015625 in cs2 (thanks 64 tick)
	fix32_t fInterval = fix32_0015625;

	// Calculate the extrapolated position
	QVector result = QVectorAdd(position, QVectorMul(velocity, fInterval));

	// Return the extrapolated position
	return result;
}

// Aimbot Functions 

/*
* Function:  findNewAimbotTarget TO-DO
* --------------------
*  Main aimbot function, goes through the player entity and find suitable targets that are in our FOV.
*  Returns the result as aimbotTarget struct, which will be passed to the next function.
*
*  localplayer:		CS2Local containing localplayer data
*  players:			Pointer to an array of CS2Player entities
*  playerAmount:	UINT8 amount of players in the player list
*  fovIncrease:		fix32_t value that is the current increase of FOV, to increase the FOV depending on how long the aimbot is activated 
*
*  returns:	aimbotTarget struct including status, target index, hitbox etc.
*
*/
aimbotTarget findNewAimbotTarget(CS2Local localplayer, CS2Player* players, UINT8 playerAmount, fix32_t fovIncrease)
{
	// Basic settings
	fix32_t fBestFOV = fix32_add((fix32_t)((INT64)fix32_one * RifleFOV), fix32_div(fovIncrease, fix32_two));
	fix32_t fHeadFOV = (fix32_t)((INT64)fix32_one * RifleHeadFOV);
	fix32_t fSmooth = (fix32_t)((INT64)fix32_one * RifleSmooth);

	// Save a local angle for further use
	QAngle localAngle;

	// Set variables depending on current weapon type
	switch (localplayer.weaponType)
	{
	case None:
		break;

		// For rifles or SMG, add punch angles for RCS
	case Rifle:
	{
		localAngle.x = localplayer.recoilAngles.x;
		localAngle.y = localplayer.recoilAngles.y;
		localAngle.z = localplayer.recoilAngles.z;

		fBestFOV = fix32_add((fix32_t)((INT64)fix32_one * RifleFOV), fix32_div(fovIncrease, fix32_two));
		fHeadFOV = (fix32_t)((INT64)fix32_one * RifleHeadFOV);
		fSmooth = (fix32_t)((INT64)fix32_one * RifleSmooth);
		break;
	}

	case SMG:
	{
		localAngle.x = localplayer.recoilAngles.x;
		localAngle.y = localplayer.recoilAngles.y;
		localAngle.z = localplayer.recoilAngles.z;

		fBestFOV = fix32_add((fix32_t)((INT64)fix32_one * SMGFOV), fix32_div(fovIncrease, fix32_two));
		fHeadFOV = (fix32_t)((INT64)fix32_one * SMGHeadFOV);
		fSmooth = (fix32_t)((INT64)fix32_one * SMGSmooth);
		break;
	}

	case Pistol:
	{
		localAngle.x = localplayer.recoilAngles.x;
		localAngle.y = localplayer.recoilAngles.y;
		localAngle.z = localplayer.recoilAngles.z;

		fBestFOV = fix32_add((fix32_t)((INT64)fix32_one * PistolFOV), fix32_div(fovIncrease, fix32_two));
		fHeadFOV = (fix32_t)((INT64)fix32_one * PistolHeadFOV);
		fSmooth = (fix32_t)((INT64)fix32_one * PistolSmooth);
		break;
	}

	// Rest of weapons
	default:
	{
		localAngle.x = localplayer.viewAngles.x;
		localAngle.y = localplayer.viewAngles.y;
		localAngle.z = localplayer.viewAngles.z;

		fBestFOV = (fix32_t)((INT64)fix32_one * RestFOV);
		fHeadFOV = (fix32_t)((INT64)fix32_one * RestHeadFOV);
		fSmooth = (fix32_t)((INT64)fix32_one * RestSmooth);
		break;
	}
	}

	// Variable which will hold the result, if found
	aimbotTarget bestTarget;
	bestTarget.found = FALSE;
	bestTarget.index = -1;
	bestTarget.hitbox = Chest;
	bestTarget.smooth = fSmooth;

	// Loop through the playerlist, find best target for current aim angles and weapon
	for (int i = 0; i < playerAmount; i++)
	{
		CS2Player* tempPlayer = &players[i];

		// Dont use player thats not visible
		if (tempPlayer->spotted == FALSE && AimbotVisibleCheck)
		{
		    continue;
		}

		// create fix32 vectors for target player bone/hitbox positions
		QVector _f32_playerPositionHead = tempPlayer->bonePositionHead;
		QVector _f32_playerPositionPelvis = tempPlayer->bonePositionPelvis;

		// There is no "chest/neck" bone, we use middle point between head and stomach
		QVector _f32_playerPositionChest = QVectorDiv(QVectorAdd(_f32_playerPositionHead, _f32_playerPositionPelvis), fix32_two);

		// Vars used for target validation
		QAngle aimAngles;
		fix32_t fDistance;
		fix32_t fFOV;

		if (localplayer.weaponType != Pistol)
		{
			// First check if the stomach is valid with these settings
			aimAngles = CalcAngle(localplayer.cameraPosition, _f32_playerPositionPelvis);
			fDistance = QDistanceTo(localplayer.cameraPosition, _f32_playerPositionPelvis);
			fFOV = GetScaledFov(localAngle, aimAngles, fDistance);

			if (fix32_less(fFOV, fBestFOV))
			{
				fBestFOV = fFOV;
				bestTarget.found = TRUE;
				bestTarget.index = tempPlayer->index;
				bestTarget.hitbox = Pelvis;
			}

			// Check if the chest is also valid and possibly closer than stomach
			aimAngles = CalcAngle(localplayer.cameraPosition, _f32_playerPositionChest);
			fDistance = QDistanceTo(localplayer.cameraPosition, _f32_playerPositionChest);
			fFOV = GetScaledFov(localAngle, aimAngles, fDistance);

			if (fix32_less(fFOV, fBestFOV))
			{
				fBestFOV = fFOV;
				bestTarget.found = TRUE;
				bestTarget.index = tempPlayer->index;
				bestTarget.hitbox = Chest;
			}
		}

		// Check if the head is also valid and at least 2x closer than chest/stomach
		aimAngles = CalcAngle(localplayer.cameraPosition, _f32_playerPositionHead);
		fDistance = QDistanceTo(localplayer.cameraPosition, _f32_playerPositionHead);
		fFOV = GetScaledFov(localAngle, aimAngles, fDistance);

		if (fix32_less(fFOV, fBestFOV) || fix32_less(fFOV, fHeadFOV))
		{
			fBestFOV = fFOV;
			bestTarget.found = TRUE;
			bestTarget.index = tempPlayer->index;
			bestTarget.hitbox = Head;
		}
	}

	return bestTarget;
}

// Sound ESP Functions

/*
* Function:  findNewSoundTarget
* --------------------
*  Main Sound ESP function, goes through the player entity and find suitable targets that are in our FOV.
*  Returns the result as soundTarget struct, which will be used to interact with the headset.
*
*  localplayer:		CS2Local containing localplayer data
*  players:			Pointer to an array of CS2Player entities
*  playerAmount:	UINT8 amount of players in the player list		
*
*  returns:	soundTarget struct indicating distance left-right and if a target was found 
*
*/
soundTarget findNewSoundTarget(CS2Local localplayer, CS2Player* players, UINT8 playerAmount)
{
	// Preallocate the variables
	QAngle localAngle;

	localAngle.x = localplayer.viewAngles.x;
	localAngle.y = localplayer.viewAngles.y;
	localAngle.z = localplayer.viewAngles.z;

	// FOV that it should beep one
	fix32_t fBestFOV = (fix32_t)((INT64)fix32_one * SoundFOV);

	// Variable which will hold the result, if found
	soundTarget bestTarget;
	bestTarget.found = FALSE;
	bestTarget.left = 0;
	bestTarget.right = 0;

	// Go through all player entities
	for (int i = 0; i < playerAmount; i++)
	{
		CS2Player* tempPlayer = &players[i];

		// When player is visible dont use it for soundesp
		if (tempPlayer->spotted == TRUE)
		{
			continue;
		}

		// Get current position of the player
		QVector playerPosition = tempPlayer->bonePositionPelvis;
		QVector playerVelocity = tempPlayer->velocity;

		// Get extrapolated position
		QVector extrapolatedPosition = extrapolatePosition(playerPosition, playerVelocity);

		// Calculate angles and FOV for us for this entity
		QAngle aimAngles = CalcAngle(localplayer.cameraPosition, extrapolatedPosition);
		fix32_t fDistance = QDistanceTo(localplayer.cameraPosition, extrapolatedPosition);
		fix32_t fFOV = GetScaledFov(localAngle, aimAngles, fDistance);

		// Check if this player would be better than the one we have currently
		if (fix32_less(fFOV, fBestFOV)) /*Stores the best identified target*/
		{
			// check if the target is actually still alive (prevent beeping on dead players)
			if (getPlayerByIndex(tempPlayer, tempPlayer->index, &localplayer, FALSE))
			{
				// Save this one as best position for now
				fBestFOV = fFOV;

				// Set it in the soundTarget struct which is used to play a sound
				bestTarget.found = TRUE;
				bestTarget.left = (INT16)int_from_fix32(fBestFOV);
				bestTarget.right = (INT16)int_from_fix32(fBestFOV);

				// sound esp is not about finding the best entity, if its in fov its enough
				return bestTarget;
			}
		}
	}

	// If we get here, no player entity in Sound ESP FOV was found.
	return bestTarget;
}

/*
* Function:  aimAtTarget
* --------------------
*  Final aimbot function, takes the previously found target and verifies if it is still a valid target.
*  Afterwards it recalculates the current aimbot coordinates to always have the best movement possible.
*
*  localplayer:		CS2Local containing localplayer data
*  players:			Pointer to an array of CS2Player entities
*  playerAmount:	UINT8 amount of players in the player list
*  aimTarget:		aimbotTarget struct containing the current best player to aim at
*  useSmooth:		BOOLEAN that indicates if smoothing should be used for aimbot moving operations
*
*  returns:	Returns the mouse coordinates as w2Mouse struct if found
*
*/
w2Mouse aimAtTarget(CS2Local localplayer, CS2Player* players, UINT8 playerAmount, aimbotTarget aimTarget, BOOLEAN useSmooth)
{
	// Create struct which will be passed to XHCI functions
	w2Mouse res = { .x = 0, .y = 0, .found = FALSE };
	CS2Player* tempPlayer;

	// Find the player with right ID
	for (int i = 0; i < playerAmount; i++)
	{
		// Get current player data
		tempPlayer = &players[i];

		// Check if it is the index that we found previously
		if (tempPlayer->index == aimTarget.index)
		{
			LOG_DBG("[CS2] Found aimbotTarget\r\n");

			res.found = TRUE;

			LOG_DBG("[CS2] 1AP X: %d Y: %d Z: %d\r\n", fix32_to_int(tempPlayer->bonePositionHead.x), fix32_to_int(tempPlayer->bonePositionHead.y), fix32_to_int(tempPlayer->bonePositionHead.z));

			// As we're using a cached entitylist, update the player again just to be 100% sure
			// Null the tempplayer again to be safe...
			nullBuffer((EFI_PHYSICAL_ADDRESS)tempPlayer, sizeof(CS2Player));

			// Update player data
			res.found = getPlayerByIndex(tempPlayer, aimTarget.index, &localplayer, FALSE);

			break;
		}
	}

	// bail out if the target was not found
	if (res.found == FALSE)
	{
		return res;
	}

	// At this point recalculate the data, as we cache the aimbotTarget until it is invalid, we might have some stale data...

	// Get our viewangles regarding the current weapon in use
	QAngle localAngle;
	switch (localplayer.weaponType)
	{
	case None:
		break;

		// For rifles or smgs, add punch angles for RCS
	case Rifle:
	{
		localAngle.x = localplayer.recoilAngles.x;
		localAngle.y = localplayer.recoilAngles.y;
		localAngle.z = localplayer.recoilAngles.z;
		break;
	}
	case SMG:
	{
		localAngle.x = localplayer.recoilAngles.x;
		localAngle.y = localplayer.recoilAngles.y;
		localAngle.z = localplayer.recoilAngles.z;
		break;
	}
	case Pistol:
	{
		localAngle.x = localplayer.recoilAngles.x;
		localAngle.y = localplayer.recoilAngles.y;
		localAngle.z = localplayer.recoilAngles.z;
		break;
	}

	// Snipers and pistols
	default:
	{
		localAngle.x = localplayer.viewAngles.x;
		localAngle.y = localplayer.viewAngles.y;
		localAngle.z = localplayer.viewAngles.z;
		break;
	}
	}

	// Target vector
	QVector targetVector;

	// Find the enemy bone/hitbox position
	QVector _f32_playerPositionHead = tempPlayer->bonePositionHead;
	QVector _f32_playerPositionPelvis = tempPlayer->bonePositionPelvis;

	// There is no "chest/neck" bone, we use middle point between head and stomach
	QVector _f32_playerPositionChest = QVectorDiv(QVectorAdd(_f32_playerPositionHead, _f32_playerPositionPelvis), fix32_two);

	switch (aimTarget.hitbox)
	{
	case Pelvis:
	{
		targetVector = _f32_playerPositionPelvis;
		break;
	}
	case Chest:
	{
		targetVector = _f32_playerPositionChest;
		break;
	}
	case Head:
	{
		targetVector = _f32_playerPositionHead;
		break;
	}
	}

	// extrapolate the found position using the current velocity
	QVector playerVelocity = tempPlayer->velocity;
	QVector extrapolatedPosition = extrapolatePosition(targetVector, playerVelocity);

	// calculate the angles for the extrapolated position
	QAngle angAimAngles = CalcAngle(localplayer.cameraPosition, extrapolatedPosition);

	// Calculate general variables for checks
	fix32_t fDistance = QDistanceTo(localplayer.cameraPosition, extrapolatedPosition);
	fix32_t fFOV = GetScaledFov(localAngle, angAimAngles, fDistance);

	// general check to avoid 360s
	if (fix32_less(fFOV, fix32_25))
	{
		// Modify the angles if smoothing should be used
		if (useSmooth == TRUE)
		{
			angAimAngles = SmoothAngle(localAngle, angAimAngles, aimTarget.smooth);
		}

		// Calculate the final angle
		QAngle angDelta = QAngleSub(localAngle, angAimAngles);
		QAngleNormalize(&angDelta);

		// Convert the angle into 2D positions
		res.x = (INT16)int_from_fix32(fix32_div(angDelta.y, fix32_022));
		res.y = (INT16)int_from_fix32(fix32_div(fix32_mul(angDelta.x, fix32_neg_one), fix32_022));
	}
	else
	{
		res.x = 0;
		res.y = 0;
		res.found = FALSE;
	}

	// Return the w2Mouse struct, if no target was found it is empty
	return res;
}

// Main Functions

/*
* Function:  CheatCs2Main
* --------------------
*  Main cheat loop, will keep the entitylist up-to-date and perform the activated cheat functions.
*
*  returns:	Nothing
*
*/
VOID EFIAPI CheatCs2Main()
{
	// Verify we found all neccessary modules & offsets
	if (cs2.dirBase == 0 || client.baseAddress == 0)
	{
		// Wrong initialization
		LOG_ERROR("[CS2] Failed init\r\n");

		// Not fully initialized
		return;
	}

	// Increase the reset counter every execution
	resetCounter = resetCounter + 1;

	// Reset the whole array and caching every 5000 executions
	if (resetCounter >= 5000)
	{
		// Reset the end and null the array
		arrayEnd = 0;
		nullBuffer((EFI_PHYSICAL_ADDRESS)playersGlobal, sizeof(CS2Player) * 16);
		readIndex = 1;
		resetCounter = 0;

		// Reset the memory caching
		resetCaching();
	}

	// First get current localplayer details
	CS2Local localPlayer;
	nullBuffer((EFI_PHYSICAL_ADDRESS)&localPlayer, sizeof(CS2Local));
	if (getLocalPlayer(&localPlayer) == FALSE)
	{
		// Failed dumping localplayer, maybe not ingame yet
		return;
	}

	// Get 2 player entities per execution (minimize time in SMM, not required to update every entity each execution)
	for (int i = readIndex; i < (readIndex + readNumber); i++)
	{
		CS2Player tempPlayer;

		// Null it explicitly to be safe...
		nullBuffer((EFI_PHYSICAL_ADDRESS)&tempPlayer, sizeof(CS2Player));

		if (getPlayerByIndex(&tempPlayer, i, &localPlayer, FALSE)) // TRUE for Update Teammates
		{
			// Found a successfull entity, check if it already exists in our global array
			BOOLEAN matchedEntry = FALSE;
			for (int j = 0; j < arrayEnd; j++)
			{
				// Prevent array overload
				if (j >= 16)
				{
					break;
				}

				// Already exists! Update this entry
				if (playersGlobal[j].index == tempPlayer.index)
				{
					// Already exists, replace the entity in the array
					pMemCpy((EFI_PHYSICAL_ADDRESS)&playersGlobal[j], (EFI_PHYSICAL_ADDRESS)&tempPlayer, sizeof(CS2Player));
					matchedEntry = TRUE;
					break;
				}
			}

			// Check if we found a match
			if (matchedEntry == FALSE)
			{
				// Doesnt exist yet, add it at the end
				pMemCpy((EFI_PHYSICAL_ADDRESS)&playersGlobal[arrayEnd], (EFI_PHYSICAL_ADDRESS)&tempPlayer, sizeof(CS2Player));
				arrayEnd = arrayEnd + 1;
			}
		}
		else
		{
			// Failed to update this entity, check if it exists in the global entitylist
			for (int j = 0; j < arrayEnd; j++)
			{
				// Check if the cached entity has same index as i (i would be tempPlayer->index if it passed the pawn and controller check)
				if (playersGlobal[j].index == i)
				{
					// Entity is still cached, we should remove it to prevent shooting at empty spaces...
					nullBuffer((EFI_PHYSICAL_ADDRESS)&playersGlobal[j], sizeof(CS2Player));
					break;
				}
			}
		}
	}

	// Increase readIndex after reading
	readIndex = readIndex + readNumber;

	// If we've found 10 total enemys or passed the entitylist we reset the read index
	if (readIndex >= 18)
	{
		// Set the readIndex to 0 so it reads from the start again
		readIndex = 1;
	}

	// Reset the array as soon as we hit 7 entity (changed server or smth)
	if (arrayEnd >= 7)
	{
		// Reset the end and null the array
		arrayEnd = 0;
		nullBuffer((EFI_PHYSICAL_ADDRESS)playersGlobal, sizeof(CS2Player) * 16);
		readIndex = 1;
		resetCounter = 0;
	}

	// Get current states of both transaction descriptors we have collected
	// transaction descriptor 1
	UINT8 xhciKeyState1 = GetButtonXHCI1();

	// transaction descriptor 2
	UINT8 xhciKeyState2 = GetButtonXHCI2();

	UINT8 combinedKeyState = xhciKeyState1 | xhciKeyState2;

	// Check if Aimbot is enabled, if yes check if aimbotKey is pressed and we have an enabled weapon equipped and we have ammo
	if (ENABLE_AIM == TRUE && (combinedKeyState & aimbotKey) != 0 && (localPlayer.weaponType & aimbotWeaponTypes) != 0 && localPlayer.weaponAmmo != 0)
	{
		// Check if we already have a target
		if (gAimTarget.index == NO_TARGET)
		{
			// No target found, search for a new one
			fix32_t fovIncrease = fix32_from_int(gFovIncrease);
			gAimTarget = findNewAimbotTarget(localPlayer, playersGlobal, arrayEnd, fovIncrease);
		}

		LOG_DBG("[CS2] AT: %d, H: %d\r\n", gAimTarget.index, gAimTarget.hitbox);

		// Aim at Target if found
		if (gAimTarget.index > NO_TARGET)
		{
			// Create variable to store target
			w2Mouse mouseMove = { .x = 0, .y = 0, .found = FALSE };

			// get coordinates of the target
			mouseMove = aimAtTarget(localPlayer, playersGlobal, arrayEnd, gAimTarget, TRUE);

			// Check incase something fails
			if (mouseMove.found == FALSE)
			{
				// Reset targetIndex and go to end
				gAimTarget.index = NO_TARGET;
				gFovIncrease = 0;
				gLastPunch.x = fix32_zero;
				gLastPunch.y = fix32_zero;
				gLastPunch.z = fix32_zero;
			}
			else
			{
				if (gFovIncrease <= 16)
				{
					gFovIncrease += 1;
				}

				// Move mouse
				LOG_DBG("[CS2] MO X: %d Y: %d\r\n", mouseMove.x, mouseMove.y);

				// Another check as newer usb packet might have arrived, check if key is really pressed
				if (((xhciKeyState1 & 1) != 0 || (xhciKeyState2 & 1) != 0) && !((xhciKeyState1 & 0x10) != 0 || (xhciKeyState2 & 0x10) != 0))
				{
					// Move mouse
					MoveMouseXHCI(mouseMove.x, mouseMove.y, FALSE);
				}
			}
		}
	}
	else if (ENABLE_SOUND == TRUE)
	{
		// Reset so it searches on next run
		gAimTarget.index = NO_TARGET;
		gFovIncrease = 0;
		gLastPunch.x = fix32_zero;
		gLastPunch.y = fix32_zero;
		gLastPunch.z = fix32_zero;

		// Sound ESP Logic
		soundTarget soundResult;
		soundResult = findNewSoundTarget(localPlayer, playersGlobal, arrayEnd);

		// Check if a target was found
		if (soundResult.found == TRUE)
		{
			// Play sound
			BeepXHCI(soundResult.left, soundResult.right, 2);
		}
	}

	return;
}

/*
* Function:  InitCheatCS2
* --------------------
*  Initializes the cs2 cheat by getting the modules and running signature checks.
*
*  returns:	TRUE if everything worked, FALSE if it failed to initialize.
*
*/
BOOLEAN EFIAPI InitCheatCS2()
{
	// Search for process "cs2.exe"
	BOOLEAN status = FALSE;

	// Get cs2 process details
	WinProc process;
	status = dumpSingleProcess(winGlobal, "cs2.exe", &process);

	if (status == FALSE)
	{
		LOG_ERROR("[CS2] CS2 not found\r\n");

		// Failed finding cs2, abort
		return FALSE;
	}
	else
	{
		// Save details in our global variable
		cs2.dirBase = process.dirBase;
		cs2.physProcess = process.physProcess;
		cs2.process = process.process;
		cs2.size = process.size;
		cs2.vadRoot = process.vadRoot;

		// Found cs2
		LOG_INFO("[CS2] Found CS2, DirBase %p\r\n", cs2.dirBase);
	}

	// Found cs2 and the needed information (dirbase, vadroot etc.)

	// Search for the needed modules now
	// Module: client.dll
	client.name = "client.dll";
	status = dumpSingleModule(winGlobal, &cs2, &client);

	if (status == FALSE)
	{
		LOG_ERROR("[CS2] Failed client.dll\r\n");

		return FALSE;
	}

	LOG_INFO("[CS2] Module client.dll, BaseAddress %p, size: %d\r\n", client.baseAddress, client.sizeOfModule);

	// If we get here, we've found all modules we need for the cheat to work
	// Start searching for global pointers

	// Pointer: Entitylist
	ofsDwEntityList = virtFindPattern(client.baseAddress, client.sizeOfModule, cs2.dirBase, sig_dwEntityList, 0x0, TRUE, FALSE);

	if (ofsDwEntityList == 0)
	{
		// Failed finding EntityList
		LOG_ERROR("[CS2] Failed finding EntityList\r\n");

		return FALSE;
	}

	LOG_INFO("[CS2] dwEntitylist: %p\r\n", ofsDwEntityList);

	// Pointer: dwLocalPlayerController - Shoutout to khook for this sig
	ofsDwLocalPlayerController = virtFindPattern(client.baseAddress, client.sizeOfModule, cs2.dirBase, sig_dwLocalPlayerController, 0x0, TRUE, FALSE);

	if (ofsDwLocalPlayerController == 0)
	{
		// Failed finding localplayercontroller
		LOG_ERROR("[CS2] Failed finding localplayercontroller\r\n");

		return FALSE;
	}

	LOG_INFO("[CS2] dwLocalPlayerController: %p\r\n", ofsDwLocalPlayerController);

	// Pointer: dwCSInput
	ofsDwCSInput = virtFindPattern(client.baseAddress, client.sizeOfModule, cs2.dirBase, sig_dwCSInput, 0x0, TRUE, FALSE);

	if (ofsDwCSInput == 0)
	{
		// Failed finding CSInput
		LOG_ERROR("[CS2] Failed finding CSInput\r\n");

		return FALSE;
	}

	LOG_INFO("[CS2] dwCSInput: %p\r\n", ofsDwCSInput);

	// Null global entitylist for further use
	nullBuffer((EFI_PHYSICAL_ADDRESS)playersGlobal, sizeof(CS2Player) * 16);

	// Initialize the bone min and max vectors
	headBoneMin.x = 0xffffffff00000000;		// -1.f
	headBoneMin.y = 0x1cccccc00;			// 1.8f
	headBoneMin.z = 0x0;					// 0.f

	headBoneMax.x = 0x380000000;			// 3.5f
	headBoneMax.y = 0x33333340;				// 0.2f
	headBoneMax.z = 0x0;					// 0.f

	pelvisBoneMin.x = 0xfffffffd4ccccbff;	// -2.7f
	pelvisBoneMin.y = 0x119999a00;			// 1.1f
	pelvisBoneMin.z = 0xfffffffccccccbff;	// -3.2f

	pelvisBoneMax.x = 0xfffffffd4ccccbff;	// -2.7f
	pelvisBoneMax.y = 0x119999a00;			// 1.1f
	pelvisBoneMax.z = 0x333333400;			// 3.2f

	// Successfully initialized cheat for cs2
	LOG_INFO("[CS2] Initialized! \r\n");

	return TRUE;
}