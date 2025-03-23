/*++
*
* Source file for signature scanning related functions
*
--*/

// our includes
#include "sigScan.h"
#include "../logging/logging.h"

/*
* Function:  CompareBytes
* --------------------
* Goes through the passed memory buffer and checks for matches on the pattern.
* Checks if it is either a match, ' ' or '?'
*
*  bytes:			Pointer to the memory buffer to scan
*  pattern:			Pointer to the pattern that should be scanned for
*
*  returns:	FALSE if it doesnt match, true if it does match.
*
*/
BOOLEAN CompareBytes(const unsigned char* bytes, const char* pattern)
{
	// Go through the pattern and bytes equally
	for (; *pattern; *pattern != ' ' ? ++bytes : bytes, ++pattern)
	{
		// check if the pattern expects an expect match at this position
		if (*pattern == ' ' || *pattern == '?')
			continue;

		// Check if the byte position matches the pattern
		if (*bytes != getByte(pattern))
			return FALSE;

		// We matched, increase pattern for next match
		++pattern;
	}

	// No difference observed, return true
	return TRUE;
}

/*
* Function:  virtFindPattern
* --------------------
* Searches for a given pattern in the memory area passed as parameter. Contains various features for special signatures (not just memory based).
*
*  base:			UINT64 virtual memory base where it should start scanning
*  size:			UINT64 size starting from base where it should scan
*  dirBase:			UINT64 directory base which is used to translate virtual to physcical memory
*  pattern:			Pointer to the pattern that should be scanned for
*  extra:			UINT32 extra offset that should be added on the found signature
*  ripRelative:		BOOLEAN that indicates if rip relative addressing should be done (read the address and return it)
*  substract:		BOOLEAN that indicates if the base should be returned from the address (returns offset)
*
*  returns:	0 if the signature was not found or it failed. Otherwise it returns the address where it was found.
*
*/
EFI_VIRTUAL_ADDRESS virtFindPattern(EFI_VIRTUAL_ADDRESS base, UINT64 size, UINT64 dirBase, const char* pattern, UINT32 extra, BOOLEAN ripRelative, BOOLEAN substract)
{
	// Check if one of the basic parameters (base, size, dirbase) were not properly set
	if (base == 0 || size == 0 || dirBase == 0)
	{
		LOG_ERROR("IPS\r\n");

		return 0;
	}

	// Result will be stored in this variable
	EFI_VIRTUAL_ADDRESS address = 0;

	// Calculate amount of pages that should be scanned
	UINT32 pageAmount = (size / 0x1000) + 1;

	// Temporarily store the memory in SMM
	unsigned char buffer[0x2000];

	// Now loop through all pages
	for (UINT32 i = 0; i < pageAmount; i++)
	{
		// Convert the virtual addresses to physical addresses
		EFI_PHYSICAL_ADDRESS pSrc1 = VTOPForce(base + (i * 0x1000), dirBase);
		EFI_PHYSICAL_ADDRESS pSrc2 = VTOPForce(base + ((i + 1) * 0x1000), dirBase);

		// Check if we successfully converted it
		if (pSrc1 == 0 || pSrc2 == 0)
		{
			// Failed converting, check the whole supplied region to be sure
			continue;
		}
		else
		{
			// Valid address, copy the memory to SMM
			pMemCpy((EFI_PHYSICAL_ADDRESS)buffer, pSrc1, 0x1000);
			pMemCpy((EFI_PHYSICAL_ADDRESS)buffer + 0x1000, pSrc2, 0x1000);

			// Compare the memory page by looping through it
			for (int j = 0; j < 0x2000; j++)
			{
				if (CompareBytes((unsigned char *)buffer + j, pattern))
				{
					// Successfully found the pattern, process it to get the correct result
					LOG_VERB("[SIG] Found pattern at %p\r\n", base + (i * 0x1000) + j);

					// Set the starting address where we found the match
					address = base + (i * 0x1000) + j;

					// Check if the ripRelative operation should be done
					if (ripRelative)
					{
						// First read the rip displacement at the location
						UINT32 displacement = 0;
						vMemRead((EFI_PHYSICAL_ADDRESS) &displacement, address + 0x3, sizeof(UINT32), dirBase);

						// Check if we successfully read something
						LOG_VERB("[SIG] Read displacement: %x\r\n", displacement);

						// Add displacement to the address
						address = address + 0x7 + displacement;
					}

					// Check if the substract operation should be done
					if (substract)
					{
						// Substract the base from the address to get the relative offset
						address -= base;
					}

					// Add the supplied extra, if set
					address += extra;

					// Remove the base again so it's an offset
					address -= base;

					// Return the result
					return address;
				}
			}
		}
	}

	// Signature not found, return 0
	return 0;
}

/*
* Function:  physFindPattern
* --------------------
*  Searches the given physical memory area for a given pattern. Only uses memory based patterns with the wildcard '0xAA'.
*
*  base:			EFI_PHYSICAL_ADDRESS base where the physical memory area starts
*  size:			UINT32 size of the physical memory area
*  pattern:			Pointer to byte array which will contain the pattern
*  patternLength:	UINT32 size of the pattern byte array
*
*  returns:	Returns the address to the pattern if found, otherwise returns 0.
*
*/
EFI_PHYSICAL_ADDRESS physFindPattern(EFI_PHYSICAL_ADDRESS base, UINT32 size, const unsigned char* pattern, UINT32 patternLength)
{
	// Define the wildcard pattern, means this position accepst any byte
	unsigned char wildcard = 0xAA;

	// Now loop through the total size of the physical memory given
	for (UINT64 i = 0; i < size - patternLength; i++)
	{
		// Defines if the pattern is at the current position. Will be set to 'false' in case of mismatching bytes.
		BOOLEAN found = TRUE;

		// Buffer to temporarily hold the memory
		unsigned char buffer[32];

		// copy the memory from the current position into our buffer
		pMemCpy((EFI_PHYSICAL_ADDRESS)buffer, base + i, 32);

		// Now go through our buffer for the length of the pattern
		for (UINT32 j = 0; j < patternLength; j++)
		{
			// Check if the current pattern position is a wildcard or if the pattern matches the memory position
			if (pattern[j] != wildcard && pattern[j] != ((unsigned char*)buffer)[j])
			{
				// No match, set found to false 
				found = FALSE;

				// Break out of the for loop as this memory position is incorrect
				break;
			}
		}

		// Check if we've already found it
		if (found != FALSE)
		{
			// Found it! Return the address based on base + current position
			return ((EFI_PHYSICAL_ADDRESS)base + i);
		}
	}

	// If we get here, it means we have found nothing. Return 0
	return 0;
}
