/*++
*
* Header file for memory map related functions
*
--*/

#ifndef __plouton_mm_uefi_h__
#define __plouton_mm_uefi_h__

// our includes
#include "../general/config.h"
#include "memory.h"

// edk2 includes
#include <Uefi/UefiSpec.h>

// Global variables we want to share
extern EFI_MEMORY_DESCRIPTOR*    mUefiMemoryMap;
extern UINTN                     mUefiMemoryMapSize;
extern UINTN                     mUefiDescriptorSize;

// Structures

// Definitions

// from plouton.c
extern EFI_BOOT_SERVICES* gBS;



// calculate the next memory map descriptor by adding the size of a descriptor to the current entry
#define NEXT_MEMORY_DESCRIPTOR(MemoryDescriptor, Size) \
  ((EFI_MEMORY_DESCRIPTOR *)((UINT8 *)(MemoryDescriptor) + (Size)))

// calculate the previous memory map descriptor by subtracting the size of a descriptor to the current entry
#define PREVIOUS_MEMORY_DESCRIPTOR(MemoryDescriptor, Size) \
  ((EFI_MEMORY_DESCRIPTOR *)((UINT8 *)(MemoryDescriptor) - (Size)))

// Functions

/*
 * Function:  CheckUefiMemoryMap
 * --------------------
 * Checks if the passed memory map is valid
 */
BOOLEAN CheckUefiMemoryMap(EFI_MEMORY_DESCRIPTOR* MemoryMap, UINT64 MemoryMapSize, UINT64 MemoryMapDescriptorSize);

/*
 * Function:  SortMemoryMap
 * --------------------
 * Sorts the passed memory map
 */
VOID SortMemoryMap(IN OUT EFI_MEMORY_DESCRIPTOR* MemoryMap, IN UINTN MemoryMapSize, IN UINTN DescriptorSize);

/*
 * Function:  MergeMemoryMapForNotPresentEntry
 * --------------------
 * Merges entries in the memory map together
 */
VOID MergeMemoryMapForNotPresentEntry(IN OUT EFI_MEMORY_DESCRIPTOR* MemoryMap, IN OUT UINTN* MemoryMapSize, IN UINTN DescriptorSize);

/*
 * Function:  InitUefiMemoryMap
 * --------------------
 * Initialize a copy of the memory map in the passed pointer
 */
BOOLEAN InitUefiMemoryMap(UINT64 NewMemoryMap);

#endif