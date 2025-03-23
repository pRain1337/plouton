/*++
*
* Source file for XHCI manipulation related functions
*
--*/

// our includes
#include "xhci.h"
#include "../logging/logging.h"
#include "audio/audioDriver.h"
#include "mouse/mouseDriver.h"


// GLOBALS

mouseProfile_t mouseProfile;
audioProfile_t audioProfile;

// Offsets used
UINT8 MBAROffset = 0x10;
UINT8 DCABOffset = 0xB0;


/*
* Function:  initXHCI
* --------------------
* This function initializes the XHCI manipulation functions by searching for a suitable device and endpoint and then saving the addresses for manipulation in global variables.
*
*  returns:	0 if it fails, otherwise it returns the address to the endpoint of the found device as EFI_PHYSICAL_ADDRESS
*
*/
BOOLEAN initXHCI()
{
	LOG_INFO("[XHC] Initializing XHCI... \r\n");

	// Go through all activated audio drivers
	for (int i = 0; i < sizeof(InitAudioDriverFuns) / sizeof(InitAudioDriverFun); i++)
	{
		audioProfile = InitAudioDriverFuns[i]();
		if (audioProfile.initialized == TRUE)
		{
			LOG_INFO("[XHC] Found supported audio driver! \r\n");
			break;
		}
	}

	// Go through all activated mouse drivers
	for (int i = 0; i < sizeof(InitMouseDriversFuns) / sizeof(mouseInitFuns); i++)
	{
		mouseProfile = InitMouseDriversFuns[i].InitMouseDriverFun(InitMouseDriversFuns[i].MouseDriverTdCheckFun);
		if (mouseProfile.initialized == TRUE)
		{
			LOG_INFO("[XHC] Found supported mouse driver! \r\n");
			break;
		}
	}

	// Atleast one of those should be activated for XHCI to be considered activated
	return audioProfile.initialized | mouseProfile.initialized;
}

/*
* Function:  audioInitialized
* --------------------
*  Returns whether the audio driver is successfully initialized & ready to use
*
*  returns:	TRUE if audio is initialized, FALSE otherwise
*
*/
BOOLEAN audioInitialized()
{
	return audioProfile.initialized;
}

/*
* Function:  mouseInitialized
* --------------------
*  Returns whether the mouse driver is successfully initialized & ready to use
*
*  returns:	TRUE if mouse is initialized, FALSE otherwise
*
*/
BOOLEAN mouseInitialized()
{
	return mouseProfile.initialized;
}

/*
* Function:  getEndpointRing
* --------------------
* This function searches for an endpoint that matches the passed specification and returns the pointer to it.
*
*  DCAB:						EFI_PHYSICAL_ADDRESS pointer to the device context array base where all endpoint pointers are stored
*  contextType:					UINT16 type of the endpoint we are looking for
*  contextPacketsize:			UINT16 packet size that the endpoint uses
*  contextAveragetrblength:		UINT16 average trb length of the endpoint
*  isAudio:						BOOLEAN which indicates if we should skip some mouse checks as its a sound device
*
*  returns:	0 if it fails, otherwise it returns the address of the endpoint ring
*
*/
EFI_PHYSICAL_ADDRESS getEndpointRing(EFI_PHYSICAL_ADDRESS DCAB, UINT16 contextType, UINT16 contextPacketsize, UINT16 contextAveragetrblength, BOOLEAN isAudio, MouseDriverTdCheck MouseDriverTdCheckFun)
{
	LOG_VERB("[XHC] Checking for Type 0x%x Packetsize 0x%x Average TRB Length 0x%x isAudio %d \r\n", contextType, contextPacketsize, contextAveragetrblength, isAudio);
	// Loop through all device contexts, theres no length identifier but max possible is 64 (Skip hubs, dont connect mouse over hub)
	for (UINT8 i = 0; i < 64; i++)
	{
		// Use status variable to indicate success
		BOOLEAN status = FALSE;

		// Read EFI_PHYSICAL_ADDRESS address of the device and check if its 0
		EFI_PHYSICAL_ADDRESS context = 0;
		status = pMemCpyForce((EFI_PHYSICAL_ADDRESS)&context, DCAB + (i * 0x8), sizeof(EFI_PHYSICAL_ADDRESS));

		// Check if we got a valid address
		if (context == 0 || status == FALSE)
		{
			// No valid address, we reached the end and can abort the function
			break;
		}
		else
		{
			// Got a valid address for a device

			// Round down context as they are stored incorrectly
			context = context & 0xFFFFFFFFFFFF00;

			// Read the root port number of this device
			UINT8 RootPortNumber = 0;
			status = pMemCpyForce((EFI_PHYSICAL_ADDRESS)&RootPortNumber, context + 0x6, sizeof(UINT8));

			// Check if the device has a root port number
			if (RootPortNumber == 0 || status == FALSE)
			{
				// Hub or whatever, skip and check next device
				LOG_DBG("[XHC] Skipped hub \r\n");

				continue;
			}
			else
			{
				// Legitimate device which has a port, verify the specification
				LOG_VERB("[XHC] Found valid device on port %d \r\n", RootPortNumber);

				// Read the amount of endpoints the device has
				UINT8 endpointAmount = 0;
				UINT8 slotContextEntries = 0;
				status = pMemCpyForce((EFI_PHYSICAL_ADDRESS)&slotContextEntries, context + 0x3, sizeof(UINT8));

				// XHCI stores them strange, convert it to a valid entry
				slotContextEntries = slotContextEntries >> 3;

				// Check if the device has endpoints
				if (slotContextEntries == 0 || status == FALSE)
				{
					// Device with no endpoints cant be used as mouse/audio, check next device
					LOG_DBG("[XHC] No Context Entries \r\n");

					continue;
				}

				// A endpoint can have 2 slot context entries (in & out), thus we calculate the endpoint amount based on if its an odd number
				if (slotContextEntries >= 2)
				{
					// Check if its dividable by 2
					if (slotContextEntries % 2)
					{
						// Remove one slot context entry
						slotContextEntries = slotContextEntries - 1;
					}

					// Now divide by two to get the endpoint amount
					slotContextEntries = slotContextEntries / 2;
				}

				// Define the endpoint amount
				endpointAmount = slotContextEntries;

				// the endpoint starts at an offset of 0x20
				EFI_PHYSICAL_ADDRESS endpointBase = context + 0x20;

				// Now loop through the endpoints of the device and check the type, max packetsize and average trb length
				// Multiply the endpoint amounts, as a endpoint can have two entries
				for (UINT8 j = 0; j <= (endpointAmount * 2); j++)
				{
					LOG_DBG("[XHC] Check: %p\r\n", endpointBase + (j * 0x20) + 0x4);

					// Get endpoint type
					UINT8 endpointType = 0;
					status = pMemCpyForce((EFI_PHYSICAL_ADDRESS)&endpointType, endpointBase + (j * 0x20) + 0x4, sizeof(UINT8));

					// Calculate the proper endpoint type
					endpointType = endpointType >> 3;

					LOG_VERB("[XHC] Endpoint %d type: %p\r\n", j, endpointType);

					// Check if the endpoint type we read is the same as the passed one
					if (endpointType != contextType || status == FALSE)
					{
						// Not the same, check the next device
						LOG_DBG("[XHC] Endpoint %d Type missmatch: %p\r\n", i, endpointType);

						continue;
					}

					// Get endpoint max packetsize
					UINT16 endpointMaxPacketsize = 0;
					status = pMemCpyForce((EFI_PHYSICAL_ADDRESS)&endpointMaxPacketsize, endpointBase + (j * 0x20) + 0x6, sizeof(UINT16));

					LOG_DBG("[XHC] Endpoint %d Maxpacketsize: %p\r\n", i, endpointMaxPacketsize);

					// check if the endpoint max packet size is the same as the passed one
					if (endpointMaxPacketsize != contextPacketsize || status == FALSE)
					{
						// Not the same, check next device
						LOG_DBG("[XHC] Endpoint %d Maxpacketsize missmatch: %p\r\n", i, endpointMaxPacketsize);
						continue;
					}

					// Get endpoint avg trb length
					UINT16 endpointAveragetrblength = 0;
					status = pMemCpyForce((EFI_PHYSICAL_ADDRESS)&endpointAveragetrblength, endpointBase + (j * 0x20) + 0x10, sizeof(UINT16));

					LOG_DBG("[XHC] Endpoint %d Average TRB Length: %p\r\n", i, endpointAveragetrblength);

					// check if the endpoint average trb size is the same as the passed one
					if (endpointAveragetrblength != contextAveragetrblength || status == FALSE)
					{
						// Not the same, check next device
						LOG_DBG("[XHC] Endpoint %d Average TRB Length missmatch: %p\r\n", i, endpointAveragetrblength);
						continue;
					}

					// Endpoint has the same values as the passed one, now get the transfer ring at offset 0x8
					EFI_PHYSICAL_ADDRESS transfer_ring = 0;
					status = pMemCpyForce((EFI_PHYSICAL_ADDRESS)&transfer_ring, endpointBase + (j * 0x20) + 0x8, sizeof(EFI_PHYSICAL_ADDRESS));

					// Null the first 4 bits
					transfer_ring = transfer_ring & 0xFFFFFFFFFFFFFF0;

					// Check if we got a valid transfer ring
					if (transfer_ring == 0 || status == FALSE)
					{
						// Somehow transfer_ring was 0, continue maybe we find another device
						LOG_ERROR("[XHC] Endpoint %d transfer ring empty\r\n", i);
						continue;
					}

					LOG_VERB("[XHC] Endpoint %d Found Transfer ring: %p\r\n", i, transfer_ring);

					// Sometimes theres another endpoint that matches the config, check if this TD has one in it that starts with 0x2, if not skip it
					for (UINT16 k = 0; k <= 14; k++)
					{
						// Now get the Physical URB Enqueue Ring from the transfer ring
						EFI_PHYSICAL_ADDRESS TDPointerPhys = 0;
						status = pMemCpyForce((EFI_PHYSICAL_ADDRESS)&TDPointerPhys, transfer_ring + (k * 0x20), sizeof(EFI_PHYSICAL_ADDRESS));

						// Check if we actually read something
						if (TDPointerPhys == 0 || status == FALSE)
						{
							// Failed, check the next ring entry
							continue;
						}

						LOG_VERB("[XHC] Endpoint %d found valid Transfer ring entry 0x%x at position %d \r\n", i, TDPointerPhys, k);

						// callback to mouse driver, check if this is the device we are looking for
						BOOLEAN tdCheck = TRUE;
						if (isAudio == FALSE)
						{
							tdCheck = MouseDriverTdCheckFun(TDPointerPhys);
						}

						if (tdCheck == TRUE)
						{
							return transfer_ring;
						}
					}
				}
			}
		}
	}

	// Return 0 if we get here as we did not find a valid transfer ring
	return 0;
}

/*
* Function:  getDeviceContextArrayBase
* --------------------
* This function returns the Device context array base (DCAB) from the MBAR address passed.
*
*  base:						EFI_PHYSICAL_ADDRESS to the memory based registers of the XHCI
*
*  returns:	0 if it fails, otherwise it returns the address of the DCAB as EFI_PHYSICAL_ADDRESS
*
*/
EFI_PHYSICAL_ADDRESS getDeviceContextArrayBase(EFI_PHYSICAL_ADDRESS base)
{
	// Check if we got a valid base
	if (base == 0)
	{
		// Invalid base, return 0
		LOG_ERROR("[XHC] Invalid MBAR supplied \r\n");

		return 0;
	}

	// Return the DCAB from the offset
	EFI_PHYSICAL_ADDRESS DCAB = 0;
	pMemCpyForce((EFI_PHYSICAL_ADDRESS)&DCAB, base + DCABOffset, sizeof(EFI_PHYSICAL_ADDRESS));

	// Return the DCAB, we check validity later
	return DCAB;
}

/*
* Function:  getMemoryBaseAddress
* --------------------
* This function returns the memory based address registers of the XHCI by reading it from the PCI configuration space.
*
*  returns:	0 if it fails, otherwise it returns the address of the DCAB as EFI_PHYSICAL_ADDRESS
*
*/
EFI_PHYSICAL_ADDRESS getMemoryBaseAddress()
{
	// Read the address from the PCI configuration space (Intel 18.1, page 817) of the XHCI (Device 0x14, Function 0x0)
	EFI_PHYSICAL_ADDRESS MBAR = 0;
	if (PciReadBuffer(PCI_LIB_ADDRESS(0, 0x14, 0x0, MBAROffset), 8, &MBAR) != 8)
	{
		// Failed reading 
		LOG_ERROR("[XHC] Failed PciReadBuffer \r\n");

		return 0;
	}

	// Null last bits
	MBAR = MBAR & 0xFFFFFFFFFFFFFF00;

	// Return MBAR, we will check validity later
	return MBAR;
}

/*
* Function:  BeepXHCI
* --------------------
* This function will modify the outgoung packets of the found USB audio device to produce a noise that can be used for Sound ESP.
*
*  packetAmount:				soundTarget struct which contains information about the detail (was supposed to create a surround experience, doesnt work)
*  multiplication:				UINT8 value which is used as a multiplication factor to increase the volume (doesnt work)
* 
*  returns:	Nothing
*
*/
VOID BeepXHCI(UINT32 leftPackets, UINT32 rightPackets, UINT8 multiplication)
{
	// check if we even have a valid device to use for sound esp
	if (audioProfile.audioRingLocation == 0)
	{
		LOG_VERB("[XHC] Audio ring location not set\r\n");

		return;
	}

	// Loop through all transfer descriptors and write the modified data in every TD
	for (UINT8 i = 0; i < audioProfile.audioChannels; i++)
	{
		// Initialize some variables for the overwriting
		UINT16 rightEar = 0;
		UINT16 leftEar = 0;

		EFI_PHYSICAL_ADDRESS TDLocation = 0;

		// Get the pointer to the actual packet data for this instance
		pMemCpy((EFI_PHYSICAL_ADDRESS)&TDLocation, audioProfile.audioRingLocation + (i * 0x20), sizeof(EFI_PHYSICAL_ADDRESS));

		// check if its a valid location
		if (TDLocation != 0 && IsAddressValid(TDLocation))
		{
			// Valid location, we can overwrite

			// Write as many Packets in the TD as wished
			if (leftPackets != 0)
			{
				// Loop over 48 times through the packet and overwrite the data
				for (UINT8 j = 0; j < 48; j++)
				{
					// Overwrite the data for left ear based on distance from the enemy
					if (leftEar >= 0xFF && leftEar <= 0xFF00)
					{
						leftEar = 0xFF01;
					}
					else
					{
						leftEar += leftPackets * multiplication;
					}

					// Overwrite the actual USB packet
					pMemCpy(TDLocation + (j * 0x4), (EFI_PHYSICAL_ADDRESS)&leftEar, sizeof(UINT16));
				}
			}

			// Write as many Packets in the TD as wished
			if (rightPackets != 0)
			{
				// Loop over 48 times through the packet and overwrite the data
				for (UINT8 j = 0; j < 48; j++)
				{
					// Overwrite the data for right ear based on distance from the enemy
					if (rightEar >= 0xFF && rightEar <= 0xFF00)
					{
						rightEar = 0xFF01;
					}
					else
					{
						rightEar += rightPackets * multiplication;
					}

					// Overwrite the actual USB packet
					pMemCpy(TDLocation + (j * 0x4) + 2, (EFI_PHYSICAL_ADDRESS)&rightEar, sizeof(UINT16));
				}
			}
		}
	}

	return;
}

/*
* Function:  GetButtonXHCI1
* --------------------
* This function will return the current button state in the first transfer descriptor found in a USB packet of a HID mouse.
*
*  returns:	Nothing
*
*/
UINT8 GetButtonXHCI1()
{
	// Check if this transfer descriptor is set
	if (mouseProfile.PhysicalTDLocationButton == 0)
	{
		LOG_VERB("[XHC] Mouse physical TD button location not set\r\n");

		return 0;
	}

	// Read the value from the precalculated location
	UINT8 tempButton = 0;
	pMemCpy((EFI_PHYSICAL_ADDRESS)&tempButton, mouseProfile.PhysicalTDLocationButton, sizeof(UINT8));

	// Return the value we read
	return tempButton;
}

/*
* Function:  GetButtonXHCI2
* --------------------
* This function will return the current button state in the second transfer descriptor found in a USB packet of a HID mouse.
*
*  returns:	Nothing
*
*/
UINT8 GetButtonXHCI2()
{
	// Check if this transfer descriptor is set
	if (mouseProfile.PhysicalTDLocation2Button == 0)
	{
		LOG_VERB("[XHC] Mouse physical TD 2 button location not set\r\n");

		return 0;
	}

	// Read the value from the precalculated location
	UINT8 tempButton = 0;
	pMemCpy((EFI_PHYSICAL_ADDRESS)&tempButton, mouseProfile.PhysicalTDLocation2Button, sizeof(UINT8));

	// Return the value we read
	return tempButton;
}

/*
* Function:  WriteButtonXHCI1
* --------------------
* This function will write the button state in the first transfer descriptor in a USB packet of a HID mouse.
*
*  button:						UINT8 value which indicates the button state
*
*  returns:	Nothing
*
*/
VOID WriteButtonXHCI1(UINT8 button)
{
	// Check if this transfer descriptor is set
	if (mouseProfile.PhysicalTDLocationButton == 0)
	{
		LOG_VERB("[XHC] Mouse physical TD button location not set\r\n");

		return;
	}

	// Write the passed value to the precalculated location
	UINT8 tempButton = button;
	pMemCpy(mouseProfile.PhysicalTDLocationButton, (EFI_PHYSICAL_ADDRESS)&tempButton, sizeof(UINT8));

	// Return nothing
	return;
}

/*
* Function:  WriteButtonXHCI2
* --------------------
* This function will write the button state in the second transfer descriptor in a USB packet of a HID mouse.
*
*  button:						UINT8 value which indicates the button state
*
*  returns:	Nothing
*
*/
VOID WriteButtonXHCI2(UINT8 button)
{
	// Check if this transfer descriptor is set
	if (mouseProfile.PhysicalTDLocation2Button == 0)
	{
		LOG_VERB("[XHC] Mouse physical TD 2 button location not set\r\n");

		return;
	}

	// Write the passed value to the precalculated location
	UINT8 tempButton = button;
	pMemCpy(mouseProfile.PhysicalTDLocation2Button, (EFI_PHYSICAL_ADDRESS)&tempButton, sizeof(UINT8));

	// Return nothing
	return;
}

/*
* Function:  WriteButtonXHCI2
* --------------------
* This function will write the button state in the second transfer descriptor in a USB packet of a HID mouse.
*
*  x:							INT16 value which indicates the button state
*  y:							INT16 value which indicates the button state
*  ignoreMax:					BOOLEAN which indicates if the max amount of pixels can be ignored
*
*  returns:	Nothing
*
*/
VOID MoveMouseXHCI(INT16 x, INT16 y, BOOLEAN ignoreMax)
{
	// Check if the transfer descriptor are set
	if (mouseProfile.PhysicalTDLocationMove == 0 || mouseProfile.PhysicalTDLocation2Move == 0)
	{
		LOG_VERB("[XHC] Mouse physical TD move location not set\r\n");

		return;
	}

	// Check if we should ignore the maximum or not
	if (ignoreMax == TRUE)
	{
		// Still Limit the mouse movement to max movement (-15 to 15)
		INT16 maxX = 15;
		INT16 maxY = 15;

		if (x > maxX)
			x = maxX;

		if (x < (maxX * -1))
			x = (maxX * -1);

		if (y > maxY)
			y = maxY;

		if (y < (maxY * -1))
			y = (maxY * -1);
	}
	else
	{
		// Limit the mouse movement to max movement (-5 to 5)
		INT16 maxX = 5;
		INT16 maxY = 5;

		if (x > maxX)
			x = maxX;

		if (x < (maxX * -1))
			x = (maxX * -1);

		if (y > maxY)
			y = maxY;

		if (y < (maxY * -1))
			y = (maxY * -1);
	}

	// Write the limited X value to both precalculated locations
	if (x != 1 && x != -1)
	{
		// Write the X Movement at byte 3
		pMemCpyForce(mouseProfile.PhysicalTDLocationMove, (EFI_PHYSICAL_ADDRESS)&x, sizeof(UINT16));
		pMemCpyForce(mouseProfile.PhysicalTDLocation2Move, (EFI_PHYSICAL_ADDRESS)&x, sizeof(UINT16));
	}

	// Write the limited y value to both precalculated locations
	if (y != 1 && y != -1)
	{
		// Write the Y Movement at byte 5
		pMemCpyForce(mouseProfile.PhysicalTDLocationMove + 0x2, (EFI_PHYSICAL_ADDRESS)&y, sizeof(UINT16));
		pMemCpyForce(mouseProfile.PhysicalTDLocation2Move + 0x2, (EFI_PHYSICAL_ADDRESS)&y, sizeof(UINT16));
	}

	// Return nothing
	return;
}