/*++
*
* Header file for XHCI manipulation related functions
*
--*/

#ifndef __plouton_xhci_h__
#define __plouton_xhci_h__

// our includes
#include "../target/cs2/structuresCS2.h"
#include "../memory/memory.h"
#include "../memory/string.h"
#include "../hardware/serial.h"
#include "../general/config.h"
#include "mouse/mouseDriver.h"

// edk2 includes
#include <Library/PciLib.h>

// Structures

// Definitions

// Functions

// - XHCI Initialization related functions

/*
* Function:  initXHCI
* --------------------
* This initializes the XHCI functions by parsing the necessary structures and then finding a suitable endpoint device we want to manipulate.
*/
BOOLEAN initXHCI();

/*
* Function:  audioInitialized
* --------------------
* Tells if the audio driver is initialized.
*/
BOOLEAN audioInitialized();

/*
* Function:  mouseInitialized
* --------------------
* Tells if the mouse driver is initialized.
*/
BOOLEAN mouseInitialized();

/*
* Function:  getMemoryBaseAddresses
* --------------------
* This function finds all XHCI controllers in the system and writes it into the passed array
*/
BOOLEAN getMemoryBaseAddresses(EFI_PHYSICAL_ADDRESS* mbars);

/*
* Function:  getDeviceContextArrayBase
* --------------------
* This function returns the Device Context Array Base (DCAB) which contains all USB devices in the system.
*/
EFI_PHYSICAL_ADDRESS getDeviceContextArrayBase(EFI_PHYSICAL_ADDRESS base);

/*
* Function:  getEndpointRing
* --------------------
* This function finds a specific endpoint of a device by searching for specific specifications.
*/
EFI_PHYSICAL_ADDRESS getEndpointRing(EFI_PHYSICAL_ADDRESS DCAB, UINT16 contextType, UINT16 contextPacketsize, UINT16 contextAveragetrblength, BOOLEAN isAudio, MouseDriverTdCheck MouseDriverTdCheckFun);

// - Manipulation related functions

/*
* Function:  MoveMouseXHCI
* --------------------
* This function modifies the x and y coordinates of a USB mouse HID packet.
*/
VOID MoveMouseXHCI(INT16 x, INT16 y, BOOLEAN ignoreMax);

/*
* Function:  WriteButtonXHCI2
* --------------------
* This function changes the button state in the second descriptor in a USB mouse HID packet.
*/
VOID WriteButtonXHCI2(UINT8 button);

/*
* Function:  WriteButtonXHCI1
* --------------------
* This function changes the button state in the first descriptor in a USB mouse HID packet.
*/
VOID WriteButtonXHCI1(UINT8 button);

/*
* Function:  GetButtonXHCI2
* --------------------
* This function gets the current button state from the second descriptor from a USB mouse HID packet.
*/
UINT8 GetButtonXHCI2();

/*
* Function:  GetButtonXHCI1
* --------------------
* This function gets the current button state from the first descriptor from a USB mouse HID packet.
*/
UINT8 GetButtonXHCI1();

/*
* Function:  BeepXHCI
* --------------------
* This function modifies the data in a USB audio device to produce a noticeable difference.
*/
VOID BeepXHCI(UINT32 leftPackets, UINT32 rightPackets, UINT8 multiplication);

#endif