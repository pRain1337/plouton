/*++
*
* Source file for memory related functions
*
--*/

// our includes
#include "memory.h"
#include "string.h"
#include "../logging/logging.h"


EFI_PHYSICAL_ADDRESS cachingPointer = 0;
UINT32 cachingSize = 0;


/* Functions related to physical memory */

/*
* Function:  pMemCpy
* --------------------
* Copies the memory from the source to the destination while validating any address
*
*  dest:			EFI_PHYSICAL_ADDRESS (UINT64) address of where to copy the memory
*  src:				EFI_PHYSICAL_ADDRESS (UINT64) address of from where to copy the memory
*  size:			size_t size that defines how much memory is copied
*
*  returns:	Returns false if the address is not in the memory map, otherwise true
*
*/
BOOLEAN pMemCpy(EFI_PHYSICAL_ADDRESS dest, EFI_PHYSICAL_ADDRESS src, size_t size)
{
	// Check if the address ranges are in allowed range
	if ((IsAddressValid(src) == FALSE || IsAddressValid((src + size - 1)) == FALSE))
	{
		LOG_DBG("[MEM] Aborted duo to disallowed memory range \r\n");

		// Return to invalid memory range
		return FALSE;
	}

	// Typecast src and dest addresses to bytes
	UINT8* csrc = (UINT8*)src;
	UINT8* cdest = (UINT8*)dest;

	// Copy contents of src[] to dest[] 
	for (int i = 0; i < size; i++)
		cdest[i] = csrc[i];

	// Return true
	return TRUE;
}

/*
* Function:  pMemCpyForce
* --------------------
* Copies the memory from the source to the destination while not validating any address
*
*  dest:			EFI_PHYSICAL_ADDRESS (UINT64) address of where to copy the memory
*  src:				EFI_PHYSICAL_ADDRESS (UINT64) address of from where to copy the memory
*  size:			size_t size that defines how much memory is copied
*
*  returns:	Always returns true as it doesnt do any validity checks and just blindly copies
*
*/
BOOLEAN pMemCpyForce(EFI_PHYSICAL_ADDRESS dest, EFI_PHYSICAL_ADDRESS src, size_t n)
{
	// Check if we got valid addresses
	if (dest == 0 || src == 0 || n == 0)
	{
		LOG_DBG("[MEM] Aborted duo addresses not set \r\n");

		// Invalid parameter or we really tried to read exactly at physical 0x0000
		return FALSE;
	}

	// Typecast src and dest addresses to bytes 
	UINT8* csrc = (UINT8*)src;
	UINT8* cdest = (UINT8*)dest;

	// Copy contents of src[] to dest[] 
	for (int i = 0; i < n; i++)
		cdest[i] = csrc[i];

	return TRUE;
}

/*
* Function:  IsAddressValid
* --------------------
* Copies the memory from the source to the destination while not validating any address
*
*  address:			EFI_PHYSICAL_ADDRESS (UINT64) address to verify for validity
*
*  returns:	Returns false if the address does not exist in the memory map, otherwise true.
*
*/
BOOLEAN IsAddressValid(EFI_PHYSICAL_ADDRESS address)
{
	// check if we even got a valid address
	if (address == 0)
	{
		LOG_DBG("[MEM] Aborted duo to missing address \r\n");

		// Return false
		return FALSE;
	}

	// check if the memory map was already initialized
	if (mUefiMemoryMap != NULL)
	{
		// Look at the first entry at the start of the memory map
		EFI_MEMORY_DESCRIPTOR* MemoryMap = mUefiMemoryMap;

		// calculate the amount of entries by dividing total size with the size of a single entry
		UINTN MemoryMapEntryCount = mUefiMemoryMapSize / mUefiDescriptorSize;

		// Go through all entries
		for (UINTN Index = 0; Index < MemoryMapEntryCount; Index++)
		{
			// check if the passed address exists inbetween the start and end of this physical address
			if ((address >= MemoryMap->PhysicalStart) && (address < MemoryMap->PhysicalStart + EFI_PAGES_TO_SIZE((UINTN)MemoryMap->NumberOfPages)))
			{
				// Inbetween this memory range -> return true
				return TRUE;
			}

			// Not inbetween, get the next memory map entry
			MemoryMap = NEXT_MEMORY_DESCRIPTOR(MemoryMap, mUefiDescriptorSize);
		}
	}
	else
	{
		LOG_ERROR("[MEM] Memory map not initialized yet \r\n");
	}

	// Not found, return false
	return FALSE;
}

/* Functions related to virtual to physical translation */

/*
* Function:  VTOP
* --------------------
* Translates the given virtual address using the given directory base to the associated physical memory address while only reading validated addresses.
*
*  address:			EFI_VIRTUAL_ADDRESS (UINT64) virtual address to translate to physical
*  directoryBase:	EFI_PHYSICAL_ADDRESS (UINT64) address of the directory base to use for the translation
*
*  returns:	Returns 0 if translation failed, otherwise the physical address.
*
*/
EFI_PHYSICAL_ADDRESS VTOP(EFI_VIRTUAL_ADDRESS address, EFI_PHYSICAL_ADDRESS directoryBase)
{
	// Check if address was properly set
	if (address == 0)
	{
		LOG_DBG("[MEM] address is 0 \r\n");

		// No valid virtual address, abort
		return 0;
	}

	// check if directory base was properly set
	if (directoryBase == 0)
	{
		LOG_DBG("[MEM] directoryBase is 0 \r\n");

		// No valid directory base, abort
		return 0;
	}

	// Null the last bits
	directoryBase &= ~0xf;

	// Calculate the relevant offsets based on the address
	UINT64 pageOffset = address & ~(~0ul << PAGE_OFFSET_SIZE);
	EFI_PHYSICAL_ADDRESS pte = ((address >> 12) & (0x1ffll));
	EFI_PHYSICAL_ADDRESS pt = ((address >> 21) & (0x1ffll));
	EFI_PHYSICAL_ADDRESS pd = ((address >> 30) & (0x1ffll));
	EFI_PHYSICAL_ADDRESS pdp = ((address >> 39) & (0x1ffll));

	LOG_DBG("[MEM] Dirbase:  %p  VA %p  PO: %p  PTE: %p  PT: %p  PD: %p  PDP: %p\r\n", directoryBase, address, pageOffset, pte, pt, pd, pdp);

	// Read the PDPE from the directory base
	EFI_PHYSICAL_ADDRESS pdpe = 0;
	if (pMemCpy((EFI_PHYSICAL_ADDRESS)&pdpe, directoryBase + 8 * pdp, sizeof(EFI_PHYSICAL_ADDRESS)) == FALSE)
	{
		// Failed reading, abort
		return 0;
	}

	LOG_DBG("[MEM] Dump PDPE at %p results %p\r\n", directoryBase + 8 * pdp, 16, pdpe & PMASK);

	// Check if we got a valid entry
	if (~pdpe & 1)
		return 0;

	// Read the PDE entry from the directory base
	EFI_PHYSICAL_ADDRESS pde = 0;
	if (pMemCpy((EFI_PHYSICAL_ADDRESS)&pde, (pdpe & PMASK) + 8 * pd, sizeof(EFI_PHYSICAL_ADDRESS)) == FALSE)
	{
		// Failed reading, abort
		return 0;
	}

	LOG_DBG("[MEM] Dump PDE at %p results %p\r\n", (pdpe & PMASK) + 8 * pd, pde & PMASK);


	// Check if we got a valid entry
	if (~pde & 1)
		return 0;

	/* 1GB large page, use pde's 12-34 bits */
	if (pde & 0x80)
		return (pde & (~0ull << 42 >> 12)) + (address & ~(~0ull << 30));

	// Read the PTE address from the directory base
	EFI_PHYSICAL_ADDRESS pteAddr = 0;
	if (pMemCpy((EFI_PHYSICAL_ADDRESS)&pteAddr, (EFI_PHYSICAL_ADDRESS)(pde & PMASK) + 8 * pt, sizeof(EFI_PHYSICAL_ADDRESS)) == FALSE)
	{
		// Failed reading, abort
		return 0;
	}

	LOG_DBG("[MEM] Dump pteAddr at %p results %p\r\n", (pde & PMASK) + 8 * pt, pteAddr & PMASK);


	// Check if we got a valid entry
	if (~pteAddr & 1)
		return 0;

	/* 2MB large page */
	if (pteAddr & 0x80)
		return (pteAddr & PMASK) + (address & ~(~0ull << 21));

	// Read the address from the directory base
	if (pMemCpy((EFI_PHYSICAL_ADDRESS)&address, (pteAddr & PMASK) + 8 * pte, sizeof(EFI_VIRTUAL_ADDRESS)) == FALSE)
	{
		// Failed reading, abort
		return 0;
	}

	// 16.04 fix large address in corrupted page tables, 48bit physical address is enough
	address = address & PMASK;

	LOG_DBG("[MEM] Dump address at %p\r\n", (pteAddr & PMASK) + 8 * pte);

	// Check if we got a valid address
	if (!address)
		return 0;

	// Return the address and add the page relative offset again
	return address + pageOffset;
}

/*
* Function:  VTOPForce
* --------------------
* Translates the given virtual address using the given directory base to the associated physical memory address while NOT checking if the addresses are valid.
*
*  address:			EFI_VIRTUAL_ADDRESS (UINT64) virtual address to translate to physical
*  directoryBase:	EFI_PHYSICAL_ADDRESS (UINT64) address of the directory base to use for the translation
*
*  returns:	Returns 0 if translation failed, otherwise the physical address.
*
*/
EFI_PHYSICAL_ADDRESS VTOPForce(EFI_VIRTUAL_ADDRESS address, EFI_PHYSICAL_ADDRESS directoryBase)
{
	// Check if address was properly set
	if (address == 0)
	{
		LOG_VERB("[MEM] address is 0 \r\n");

		// No valid virtual address, abort
		return 0;
	}

	// check if directory base was properly set
	if (directoryBase == 0)
	{
		LOG_VERB("[MEM] directoryBase is 0 \r\n");

		// No valid directory base, abort
		return 0;
	}

	// Null the last bits
	directoryBase &= ~0xf;

	// Calculate the relevant offsets based on the address
	UINT64 pageOffset = address & ~(~0ul << PAGE_OFFSET_SIZE);
	EFI_PHYSICAL_ADDRESS pte = ((address >> 12) & (0x1ffll));
	EFI_PHYSICAL_ADDRESS pt = ((address >> 21) & (0x1ffll));
	EFI_PHYSICAL_ADDRESS pd = ((address >> 30) & (0x1ffll));
	EFI_PHYSICAL_ADDRESS pdp = ((address >> 39) & (0x1ffll));

	LOG_DBG("[MEM] Dirbase: %p  VA: %p  PO: %p  PTE: %p  PT: %p  PD: %p  PDP: %p\r\n", directoryBase, address, pageOffset, pte, pt, pd, pdp);


	// Read the PDPE from the directory base
	EFI_PHYSICAL_ADDRESS pdpe = 0;
	pMemCpyForce((EFI_PHYSICAL_ADDRESS)&pdpe, directoryBase + 8 * pdp, sizeof(EFI_PHYSICAL_ADDRESS));

	LOG_DBG("[MEM] PDPEA %p  PDPE: %p\r\n", directoryBase + 8 * pdp, pdpe);

	// Check if we got a valid entry
	if (~pdpe & 1)
		return 0;

	// Read the PDE from the directory base
	EFI_PHYSICAL_ADDRESS pde = 0;
	pMemCpyForce((EFI_PHYSICAL_ADDRESS)&pde, (pdpe & PMASK) + 8 * pd, sizeof(EFI_PHYSICAL_ADDRESS));

	LOG_DBG("[MEM] PDEA %p  PDE: %p\r\n", (pdpe & PMASK) + 8 * pd, pde);

	// Check if we got a valid entry
	if (~pde & 1)
		return 0;

	/* 1GB large page, use pde's 12-34 bits */
	if (pde & 0x80)
		return (pde & (~0ull << 42 >> 12)) + (address & ~(~0ull << 30));

	// Read the PTE address from the directory base
	EFI_PHYSICAL_ADDRESS pteAddr = 0;
	pMemCpyForce((EFI_PHYSICAL_ADDRESS)&pteAddr, (pde & PMASK) + 8 * pt, sizeof(EFI_PHYSICAL_ADDRESS));

	LOG_DBG("[MEM] PTEAA %p  PTEA: %p\r\n", (pde & PMASK) + 8 * pt, pteAddr);

	// Check if we got a valid entry
	if (~pteAddr & 1)
		return 0;

	/* 2MB large page */
	if (pteAddr & 0x80)
		return (pteAddr & PMASK) + (address & ~(~0ull << 21));

	// Read the physical address from the directory base
	pMemCpyForce((EFI_PHYSICAL_ADDRESS)&address, (pteAddr & PMASK) + 8 * pte, sizeof(EFI_PHYSICAL_ADDRESS));

	// Null the last bits as it should be a 48-bit pointer
	address = address & PMASK;

	LOG_DBG("[MEM] PTE %p  R: %p\r\n", (pteAddr & PMASK) + 8 * pte, address);

	// Return the address with the page relative offset
	return address + pageOffset;
}

/*
* Function:  caching
* --------------------
* This function introduces a wrapper around VTOP with the included functionality of caching by utilizing a preallocated page.
* Supports a maximum of 255 entries in the cache.
*
*  address:			EFI_VIRTUAL_ADDRESS (UINT64) virtual address to translate to physical
*  directoryBase:	EFI_PHYSICAL_ADDRESS (UINT64) address of the directory base to use for the translation
*
*  returns:	Returns 0 if translation failed, otherwise the physical address.
*
*/
EFI_PHYSICAL_ADDRESS caching(EFI_VIRTUAL_ADDRESS address, EFI_PHYSICAL_ADDRESS directoryBase)
{
	// Check if caching is already setup
	if (cachingPointer == 0)
	{
		// Set it up now
		EFI_PHYSICAL_ADDRESS physAddress;
		gSmst2->SmmAllocatePages(AllocateAnyPages, EfiRuntimeServicesData, 1, &physAddress);

		// Check if we got a valid address
		if (physAddress != 0)
		{
			// Assign it to the global pointer
			cachingPointer = physAddress;
			LOG_INFO("[MEM] Allocated caching memory at 0x%x \r\n", cachingPointer);
		}
		else
		{
			LOG_ERROR("[MEM] Failed allocating caching memory \r\n");
		}
	}

	// Handle everything with pagebases so theres only 1 translation per page
	EFI_VIRTUAL_ADDRESS virtualPageBase = address & 0xfffffffffffff000;
	EFI_VIRTUAL_ADDRESS virtualRelative = address & 0xfff;

	// Only support a maximum of 255 pages, check if we already reached the limit
	if (cachingSize >= 255)
	{
		// Reset the caching by setting the size to 0
		cachingSize = 0;
	}

	// Loop through the page to check if an entry exists
	for (UINT32 i = 0; i < cachingSize; i++)
	{
		// Get the current caching entry at this position
		Cache* tempCache = (Cache*)(cachingPointer + (sizeof(Cache) * i));

		// Check if the virtual page address matches the one requested and the entry has a valid physical address
		if (tempCache->vAddress == virtualPageBase && tempCache->pAddress != 0)
		{
			// Found the entry, return physical address
			return tempCache->pAddress + virtualRelative;
		}
	}

	// If we get here, it means the entry was not found in the caching, thus set it up as next entry 
	EFI_PHYSICAL_ADDRESS tempPhysical = VTOP(address, directoryBase);

	// Check if we successfully translated the virtual address
	if (tempPhysical != 0)
	{
		// Successfully translated, increase the caching size by 1
		cachingSize = cachingSize + 1;

		// Create a new caching structure at the last position
		Cache* tempCache = (Cache*)(cachingPointer + (cachingSize * sizeof(Cache)));

		// Set the physical and virtual address in the new entry
		tempCache->pAddress = tempPhysical & 0xfffffffffffff000;
		tempCache->vAddress = virtualPageBase & 0xfffffffffffff000;

		// Return this physical address
		return tempPhysical;
	}
	else
	{
		// Failed to get physical address, return 0 as this is most likely an invalid virtual address
		return 0;
	}
}

/*
* Function:  resetCaching
* --------------------
* Resets the VTOP caching system
*
*  returns:	Nothing.
*
*/
VOID resetCaching()
{
	if (cachingPointer != 0)
	{
		cachingSize = 0;
		nullBuffer((EFI_PHYSICAL_ADDRESS)cachingPointer, 0x1000);
	}
}

/*
* Function:  getPageTableEntry
* --------------------
* This function returns the pointer to the page entry in the directory base.
* It is used to perform modifications in the page tables.
*
*  address:			UINT64 virtual address to get the page table entry for
*  directoryBase:	UINT64 address of the directory base to use for the translation
*
*  returns:	Returns 0 if page table found, otherwise the physical address.
*
*/
EFI_PHYSICAL_ADDRESS getPageTableEntry(EFI_VIRTUAL_ADDRESS address, EFI_PHYSICAL_ADDRESS directoryBase)
{
	// Check if address was properly set
	if (address == 0)
	{
		LOG_VERB("[MEM] address is 0 \r\n");

		// No valid virtual address, abort
		return 0;
	}

	// check if directory base was properly set
	if (directoryBase == 0)
	{
		LOG_VERB("[MEM] directoryBase is 0 \r\n");

		// No valid directory base, abort
		return 0;
	}

	// Null the last bits
	directoryBase &= ~0xf;

	// Calculate the relevant offsets based on the address
	UINT64 pageOffset = address & ~(~0ul << PAGE_OFFSET_SIZE);
	EFI_PHYSICAL_ADDRESS pte = ((address >> 12) & (0x1ffll));                 // PTE index
	EFI_PHYSICAL_ADDRESS pt = ((address >> 21) & (0x1ffll));                  // PDE Index
	EFI_PHYSICAL_ADDRESS pd = ((address >> 30) & (0x1ffll));                  // PDPT index
	EFI_PHYSICAL_ADDRESS pdp = ((address >> 39) & (0x1ffll));                 // PML4 index

	LOG_DBG("[MEM] Dirbase: %p  VA: %p  PO: %p  PTE: %p  PT: %p  PD: %p  PDP: %p\r\n", directoryBase, address, pageOffset, pte, pt, pd, pdp);

	// Read the PDPE from the directory base
	EFI_PHYSICAL_ADDRESS pdpe = 0; // PML4_entry
	if (pMemCpy((EFI_PHYSICAL_ADDRESS)&pdpe, directoryBase + 8 * pdp, sizeof(EFI_PHYSICAL_ADDRESS)) == FALSE)
	{
		// Failed reading, abort
		return 0;
	}

	LOG_DBG("[MEM] Dump PDPE at %p results %p\r\n", directoryBase + 8 * pdp, 16, pdpe & PMASK);

	// Check if we got a valid entry
	if (~pdpe & 1)
		return 0;

	// Read the PDE from the directory base
	EFI_PHYSICAL_ADDRESS pde = 0;
	if (pMemCpy((EFI_PHYSICAL_ADDRESS)&pde, (pdpe & PMASK) + 8 * pd, sizeof(EFI_PHYSICAL_ADDRESS)) == FALSE)
	{
		// Failed reading, abort
		return 0;
	}

	LOG_DBG("[MEM] Dump PDE at %p results %p\r\n", (pdpe & PMASK) + 8 * pd, pde & PMASK);

	// Check if we got a valid entry
	if (~pde & 1)
		return 0;

	/* 1GB large page, use pde's 12-34 bits */
	if (pde & 0x80)
		return (pdpe & PMASK) + 8 * pd;

	// Read the PTE address from the directory base
	EFI_PHYSICAL_ADDRESS pteAddr = 0;
	if (pMemCpy((EFI_PHYSICAL_ADDRESS)&pteAddr, (pde & PMASK) + 8 * pt, sizeof(EFI_PHYSICAL_ADDRESS)) == FALSE)
	{
		// Failed reading, abort
		return 0;
	}

	LOG_DBG("[MEM] Dump pteAddr at %p results %p\r\n", (pde & PMASK) + 8 * pt, pteAddr & PMASK);

	// Check if we got a valid entry
	if (~pteAddr & 1)
		return 0;

	/* 2MB large page */
	if (pteAddr & 0x80)
		return (pde & PMASK) + 8 * pt;

	/* 4KB page */
	return (pteAddr & PMASK) + 8 * pte;
}

/* Functions related to virtual memory */

/*
* Function:  vMemCpy
* --------------------
* This function copies the memory with the given size from the virtual source to the virtual destination.
* This function uses verification and aborts if the address is not part of the memory map.
*
*  dest:			EFI_VIRTUAL_ADDRESS (UINT64) virtual address to copy the memory into
*  src:				EFI_VIRTUAL_ADDRESS (UINT64) virtual address to copy the memory from
*  size:			size_t size of the memory to copy into the destination
*  directoryBase:	EFI_PHYSICAL_ADDRESS (UINT64) address of the directory base to use for the translation
*
*  returns:	Returns false if translation or physical memory copy failed, otherwise true.
*
*/
BOOLEAN vMemCpy(EFI_VIRTUAL_ADDRESS dest, EFI_VIRTUAL_ADDRESS src, size_t size, EFI_PHYSICAL_ADDRESS directoryBaseDest, EFI_PHYSICAL_ADDRESS directoryBaseSrc)
{
	// Translate the virtual source address into a physical address
	UINT64 pSrc = VTOP(src, directoryBaseSrc);

	// Translate the virtual destination address into a physical address
	UINT64 pDest = VTOP(dest, directoryBaseDest);

	// Check if we got a valid address
	if (pSrc == 0 || pDest == 0 || size == 0)
	{
		// Abort execution at this point
		return FALSE;
	}

	// Read physical address into our buffer and return the result directly
	return pMemCpy(pDest, pSrc, size);
}

/*
* Function:  vMemRead
* --------------------
* This function copies the memory with the given size from the virtual source while translating it first into a physical address.
* This function uses verification and aborts if the address is not part of the memory map.
*
*  dest:			EFI_PHYSICAL_ADDRESS (UINT64) physical address to copy the memory into
*  src:				EFI_VIRTUAL_ADDRESS (UINT64) virtual address to copy the memory from
*  size:			size_t size of the memory to copy into the destination
*  directoryBase:	EFI_PHYSICAL_ADDRESS (UINT64) address of the directory base to use for the translation
*
*  returns:	Returns false if translation or physical memory copy failed, otherwise true.
*
*/
BOOLEAN vMemRead(EFI_PHYSICAL_ADDRESS dest, EFI_VIRTUAL_ADDRESS src, size_t size, EFI_PHYSICAL_ADDRESS directoryBase)
{
	// Translate the virtual source address into a physical address
	UINT64 pSrc = VTOP(src, directoryBase);

	// Check if we got a valid address
	if (pSrc == 0 || dest == 0 || size == 0)
	{
		// Abort execution at this point
		return FALSE;
	}

	// Read physical address into our buffer and return the result directly
	return pMemCpy(dest, pSrc, size);
}

/*
* Function:  vMemReadForce
* --------------------
* This function copies the memory with the given size from the virtual source while translating it first into a physical address.
* This function does NOT use verification and could halt the system if the physical address it accesses is invalid.
*
*  dest:			EFI_PHYSICAL_ADDRESS (UINT64) physical address to copy the memory into
*  src:				EFI_VIRTUAL_ADDRESS (UINT64) virtual address to copy the memory from
*  size:			size_t size of the memory to copy into the destination
*  directoryBase:	EFI_PHYSICAL_ADDRESS (UINT64) address of the directory base to use for the translation
*
*  returns:	Returns false if translation or physical memory copy failed, otherwise true.
*
*/
BOOLEAN vMemReadForce(EFI_PHYSICAL_ADDRESS dest, EFI_VIRTUAL_ADDRESS src, size_t size, EFI_PHYSICAL_ADDRESS directoryBase)
{
	// Translate the virtual source address into a physical address without doing any physical memory verification
	EFI_PHYSICAL_ADDRESS pSrc = VTOPForce(src, directoryBase);

	// Check if we got a valid address
	if (pSrc == 0 || dest == 0 || size == 0)
	{
		// Abort execution at this point
		return FALSE;
	}

	// Read physical address into our buffer and return the result directly without performing physical memory verification
	return pMemCpyForce(dest, pSrc, size);
}

/*
* Function:  vRead
* --------------------
* This function copies the memory with the given size from the virtual source while translating it first into a physical address.
* This function uses physical memory verification and utilizes caching for better performance.
*
*  dest:			EFI_PHYSICAL_ADDRESS (UINT64) physical address to copy the memory into
*  src:				EFI_VIRTUAL_ADDRESS (UINT64) virtual address to copy the memory from
*  size:			size_t size of the memory to copy into the destination
*  directoryBase:	EFI_PHYSICAL_ADDRESS (UINT64) address of the directory base to use for the translation
*
*  returns:	Returns false if translation or physical memory copy failed, otherwise true.
*
*/
BOOLEAN vRead(EFI_PHYSICAL_ADDRESS dest, EFI_VIRTUAL_ADDRESS src, size_t size, EFI_PHYSICAL_ADDRESS directoryBase)
{
	// Translate the virtual source address into a physical address while using the caching feature for better performance
	EFI_PHYSICAL_ADDRESS pSrc = caching(src, directoryBase);

	// Check if we got a valid address
	if (pSrc == 0 || dest == 0 || size == 0)
	{
		// Abort execution at this point
		return FALSE;
	}

	// Read physical address into our buffer and return the result directly
	return pMemCpy(dest, pSrc, size);
}

/*
* Function:  vMemWrite
* --------------------
* This function copies the memory with the given size from the physical source into the virtual destination that is translated into a physical address.
* This function uses verification and aborts if the address is not part of the memory map.
*
*  dest:			EFI_VIRTUAL_ADDRESS (UINT64) virtual address to copy the memory into
*  src:				EFI_PHYSICAL_ADDRESS (UINT64) physical address to copy the memory from
*  size:			size_t size of the memory to copy into the destination
*  directoryBase:	EFI_PHYSICAL_ADDRESS (UINT64) address of the directory base to use for the translation
*
*  returns:	Returns false if translation or physical memory copy failed, otherwise true.
*
*/
BOOLEAN vMemWrite(EFI_VIRTUAL_ADDRESS dest, EFI_PHYSICAL_ADDRESS src, size_t size, EFI_PHYSICAL_ADDRESS directoryBase)
{
	// Translate the virtual destination address into a physical address
	UINT64 pDest = VTOP(dest, directoryBase);

	// Check if we got a valid address
	if (pDest == 0 || src == 0 || size == 0)
	{
		// Abort execution at this point
		return FALSE;
	}

	// Read physical address into our buffer and return the result directly
	return pMemCpy(pDest, src, size);
}
