/*++
*
* Source file for string/memory related functions
*
--*/

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif

// our includes
#include "string.h"

/*
* Function:  toLower
* --------------------
* Converts the given char to lowercase, used for string comparisons.
*
*  c:				Char to convert to lowercase
*
*  returns:	
*
* Credits to: https://github.com/Oliver-1-1/SmmInfect/blob/main/SmmInfect/string.c#L3
* 
*/
INT32 toLower(INT32 c)
{
	return (c >= 'A' && c <= 'Z') ? (c + 'a' - 'A') : c;
}

/*
* Function:  strcmp
* --------------------
* Compares two strings and returns the location of the difference.
*
*  str1:			Pointer to string number one
*  str2:			Pointer to string number two
*
*  returns:	Where a difference happened. 0 if no difference.
*
*/
INT32 strcmp(const CHAR8* str1, const CHAR8* str2)
{
	// Increase both string positions and check if they are the same
	for (; toLower(*str1) == toLower(*str2); ++str1, ++str2)
		// Check if string one has already ended -> comparison complete with no difference, return 0
		if (*str1 == 0)
			return 0;

	// For loop ended before null-terminator of string one was reached, return the relative position of the difference
	return *(UINT8*)str1 < *(UINT8*)str2 ? -1 : 1;
}

// Disable optimizations so it doesnt get replaced by memset
#pragma optimize("", off)

/*
* Function:  nullBuffer
* --------------------
* Compares two strings and returns the location of the difference.
*
*  buffer:			UINT64 Pointer to a memory buffer that should be nulled
*  size:			UINT16 size of the memory buffer
*
*  returns:	Nothing.
*
*/
void nullBuffer(EFI_PHYSICAL_ADDRESS buffer, UINT16 size)
{
	UINT8* pListChar = (UINT8*)buffer;
	for (int i = 0; i < size; i++) 
	{ 
		// Null each byte
		pListChar[i] = 0; 
	}
}

// Enable optimizations again
#pragma optimize("", on)

/*
* Function:  strlen
* --------------------
* Gets the length of an ASCII string
*
*  str:			input string
*
*  returns:	Length of the input string.
*
*/
UINT32 strlen(const char* str)
{
    const char* orig = str;
    while(*(str++));
    return (size_t)(str - orig - 1);
}


#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif