/*++
*
* Header file for string/memory related functions
*
--*/

#ifndef __plouton_string_h__
#define __plouton_string_h__

// edk2 includes
#include <Base.h>
#include <Uefi/UefiBaseType.h>

// Structures

// Definitions

// Definition for linux compilers
#ifdef __GNUC__
typedef UINT32 size_t;
#endif

// Functions

/*
* Function:  toLower
* --------------------
* Converts the given char to lowercase.
*/
INT32 toLower(INT32 c);

/*
* Function:  strcmp
* --------------------
* Compares two strings and returns the position where there is a difference.
*/
INT32 strcmp(const CHAR8* str1, const CHAR8* str2);

/*
* Function:  nullBuffer
* --------------------
* Nulls a passed memory area.
*/
void nullBuffer(EFI_PHYSICAL_ADDRESS buffer, UINT16 size);

/*
* Function:  strlen
* --------------------
* Returns length of an ascii string
*/
UINT32 strlen(const char* str);

#endif