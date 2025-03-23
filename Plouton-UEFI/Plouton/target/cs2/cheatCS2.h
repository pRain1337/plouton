/*++
*
* Header file for the functions used for the cs2 cheat
*
--*/

#ifndef __plouton_cheat_cs2_h__
#define __plouton_cheat_cs2_h__

// our includes
#include "math.h"
#include "offsetsCS2.h"
#include "structuresCS2.h"
#include "../../memory/sigScan.h"
#include "../../os/windows/windows.h"
#include "../../os/windows/NTKernelTools.h"
#include "../../hardware/serial.h"
#include "../../floats/floatlib.h"
#include "../../hardware/xhci.h"

// Structures

// General structures

// Game Structures

// Type that is assigned per weapon for different settings
typedef enum {
	None	= 0x0,
	Rifle	= 0x1,
	Pistol	= 0x2,
	Sniper	= 0x4,
	SMG		= 0x8
} WeaponType;

// Aimbot stuff
typedef enum {
	Pelvis,
	Chest,
	Head
} AimbotHitbox;

typedef struct
{
	BOOLEAN       found;
	INT32         index;
	AimbotHitbox  hitbox;
	fix32_t       smooth;
} aimbotTarget;

typedef struct _w2Mouse
{
	INT16 x;
	INT16 y;
	BOOLEAN found;
} w2Mouse, * Pw2Mouse;

// Sound ESP stuff
typedef struct _soundTarget
{
	INT16 left;
	INT16 right;
	BOOLEAN found;
} soundTarget, * PsoundTarget;

// Localplayer Structures
typedef struct _CS2Local {
	UINT64					pawn;                   // Pointer to the localplayer pawn structure
	UINT64					controller;             // Pointer to the localplayer controller structure
	UINT32				    team;                   // Team ID of the localplayer
	QVector					viewAngles;				// Viewangles of the localplayer
	UINT64                  gameScene;				// Pointer to the game scene structure
	Vector					origin;                 // Origin Position of the localplayer
	Vector					vecViewOffset;          // Relative position of the eye of the localplayer
	QVector					cameraPosition;			// Camera Position of the localplayer
	UINT64                  activeWeapon;			// Pointer to the weapon structure of the localplayer
	UINT32                  weaponAmmo;				// If the weapon has ammo currently or is empty
	UINT16                  weaponId;				// ID of the weapon that is currently held by the localplayer
	WeaponType              weaponType;				// Type of the current weapon of the localplayer
	CS2CUtlVector           punchCache;				// Pointer to the last cache of the weapon punch
	Vector					punchAngles;			// Actual punch of the current weapon
	QAngle					recoilAngles;			// Already calculated angles with the included punch for RCS
} CS2Local, * PCS2Local;

// Player structures
typedef struct _CS2Player {
	UINT64					pawn;                   // Pointer to the player pawn structure
	UINT64					controller;             // Pointer to the player controller structure
	UINT32                  index;                  // Index of the player entity for identification
	UINT32                  health;                 // Health of the player entity
	BOOLEAN                 alive;                  // Alive status of the entity
	BOOLEAN                 immune;                 // Immune status of the entity (in warmup)
	UINT32				    team;                   // Team ID of the entity
	BOOLEAN                 spotted;                // Spotted status of the entity for the radar
	UINT64                  gameScene;				// Pointer to the game scene structure
	UINT64                  boneArray;				// Pointer to the bone array structure
	QVector					bonePositionHead;		// Vector containing the 3d location of the entity head
	QVector					bonePositionPelvis;		// Vector containing the 3d location of the entity stomach
	QVector					velocity;               // Velocity of the current moving entity
	INT32                   yawRotation;			// Current yaw (Y) rotation of the entity
} CS2Player, * PCS2Player;

// Definitions

// Functions

// Main Functions   

/*
* Function:  CheatCs2Main
* --------------------
* Basic cheat loop that calls the update functions and performs the cheat functionalities.
*/
VOID EFIAPI CheatCs2Main();

/*
* Function:  InitCheatCS2
* --------------------
* Searches and initializes the basic functions (process, modules and sig scans)
*/
BOOLEAN EFIAPI InitCheatCS2();                                                                                                         

// Feature Functions

/*
* Function:  findNewAimbotTarget
* --------------------
* Function that will find the nearest aimbot target and return it.
*/
aimbotTarget findNewAimbotTarget(CS2Local localplayer, CS2Player* players, UINT8 playerAmount, fix32_t headIncrease);

/*
* Function:  findNewSoundTarget
* --------------------
* Function that will find the nearest sound ESP target and return it
*/
soundTarget findNewSoundTarget(CS2Local localplayer, CS2Player* players, UINT8 playerAmount);

/*
* Function:  aimAtTarget
* --------------------
* Verifies that the target is still up-to-date and calculates the 2D Coordinates that should be moved to aim at at the player.
*/
w2Mouse aimAtTarget(CS2Local localplayer, CS2Player* players, UINT8 playerAmount, aimbotTarget aimTarget, BOOLEAN useSmooth);

// Localplayer Functions                                    

/*
* Function:  getLocalPlayer
* --------------------
* Gets the localplayer structure and writes it into the supplied pointer
*/
BOOLEAN getLocalPlayer(PCS2Local localplayer);

// Player functions

/*
* Function:  getPlayerByIndex
* --------------------
* Reads the player entity at the given index and returns it
*/
BOOLEAN getPlayerByIndex(PCS2Player player, UINT16 index, PCS2Local localplayer, BOOLEAN updateTeammates); 

/*
* Function:  getPlayerBoneTransform
* --------------------
* Initializes the CTransform struct with the data of the bone of the player.
*/
BOOLEAN getPlayerBoneTransform(CTransform* pCTransform, EFI_VIRTUAL_ADDRESS transform_pointer);

/*
* Function:  getPlayerPawn
* --------------------
* Returns the address of the pawn of the player based on the controller pointer.
*/
EFI_VIRTUAL_ADDRESS getPlayerPawn(EFI_VIRTUAL_ADDRESS controller);

/*
* Function:  getPlayerController
* --------------------
* Returns the address of the controller of the given entity index
*/
EFI_VIRTUAL_ADDRESS getPlayerController(UINT16 index);

// Calculation functions

/*
* Function:  extrapolatePosition
* --------------------
* Calculates the extrapolated position based on current position and active velocity
*/
QVector extrapolatePosition(QVector position, QVector velocity);

// Weapon type functions

/*
* Function:  checkRifle
* --------------------
* Verifies if the given weapon ID is a rifle.
*/
BOOLEAN checkRifle(int weaponid);

/*
* Function:  checkPistol
* --------------------
* Verifies if the given weapon ID is a pistol.
*/
BOOLEAN checkPistol(int weaponid);

/*
* Function:  checkSniper
* --------------------
* Verifies if the given weapon ID is a sniper.
*/
BOOLEAN checkSniper(int weaponid);

/*
* Function:  checkSMG
* --------------------
* Verifies if the given weapon ID is a SMG.
*/
BOOLEAN checkSMG(int weaponid);

#endif