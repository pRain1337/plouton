/*++
*
* Contains general definitions for functions for signature scanning
*
--*/

#ifndef __plouton_sigscan_h__
#define __plouton_sigscan_h__

// our includes
#include "memoryMapUEFI.h"
#include "memory.h"

// Structures

// Definitions

// General definitions of types of signature scans
enum
{
	SIGTYPE_NORMAL = 0x0, // normal
	SIGTYPE_READ = 0x1, // rpm at pattern
	SIGTYPE_SUBTRACT = 0x2, // subtract img base
};

#define INRANGE(x,a,b)		(x >= a && x <= b) 
#define getBits( x )		(INRANGE(x,'0','9') ? (x - '0') : ((x&(~0x20)) - 'A' + 0xa))
#define getByte( x )		(getBits(x[0]) << 4 | getBits(x[1]))

// Functions

/*
* Function:  CompareBytes
* --------------------
* Searches for a given pattern in a given memory buffer pointed to by a pointer.
*/
BOOLEAN CompareBytes(const unsigned char* bytes, const char* pattern);

/*
* Function:  virtFindPattern
* --------------------
* Performs a pattern scan on virtual memory for a specific signature.
*/
EFI_VIRTUAL_ADDRESS virtFindPattern(EFI_VIRTUAL_ADDRESS base, UINT64 size, UINT64 dirBase, const char* pattern, UINT32 extra, BOOLEAN ripRelative, BOOLEAN substract);

/*
* Function:  physFindPattern
* --------------------
* Performs a pattern scan on physical memory for a specific signature.
*/
EFI_PHYSICAL_ADDRESS physFindPattern(EFI_PHYSICAL_ADDRESS base, UINT32 size, const unsigned char* pattern, UINT32 patternLength);

#endif