/*++

Copyright (c) pRain1337 & Jussihi  All rights reserved.

--*/

// Our includes
#include "plouton.h"
#include "general/config.h"

#if ENABLE_MEMORY_LOG
#include "logging/memory_log.h"

// From memory_log.c, we need access to the global buffer address
// We use an extern declaration to make it accessible here.
extern EFI_PHYSICAL_ADDRESS gMemoryLogBufferAddress;

// This is the unique GUID for our UEFI variable.
// {71245e36-47a7-4458-8588-74a4411b9332}
STATIC CONST EFI_GUID gPloutonLogAddressGuid =
  { 0x71245e36, 0x47a7, 0x4458, { 0x85, 0x88, 0x74, 0xa4, 0x41, 0x1b, 0x93, 0x32 } };
#endif

/*
 * Just a workaround for stupid MVSC pragmas
 * and other GCC-only warnings treated as errors.
 */
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-value"
#endif

 // UEFI Tables (will be gone after exiting DXE stage) 
extern EFI_SYSTEM_TABLE* gST;
extern EFI_BOOT_SERVICES* gBS;

// from NTKernelTools.c
extern WinCtx* winGlobal;
extern BOOLEAN setupWindows;

// from timerRTC.c - Time units used for tracking
extern UINT8 currentSecond;
extern UINT8 currentMinute;
extern UINT8 currentDay;
extern UINT8 currentMonth;

// From memory.c
extern EFI_PHYSICAL_ADDRESS cachingPointer;

// From memoryMapUEFI.c
extern EFI_MEMORY_DESCRIPTOR*    mUefiMemoryMap;
extern UINTN                     mUefiMemoryMapSize;
extern UINTN                     mUefiDescriptorSize;

// Protocols
#include <Protocol/SmmCpu.h>
#include <Protocol/SmmBase2.h>
#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/SerialIo.h>
#include <Library/PcdLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/PciIo.h>
#include <Library/PciLib.h>

#include <ProcessorBind.h>

#include "target/targets.h"


/*
* Function:  FindTarget
* --------------------
*  This function searches for the built-in targets (games) and tries to initialize the features used for them.
*
*  returns:	true if it found and initialized target, otherwise false
*
*/
BOOLEAN FindTarget()
{
	LOG_INFO("[PL] Looking for target\r\n");

	// First check if any target games can be found
	EFI_PHYSICAL_ADDRESS dirBase = 0;
	for(INT32 i = 0; i < sizeof(targets) / sizeof(TargetEntry); i++)
	{
		dirBase = findProcess(winGlobal, targets[i].name);

		// No target found, remove this from the list
		if(!dirBase)
		{
			targets[i].initialized = FALSE;
			targets[i].dirBase = 0;
			continue;
		}
		
		// Found target, setup ...
		LOG_INFO("[PL] Found target %s with dirbase 0x%x\r\n", targets[i].name, dirBase);

		// Initialize the XHCI manipulation functionality, if the target is a game
		if (targets[i].isGame == TRUE)
		{
			if (initXHCI() == FALSE)
			{
				// failed initializing, abort execution
				LOG_ERROR("[PL] Failed initializing XHCI \r\n");
				return FALSE;
			}

			// If the aimbot feature is enabled, check if we got the physical addresses for the mouse
			if (ENABLE_AIM == TRUE && mouseInitialized() == FALSE)
			{
				LOG_ERROR("[PL] Aim enabled but not found\r\n");

				// Feature is enabled and mouse not found, abort execution
				return FALSE;
			}

			// If the sound esp feature is enabled, check if we got the physical address for the headset
			if (ENABLE_SOUND == TRUE && audioInitialized() == FALSE)
			{
				LOG_ERROR("[PL] Sound enabled but not found\r\n");

				// Feature is enabled and headset not found, abort execution
				return FALSE;
			}
		}

		// now start the cheat by initializing it
		if (targets[i].cheatInitFun())
		{
			// If we get here we successfully initialized
			// Set the current dirbase, this is used to check if the target game was restarted
			targets[i].dirBase = dirBase;

			// Set target setup to true to skip functions
			targets[i].initialized = TRUE;

			// Return true to indicate success
			return TRUE;
		}

		// If init failed, restore state
		targets[i].initialized = FALSE;
		targets[i].dirBase = 0;
	}

	// If we get here something failed, return false
	return FALSE;
}

/*
* Function:  SmmCallHandle
* --------------------
*  This is the main SMI handler which contains the main logic for initialization of NT Kernel, Hyper-V and target acquisition.
*  The initialization runs only on every other minute or after a certain number of SMIs for better performance.
*  The function will call the main Cheat functions of the games.
*
*  DispatchHandle:		Root SMI handler is called by default with the handle of the registered handler. We dont use this in the function itself.
*
*  returns:	Returns a EFI_STATUS, will be EFI_SUCCESS if everything worked
*
*/
EFI_STATUS EFIAPI SmmCallHandle(EFI_HANDLE DispatchHandle, IN CONST VOID *Context OPTIONAL, IN OUT VOID *CommBuffer OPTIONAL, IN OUT UINTN *CommBufferSize OPTIONAL)
{
	// Clear the status bits in the PCH to prevent a system halt
	pchClearUSB();

	// Initialize USB Timer to get more SMIs
	if (pchInitUSB() == FALSE)
		LOG_ERROR("[PL] Failed initializing USB timer \r\n");

	currSMIamount++;

	// check if we already have found a target (game)
	for(INT32 i = 0; i < sizeof(targets) / sizeof(TargetEntry); i++)
	{
		if (targets[i].initialized == TRUE)
		{
			// Already initialized, perform this check every 10000 executions
			if (currSMIamount > 10000)
			{
				// check if the target (game) has been restarted during lifetime of this boot
				EFI_PHYSICAL_ADDRESS newDirBase = findProcess(winGlobal, targets[i].name);

				// Check if the directory base is different from the initial directory base
				if (newDirBase == 0 || newDirBase != targets[i].dirBase)
				{
					// Directory base changed! Reset target so we reinitialize
					LOG_INFO("[PL] target re-init! \r\n");

					// Reset all variables to force reinitialization
					startSeconds = FALSE;
					targets[i].initialized = FALSE;
					targets[i].dirBase = 0;
				}

				// Reset the SMI counter so we perform the same activity again in 10000 SMIs
				currSMIamount = 0;
			}
		}
	}

	// Variable used to track status
	BOOLEAN status = FALSE;

	// Boolean used to trigger execution only on certain occassions
	BOOLEAN execute = FALSE;

	// Increase the internal counters of SMIs
	counter += 1;

	// Execute the target search on every 500 SMIs
	if (counter >= 500)
	{
		// Set execute to true to start target search
		execute = TRUE;

		// Reset the counter
		counter = 0;
	}

	// Check if this is a new minute and we should not run all the time yet
	if (lastMinute != currentMinute && startSeconds == FALSE)
	{
		// Set the current minute as last minute, so the next execution is in the next minute
		lastMinute = currentMinute;

		// Set execute to true to force a target search
		execute = TRUE;
	}
	else if (startSeconds == TRUE) // Check if we should run all the time
	{
		// Yes, set execute to true all the time
		execute = TRUE;
	}
	else
	{
		// If nothing applies, get the current CMOS time fr the next minute time
		getCMOSTime();
	}

	// Check if we should execute the target search
	if (execute == TRUE)
	{
		// Check if we've already found a target and thus should run all the time
		if (startSeconds == TRUE)
		{
			// Check what game is currently initialized and run its main loop
			for(INT32 i = 0; i < sizeof(targets) / sizeof(TargetEntry); i++)
			{
				if (targets[i].initialized == TRUE)
				{
					targets[i].cheatLoopFun();
				}
			}

			// Go to end of the handler
			goto end;
		}
		else
		{
			/// No game yet initialized, check if windows is initialized
			if (setupWindows == FALSE)
			{
				// Windows not yet initialized, run the function
				status = InitGlobalWindowsContext();

				LOG_INFO("[PL] Initializing Windows Context %x\r\n", status);

				// Save the status (will be true if everything worked)
				setupWindows = status;
			}

			// This is a separate IF so we save one SMI if we successfully initialized windows
			if (setupWindows == TRUE)
			{
				// Try to find an initialized target and execute it and exit
				for(INT32 i = 0; i < sizeof(targets) / sizeof(TargetEntry); i++)
				{
					if (targets[i].initialized == TRUE)
					{
						targets[i].cheatLoopFun();
						goto end;
					}
				}

				// If not found yet, run the 'FindTarget' function to search for all configured targets
				status = FindTarget();

				// Save the status in the startSeconds variable
				startSeconds = status;
			}
		}
	}

end:

	// Return EFI_SUCCESS and finish SMI handler
	return EFI_SUCCESS;
}

/*
* Function:  UefiMain
* --------------------
*  This function initializes the basic functionalities for plouton.
*
*  ImageHandle:				Handle of the associated image
*  SystemTable:				Pointer to the EFI_SYSTEM_TABLE which contains the BOOT_SERVICES and RUNTIME_SERVICES functions
*
*  returns:	Returns an EFI_STATUS code, EFI_SUCCESS if everything worked correctly
*
*/
EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE* SystemTable)
{
	if (SERIAL_DEBUG_LEVEL != LOG_LEVEL_NONE)
	{
		// Initialize the serial port so we can write to it
		SerialPortInitialize(SERIAL_PORT_0, SERIAL_BAUDRATE);
	}

	SerialPrintf("--------------------------------------------------\r\n");
	SerialPrintf("|                                                |\r\n");
	SerialPrintf("|                pRain & jussihi                 |\r\n");
	SerialPrintf("|                                                |\r\n");
	SerialPrintf("|             Initializing Plouton...            |\r\n");
	SerialPrintf("|                                                |\r\n");
	SerialPrintf("|                 Shoutout to:                   |\r\n");
	SerialPrintf("|                                                |\r\n");
	SerialPrintf("|                  $ kidua $                     |\r\n");
	SerialPrintf("|         # zyklon  ekknod  tiltmasteR #         |\r\n");
	SerialPrintf("|       # akandesh  uffya  balrog  zepta #       |\r\n");
	SerialPrintf("|    # river  vasyan  niceone  serbo  dayik #    |\r\n");
	SerialPrintf("|                                                |\r\n");
	SerialPrintf("--------------------------------------------------\r\n");

	// Save the system tables etc. in global variable for further usage
	gST = SystemTable;
	gBS = SystemTable->BootServices;

	// Variables used in the initialization
	EFI_STATUS status = EFI_SUCCESS;
	EFI_SMM_BASE2_PROTOCOL* SmmBase2;

	// GUIDs of protocols in use
	EFI_GUID SmmBase2Guid = EFI_SMM_BASE2_PROTOCOL_GUID;

	// Locate the EFI_SMM_BASE2_PROTOCOL which is required for the SMM SYSTEM TABLE
	status = gBS->LocateProtocol(&SmmBase2Guid, NULL, (void**)&SmmBase2);

	// Verify if it worked
	if (status != EFI_SUCCESS)
	{
		// Failed locating protocol, abort initialization
		LOG_ERROR("[PL] Failed getting SMMbase: %p\r\n", status);

		return EFI_ERROR_MAJOR;
	}

	// Get EFI_SMM_SYSTEM_TABLE2 in global var
	status = SmmBase2->GetSmstLocation(SmmBase2, &gSmst2);

	if (status != EFI_SUCCESS)
	{
		// Failed locating EFI_SMM_SYSTEM_TABLE2, abort initialization
		LOG_ERROR("[PL] Failed getting SMST: %p\r\n", status);

		return EFI_ERROR_MAJOR;
	}

	LOG_INFO("[PL] Found gSMST2: %p\r\n", (EFI_PHYSICAL_ADDRESS)gSmst2);

	// Register an SMM Root Handler to get execution
	EFI_HANDLE hSmmHandler;
	status = gSmst2->SmiHandlerRegister(&SmmCallHandle, NULL, &hSmmHandler);

	if (status != EFI_SUCCESS)
	{
		// Failed registering SMM Root handler, abort initialization
		LOG_ERROR("[PL] Failed registering ROOT SMM handler: %p\r\n", status);

		return EFI_ERROR_MAJOR;
	}

	// Prepare memory to store the physical memory map
	EFI_PHYSICAL_ADDRESS memMapAddr;
	UINT8 memMapPages = 1;
	status = gSmst2->SmmAllocatePages(AllocateAnyPages, EfiRuntimeServicesData, memMapPages, &memMapAddr);

	// Check if we successfully allocated the page
	if (status != EFI_SUCCESS)
	{
		LOG_ERROR("[PL] Failed allocating Memory map pages \r\n");

		// Return error code
		return EFI_ERROR_MAJOR;
	}
	else
	{
		LOG_INFO("[PL] Allocated Memory map memory \r\n");

		// Null it once to be sure
		for (UINT32 i = 0; i < (UINT32)(0x1000 * memMapPages); i++)
		{
			// Overwrite every byte in the pages with 0
			UINT8 temp = 0;
			pMemCpyForce(memMapAddr + i, (EFI_PHYSICAL_ADDRESS)&temp, 1);
		}

		LOG_INFO("[PL] memMapAddr: %p\r\n", memMapAddr);
	}

	LOG_INFO("[PL] Initializing UEFI Memory Map \r\n");

	// Initialize the memory map at the allocated memory space
	if (InitUefiMemoryMap(memMapAddr) == FALSE)
	{
		LOG_ERROR("[PL] Failed dumping Memory Map \r\n");

		return EFI_ERROR_MAJOR;
	}

	// Check if supplied memory map is valid
	if (CheckUefiMemoryMap(mUefiMemoryMap, mUefiMemoryMapSize, mUefiDescriptorSize) == TRUE)
	{
		LOG_INFO("[PL] Valid Memory Map \r\n");

		// Valid Memory Map, set it globally
	}
	else
	{
		LOG_INFO("[PL] Invalid Map \r\n");

		// Return error code
		return EFI_ERROR_MAJOR;
	}

	LOG_INFO("[PL] Allocating pages \r\n");

	// Allocate multiple pages for the caching we use for Virtual to Physical translations
	EFI_PHYSICAL_ADDRESS cachingAddress;
	UINT8 cachingPages = 2;
	status = gSmst2->SmmAllocatePages(AllocateAnyPages, EfiRuntimeServicesData, cachingPages, &cachingAddress);

	LOG_INFO("[PL] cachingAddress: %p\r\n", cachingAddress);

	// Check if we successfully allocated memory for caching
	if (status != EFI_SUCCESS)
	{
		LOG_ERROR("[PL] Failed allocating caching \r\n");

		// Have to call it here because compiler will leave it out else
		EFI_STATUS res = SmmCallHandle((EFI_HANDLE)0, NULL, NULL, NULL);

		// Return execution here
		return res;
	}
	else
	{
		LOG_INFO("[PL] Allocated caching memory \r\n");

		// Save the address for the caching
		cachingPointer = cachingAddress;

		// Null it once to be sure
		for (UINT32 i = 0; i < (UINT32)(0x1000 * cachingPages); i++)
		{
			// Overwrite every byte in the pages with 0
			UINT8 temp = 0;
			pMemCpyForce(cachingPointer + i, (EFI_PHYSICAL_ADDRESS)&temp, 1);
		}
	}

	// Allocate a single page for global structures we use to keep track (windows)
	EFI_PHYSICAL_ADDRESS winGlobalAddress;
	UINT8 winGlobalPages = 1;
	status = gSmst2->SmmAllocatePages(AllocateAnyPages, EfiRuntimeServicesData, winGlobalPages, &winGlobalAddress);

	LOG_INFO("[PL] winGlobalAddress: %p\r\n", winGlobalAddress);

	// Check if we successfully allocated the page
	if (status != EFI_SUCCESS)
	{
		LOG_ERROR("[PL] Failed allocating winGlobal \r\n");

		// Return error code
		return EFI_ERROR_MAJOR;
	}
	else
	{
		LOG_INFO("[PL] Allocated winGlobal memory \r\n");

		// Null it once to be sure
		for (UINT32 i = 0; i < (UINT32)(0x1000 * winGlobalPages); i++)
		{
			// Overwrite every byte in the pages with 0
			UINT8 temp = 0;
			pMemCpyForce(winGlobalAddress + i, (EFI_PHYSICAL_ADDRESS)&temp, 1);
		}

		// Add the win global structure in the allocated memory
		winGlobal = (WinCtx*)winGlobalAddress;

		LOG_INFO("[PL] WinGlobal: %p\r\n", (EFI_PHYSICAL_ADDRESS)winGlobal);
	}

	// Initialize the variable to keep track of the SMIs
	currSMIamount = 0;

#if ENABLE_MEMORY_LOG
	// ***************************************************
	// * Initialize Memory Logging
	// ***************************************************
	if (EFI_ERROR(InitMemoryLog(gBS)))
	{
		// If memory logging fails, we can't continue as the user has no other way to debug.
		return EFI_DEVICE_ERROR;
	}

	// ***************************************************
	// * Store Log Buffer Address in a UEFI Variable
	// ***************************************************
	if (gMemoryLogBufferAddress != 0)
	{
		status = gST->RuntimeServices->SetVariable(
			L"PloutonLogAddress",
			&gPloutonLogAddressGuid,
			EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
			sizeof(EFI_PHYSICAL_ADDRESS),
			&gMemoryLogBufferAddress
		);

		if (EFI_ERROR(status))
		{
			LOG_ERROR("[PL] Failed to set UEFI variable for log address. Status: %r\n", status);
			// This is not a fatal error, the log still works, but it will be hard to find.
		}
		else
		{
			LOG_INFO("[PL] Log address 0x%llx stored in UEFI variable 'PloutonLogAddress'.\n", gMemoryLogBufferAddress);
		}
	}
#endif // ENABLE_MEMORY_LOG

	// Test logging
	LOG_INFO("Plouton DriverEntry finished \r\n");

	// Now lets get all the required tables and protocols

	return EFI_SUCCESS;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
