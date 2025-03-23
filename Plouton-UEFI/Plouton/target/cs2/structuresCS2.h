/*++
*
* Header file for structures used in cs2
*
--*/

#ifndef __plouton_structures_cs_h__
#define __plouton_structures_cs_h__

// our includes
#include "../../floats/floatlib.h"

// These lists are used to indicate if features should be active
static const UINT8 weaponList[]  = {7, 8, 10, 13, 14, 16, 25, 27, 28, 29, 35, 39};;			// Rifle List
static const UINT8 smgList[]     = {17, 19, 23, 24, 26, 33, 34};							// smg List
static const UINT8 pistolList[]  = { 1, 2, 3, 4, 30, 32, 36, 61, 64 };						// Pistol List,
static const UINT8 sniperList[]  = {9, 11, 38, 40};											// Sniper List

// basic game data structs
typedef struct _CS2CutlVector {
	UINT64 size;                                    // Size of the vector
	UINT64 data;                                    // Pointer to the vector
} CS2CUtlVector, * PCS2CutlVector;

#endif