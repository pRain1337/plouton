/*++
*
* Source file for memory map related functions
*
--*/

// our includes
#include "memoryMapUEFI.h"
#include "../logging/logging.h"

// Global memory map related variables
EFI_MEMORY_DESCRIPTOR*    mUefiMemoryMap;
UINTN                     mUefiMemoryMapSize;
UINTN                     mUefiDescriptorSize;

/*
 * Function:  CheckUefiMemoryMap
 * --------------------
 * Checks if the passed memory map has atleast 3 valid entries.
 *
 *
 *  returns: Returns true if there is more than 3 entries, otherwise 0
 *
 */
BOOLEAN CheckUefiMemoryMap(EFI_MEMORY_DESCRIPTOR* MemoryMap, UINT64 MemoryMapSize, UINT64 MemoryMapDescriptorSize)
{
	// Used to count the valid pages
	UINTN Counter = 0;

	// Check if we even got valid parameters passed
	if (MemoryMap != NULL && MemoryMapSize != 0 && MemoryMapDescriptorSize != 0)
	{
		// Set the start of the memory map as entry
		EFI_MEMORY_DESCRIPTOR* CurMemoryMap = MemoryMap;

		// Calculate the amount of entries by dividing size by descriptor size
		UINTN MemoryMapEntryCount = MemoryMapSize / MemoryMapDescriptorSize;

		// Go through all entries
		for (UINTN Index = 0; Index < MemoryMapEntryCount; Index++)
		{
			// Check if the entry has a physical start and number of pages different than 0
			if ((CurMemoryMap->PhysicalStart != 0) || (CurMemoryMap->NumberOfPages != 0))
			{
				// Valid page, increase counter
				Counter += 1;
			}

			// Set 'CurMemoryMap' to the next entry
			CurMemoryMap = NEXT_MEMORY_DESCRIPTOR(CurMemoryMap, MemoryMapDescriptorSize);
		}
	}

	// Check if there were more than 3 entries
	if (Counter >= 3)
	{
		// Yes, return true
		return TRUE;
	}

	// Less than 3 entries or invalid parameters, return false
	return FALSE;
}

/*
 * Function:  SortMemoryMap
 * --------------------
 * Sorts the passed memory map for nicer presentation
 *
 *  MemoryMap:		Pointer to the existing memory map that should be sorted
 *  MemoryMapSize:	Size of the memory map
 *  DescriptorSize:	Size of a single descriptor in the memory map
 *
 *  returns:	Nothing
 *
 */
VOID SortMemoryMap(IN OUT EFI_MEMORY_DESCRIPTOR* MemoryMap, IN UINTN MemoryMapSize, IN UINTN DescriptorSize)
{
	// Variable used to copy between memory map entries
	EFI_MEMORY_DESCRIPTOR  TempMemoryMap = { 0, 0, 0, 0, 0 };

	// Calculate the end of the memory map
	const EFI_MEMORY_DESCRIPTOR* MemoryMapEnd = (EFI_MEMORY_DESCRIPTOR*)((UINT8*)MemoryMap + MemoryMapSize);
	
	// Select the first entry
	EFI_MEMORY_DESCRIPTOR* MemoryMapEntry = MemoryMap;

	// Get the first entry in the memory map
	EFI_MEMORY_DESCRIPTOR* NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR(MemoryMapEntry, DescriptorSize);

	// Go through the memory map entry until the entry would be out of the memory map size
	while (MemoryMapEntry < MemoryMapEnd)
	{
		// Check if the next memory map entry would be out of the memory map to prevent overflows
		while (NextMemoryMapEntry < MemoryMapEnd)
		{
			// Check if the current memory map entry is bigger than the next entry
			if (MemoryMapEntry->PhysicalStart > NextMemoryMapEntry->PhysicalStart)
			{
				// Its bigger, swap places
				pMemCpyForce((EFI_PHYSICAL_ADDRESS)&TempMemoryMap, (EFI_PHYSICAL_ADDRESS)MemoryMapEntry, sizeof(EFI_MEMORY_DESCRIPTOR));
				pMemCpyForce((EFI_PHYSICAL_ADDRESS)MemoryMapEntry, (EFI_PHYSICAL_ADDRESS)NextMemoryMapEntry, sizeof(EFI_MEMORY_DESCRIPTOR));
				pMemCpyForce((EFI_PHYSICAL_ADDRESS)NextMemoryMapEntry, (EFI_PHYSICAL_ADDRESS)&TempMemoryMap, sizeof(EFI_MEMORY_DESCRIPTOR));
			}

			// Set the current memory map entry to the next memory map entry
			NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR(NextMemoryMapEntry, DescriptorSize);
		}

		// We reached the end. Set current and next memory map entry to the same value
		MemoryMapEntry = NEXT_MEMORY_DESCRIPTOR(MemoryMapEntry, DescriptorSize);
		NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR(MemoryMapEntry, DescriptorSize);
	}

	return;
}

/*
 * Function:  MergeMemoryMapForNotPresentEntry
 * --------------------
 * Removes specific entries from the memory map which should not be touched as they are not present.
 *
 *  MemoryMap:		Pointer to the existing memory map that should be sorted
 *  MemoryMapSize:	Size of the memory map
 *  DescriptorSize:	Size of a single descriptor in the memory map
 *
 *  returns:	Nothing
 *
 */
VOID MergeMemoryMapForNotPresentEntry(IN OUT EFI_MEMORY_DESCRIPTOR* MemoryMap, IN OUT UINTN* MemoryMapSize, IN UINTN DescriptorSize)
{
	// Define some variables used for parsing
	UINT64                 MemoryBlockLength;
	EFI_MEMORY_DESCRIPTOR* NextMemoryMapEntry;

	// Calculate the end of the memory map
	const EFI_MEMORY_DESCRIPTOR* MemoryMapEnd = (EFI_MEMORY_DESCRIPTOR*)((UINT8*)MemoryMap + *MemoryMapSize);

	// Get the first entry in the memory map
	EFI_MEMORY_DESCRIPTOR* MemoryMapEntry = MemoryMap;
	EFI_MEMORY_DESCRIPTOR* NewMemoryMapEntry = MemoryMap;

	// Go through all entries until we are out of the memory map size
	while ((UINTN)MemoryMapEntry < (UINTN)MemoryMapEnd)
	{
		pMemCpyForce((EFI_PHYSICAL_ADDRESS)NewMemoryMapEntry, (EFI_PHYSICAL_ADDRESS)MemoryMapEntry, sizeof(EFI_MEMORY_DESCRIPTOR));

		// Get the next memory map entry
		NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR(MemoryMapEntry, DescriptorSize);

		do
		{
			// Get the size of the memory map entry
			MemoryBlockLength = (UINT64)(EFI_PAGES_TO_SIZE((UINTN)MemoryMapEntry->NumberOfPages));

			// Make sure the next memory map entry is still inbound of the memory map and the current memory map entry address ends where the next memory map entry address starts
			if (((UINTN)NextMemoryMapEntry < (UINTN)MemoryMapEnd) && ((MemoryMapEntry->PhysicalStart + MemoryBlockLength) == NextMemoryMapEntry->PhysicalStart))
			{
				// End is equal to start of next entry -> we can merge them together to get a smaller memory map
				MemoryMapEntry->NumberOfPages += NextMemoryMapEntry->NumberOfPages;

				// Make sure the next memory map entry is not the current memory map entry
				if (NewMemoryMapEntry != MemoryMapEntry)
				{
					// Now increase the size in the current entry
					NewMemoryMapEntry->NumberOfPages += NextMemoryMapEntry->NumberOfPages;
				}

				// Get the next memory map entry to continue this process
				NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR(NextMemoryMapEntry, DescriptorSize);
				continue;
			}
			else
			{
				// Cant merge the entries -> get the next entry
				MemoryMapEntry = PREVIOUS_MEMORY_DESCRIPTOR(NextMemoryMapEntry, DescriptorSize);
				break;
			}
		} while (TRUE);

		// Current entries cant be merged, get next entries and repeat process
		MemoryMapEntry = NEXT_MEMORY_DESCRIPTOR(MemoryMapEntry, DescriptorSize);
		NewMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR(NewMemoryMapEntry, DescriptorSize);
	}

	// Update the memory map size as we changed it due to the merges
	*MemoryMapSize = (UINTN)NewMemoryMapEntry - (UINTN)MemoryMap;

	return;
}

/*
 * Function:  InitUefiMemoryMap
 * --------------------
 * Gets the current memory map and writes it to the pointer.
 *
 *  NewMemoryMap:	Pointer to where the memory map should be written to
 *
 *  returns:	TRUE if we got a working memory map
 *
 */
BOOLEAN InitUefiMemoryMap(UINT64 NewMemoryMap)
{
	// Declare some variables used for parsing
	UINTN MemoryMapSize = 0;
	EFI_MEMORY_DESCRIPTOR* MemoryMap = NULL;
	UINTN LocalMapKey;
	UINT32 DescriptorVersion;

	EFI_STATUS status = EFI_SUCCESS;

	// Use gBS->GetMemoryMap to get the physical memory map from the DXE service
	status = gBS->GetMemoryMap(&MemoryMapSize, MemoryMap, &LocalMapKey, &mUefiDescriptorSize, &DescriptorVersion);

	// As we dont know the size in advance, call the function until we got the right size as long as we get the error EFI_BUFFER_TOO_SMALL
	do {
		// Allocate a new memory map using the MemoryMapSize we have gotten from calling gBS->GetMemoryMap
		status = gBS->AllocatePool(EfiBootServicesData, MemoryMapSize, (VOID**)&MemoryMap);

		// Check if we successfully allocated memory
		if (MemoryMap == NULL || status != EFI_SUCCESS)
		{
			return FALSE;
		}

		// Now call gBS->GetMemoryMap again with a big enough buffer
		status = gBS->GetMemoryMap(&MemoryMapSize, MemoryMap, &LocalMapKey, &mUefiDescriptorSize, &DescriptorVersion);

		// Check if it still fails
		if (EFI_ERROR(status))
		{
			// It still failed for some reasons... Free it and return NULL
			gBS->FreePool(MemoryMap);
			MemoryMap = NULL;
		}

	} while (status == EFI_BUFFER_TOO_SMALL);

	if (MemoryMap == NULL)
	{
		LOG_ERROR("Failed init memory map! \r\n");

		return FALSE;
	}

	// Now we got a memory map, first sort it!
	SortMemoryMap(MemoryMap, MemoryMapSize, mUefiDescriptorSize);

	// As the memory map is sorted, we can now try merging entries to make it smaller
	MergeMemoryMapForNotPresentEntry(MemoryMap, &MemoryMapSize, mUefiDescriptorSize);

	// Due to the merging the size has changed, also reflect it in the global variable
	mUefiMemoryMapSize = MemoryMapSize;

	// Now copy the memory map from the temporary location to the final location passed in the function parameters
	pMemCpyForce(NewMemoryMap, (UINT64)MemoryMap, MemoryMapSize);

	// Now set the location in the global variable
	mUefiMemoryMap = (EFI_MEMORY_DESCRIPTOR*)NewMemoryMap;

	// Finally deallocate the pages for the temporary memory map
	gBS->FreePool(MemoryMap);

	return TRUE;
}