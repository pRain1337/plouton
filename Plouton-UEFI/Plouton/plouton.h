/*++
*
* Contains general definitions used in Plouton
*
--*/

#ifndef __plouton_header_h__
#define __plouton_header_h__

// our includes
#include "general/config.h"
#include "hardware/serial.h"
#include "hardware/xhci.h"
#include "hardware/timerPCH.h"
#include "hardware/timerRTC.h"
#include "os/windows/NTKernelTools.h"
#include "memory/memory.h"
#include "memory/memoryMapUEFI.h"

// edk2 includes 
#include <Library/TimerLib.h>
#include <Library/IoLib.h>
#include <Protocol/SmmBase2.h>
#include <Uefi/UefiBaseType.h>

// Structures

// Definitions

// General EFI SMM System table to access runtime functions by edk2 
EFI_SMM_SYSTEM_TABLE2* gSmst2;

// For performance optimization to not trigger all functions at every SMI
BOOLEAN startSeconds = FALSE;

// Handler for this SMM module
EFI_HANDLE hPloutonHandler;

// re-init game if it was closed
UINT64 currSMIamount;

// Optimization features
UINT8 lastMinute = 0;				// Minute of the last execution, used in the beginning to not do the full initialization at every SMI but only every minute
UINT16 counter = 0;					// Used to count the SMIs received, used so we dont trigger certain scans every SMI

// Functions

/*
* Function:  FindTarget
* --------------------
* Searches for the built-in targets (games) using the wrapper function and tries to initialize the required features.
*/
BOOLEAN FindTarget();

/*
* Function:  UefiMain
* --------------------
* Initializes the main functionalities of the SMM module.
*/
EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE* SystemTable);

#endif
