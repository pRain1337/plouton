/*++
*
* Header file for offsets required for cs2 cheat
*
--*/

#ifndef __plouton_offsets_cs2_h__
#define __plouton_offsets_cs2_h__

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

// Structures

// Definitions

// Signatures
static const char* sig_dwEntityList = "48 89 35 ? ? ? ? 48 85 F6";
static const char* sig_dwLocalPlayerController = "48 89 05 ? ? ? ? 8B 9E";
static const char* sig_dwCSInput = "48 8B 0D ? ? ? ? 48 8B 01 FF 50 ? 8B DF";

// Hardcoded offsets       
/* --- https://github.com/a2x/cs2-dumper/blob/main/generated/client.dll.hpp --- */
static UINT64 ofs_m_pGameSceneNode                     = 0x328;					// C_BaseEntity             -> m_pGameSceneNode
static UINT64 ofs_m_iHealth                            = 0x344;					// C_BaseEntity             -> m_iHealth
static UINT64 ofs_m_iTeamNum                           = 0x3E3;					// C_BaseEntity             -> m_iTeamNum
static UINT64 ofs_m_vecVelocity                        = 0x400;					// C_BaseEntity             -> m_vecVelocity

static UINT64 ofs_m_bPawnIsAlive                       = 0x82C;					// CCSPlayerController      -> m_bPawnIsAlive

static UINT64 ofs_m_pClippingWeapon                    = 0x13A0;				// C_CSPlayerPawnBase       -> m_pClippingWeapon
static UINT64 ofs_m_bGunGameImmunity                   = 0x13BC;				// C_CSPlayerPawnBase       -> m_bGunGameImmunity
static UINT64 ofs_m_angEyeAngles                       = 0x1438;				// C_CSPlayerPawnBase       -> m_angEyeAngles
static UINT64 ofs_m_iIDEntIndex                        = 0x1458;				// C_CSPlayerPawnBase       -> m_iIDEntIndex
static UINT64 ofs_m_entitySpottedState                 = 0x23D0;				// C_CSPlayerPawn           -> m_entitySpottedState

static UINT64 ofs_m_iClip1                             = 0x1678;				// C_BasePlayerWeapon       -> m_iClip1
static UINT64 ofs_m_AttributeManager                   = 0x1148;				// C_EconEntity             -> m_AttributeManager
static UINT64 ofs_m_Item                               = 0x50;					// C_AttributeContainer     -> m_Item
static UINT64 ofs_m_iItemDefinitionIndex               = 0x1BA;					// C_EconItemView           -> m_iItemDefinitionIndex
static UINT64 ofs_m_aimPunchCache                      = 0x15A8;				// C_CSPlayerPawn           -> m_aimPunchCache
static UINT64 ofs_m_modelState                         = 0x170;					// CSkeletonInstance        -> m_modelState
static UINT64 ofs_m_bSpottedByMask                     = 0xC;					// EntitySpottedState_t     -> m_bSpottedByMask
static UINT64 ofs_m_vecAbsOrigin                       = 0xD0;					// CGameSceneNode           -> m_vecAbsOrigin
static UINT64 ofs_m_hPawn                              = 0x62C;					// CBasePlayerController    -> m_hPawn
static UINT64 ofs_m_vecViewOffset                      = 0xCB0;					// C_BaseModelEntity        -> m_vecViewOffset

/* --- Hardcoded gift from kidua --- 
*
*   Found in the function that passes the csinput ptr
*   Pattern: E8 ? ? ? ? EB 0E 48 8B 01
*
*/
static UINT64 ofs_viewAngles                           = 0x3D0;

// Functions

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#endif