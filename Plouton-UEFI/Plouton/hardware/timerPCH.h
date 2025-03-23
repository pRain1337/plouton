/*++
*
* Header file for functions related to SMI timer using PCH functionalities
*
--*/

#ifndef __plouton_timer_pch_h__
#define __plouton_timer_pch_h__

// our includes
#include "../general/config.h"
#include "serial.h"
#include "../memory/memory.h"

// edk2 includes
#include <Library/PciLib.h>
#include <Library/IoLib.h>

// Structures

// Definitions

// Definitions used in the function which are offset for various XHCI / PCH functions
#define PCH_SMI_EN							0x30	// Enables to toggle SMI generation for various sources
#define PCH_SMI_STS							0x34	// Indicates the reason the PCH triggered a SMI

#define XHCI_USBLegacySupportControlStatus	0x8470	// Not documented on newer specifications, check out series 200/300 or manually reverse it
#define XHCI_USBStatus						0x84	// Indicates if the SMI was triggered by the XHCI and should be cleared

// Functions

/*
* Function:  pchInitUSB
* --------------------
* Initializes the PCH timer using the XHCI SMI generation feature.
*/
BOOLEAN pchInitUSB();

/*
* Function:  pchClearUSB
* --------------------
* Clears the various status bits that are toggled when a SMI is created from the XHCI PCH timer.
*/
VOID pchClearUSB();

#endif