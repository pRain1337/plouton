/*++
*
* Header file for memory related functions
*
--*/

#ifndef __plouton_memory_h__
#define __plouton_memory_h__

// our includes
#include "../general/config.h"
#include "memoryMapUEFI.h"
#include "../hardware/serial.h"
#include "../os/windows/windows.h"

// edk2 includes
#include <Protocol/SmmBase2.h>
#include <Uefi/UefiBaseType.h>


// Structures

typedef struct _Cache
{
	UINT64 vAddress;
	UINT64 pAddress;
} Cache, *PCache;

typedef struct _MapDirEntry
{
	UINT8 length;
	UINT64 pointer;
} MapDirEntry, *PMapDirEntry;

#ifdef __GNUC__
typedef UINT32 size_t;
#endif

// Definitions

// from plouton.c
extern EFI_SMM_SYSTEM_TABLE2* gSmst2;

// - Size definitions
#define PAGE_SIZE 0x1000

// - Pagetable operations
#define PT_PRESENT 0x01
#define PT_RW 0x02

// Functions

// - Physical memory

/*
* Function:  pMemCpy
* --------------------
* This function copies data with a given size from the source to a destination physical address and verifies the addresses for validity using the DXE memory map.
*/
BOOLEAN pMemCpy(EFI_PHYSICAL_ADDRESS dest, EFI_PHYSICAL_ADDRESS src, size_t n);

/*
* Function:  pMemCpyForce
* --------------------
* This function copies data with a given size from the source to a destination physical address without any verification.
*/
BOOLEAN pMemCpyForce(EFI_PHYSICAL_ADDRESS dest, EFI_PHYSICAL_ADDRESS src, size_t n);

/*
* Function:  IsAddressValid
* --------------------
* This function verifies if the given physical address exists in the currently used memory map.
*/
BOOLEAN IsAddressValid(EFI_PHYSICAL_ADDRESS address);

// - Virtual to physical translation

/*
* Function:  VTOP
* --------------------
* This function translates the given virtual address using the directory base to the physical memory address.
*/
EFI_PHYSICAL_ADDRESS VTOP(EFI_VIRTUAL_ADDRESS address, EFI_PHYSICAL_ADDRESS directoryBase);

/*
* Function:  VTOPForce
* --------------------
* This function translates the given virtual address using the directory base to the physical memory address without any verification.
*/
EFI_PHYSICAL_ADDRESS VTOPForce(EFI_VIRTUAL_ADDRESS address, EFI_PHYSICAL_ADDRESS directoryBase);

/*
* Function:  caching
* --------------------
* This function is a wrapper around VTOP which introduces a simple caching library.
*/
EFI_PHYSICAL_ADDRESS caching(EFI_VIRTUAL_ADDRESS address, EFI_PHYSICAL_ADDRESS directoryBase);

/*
* Function:  resetCaching
* --------------------
* Resets the VTOP caching system.
*/
VOID resetCaching();

/*
* Function:  getPageTableEntry
* --------------------
* This function returns the given page table entry for a virtual address in the directory base.
*/
EFI_PHYSICAL_ADDRESS getPageTableEntry(EFI_VIRTUAL_ADDRESS address, EFI_PHYSICAL_ADDRESS directoryBase);

// - Virtual memory

/*
* Function:  vMemCpy
* --------------------
* This function reads the memory at the given virtual address and copies it to another virtual address.
*/
BOOLEAN vMemCpy(EFI_VIRTUAL_ADDRESS dest, EFI_VIRTUAL_ADDRESS src, size_t size, EFI_PHYSICAL_ADDRESS directoryBaseDest, EFI_PHYSICAL_ADDRESS directoryBaseSrc);

/*
* Function:  vMemRead
* --------------------
* This function reads the memory at the given virtual address while performing physical memory verification.
*/
BOOLEAN vMemRead(EFI_PHYSICAL_ADDRESS dest, EFI_VIRTUAL_ADDRESS src, size_t n, EFI_PHYSICAL_ADDRESS directoryBase);

/*
* Function:  vMemReadForce
* --------------------
* This function reads the memory at the given virtual address without performing physical memory verification.
*/
BOOLEAN vMemReadForce(EFI_PHYSICAL_ADDRESS dest, EFI_VIRTUAL_ADDRESS src, size_t n, EFI_PHYSICAL_ADDRESS directoryBase);

/*
* Function:  vRead
* --------------------
* This function reads the memory at the given virtual address while performing physical memory verification and using caching in the translation process.
*/
BOOLEAN vRead(EFI_PHYSICAL_ADDRESS dest, EFI_VIRTUAL_ADDRESS src, size_t n, EFI_PHYSICAL_ADDRESS directoryBase);

/*
* Function:  vMemWrite
* --------------------
* This function writes the memory to the given virtual address while performing physical memory verification.
*/
BOOLEAN vMemWrite(EFI_VIRTUAL_ADDRESS dest, EFI_PHYSICAL_ADDRESS src, size_t size, EFI_PHYSICAL_ADDRESS directoryBase);

#endif
