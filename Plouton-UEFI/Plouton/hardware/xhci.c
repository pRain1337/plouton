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
UINT8 DCABOffset = 0x30; // Intel XHCI Specs - Table 5-18: Host Controller Operational Registers

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

	// Create an array to contain a maximum of 16 XHCI controllers
	EFI_PHYSICAL_ADDRESS mbars[16];

	// Null the local array
	nullBuffer((EFI_PHYSICAL_ADDRESS)mbars, sizeof(EFI_PHYSICAL_ADDRESS) * 16);

	// Find the MBARs of all XHCI controllers
	if (getMemoryBaseAddresses(mbars))
	{
		LOG_INFO("[XHC] Found atleast one XHCI controller! \r\n");
	}
	else
	{
		// Did not find any XHCI controller...
		LOG_ERROR("[XHC] Failed finding any XHCI controller... \r\n");

		return FALSE;
	}

	// Go through all MBARs in the array
	for (int m = 0; m < 16; m++)
	{
		// Check if this entry is valid
		if (mbars[m] == 0)
			continue;

		// Go through all activated audio drivers
		if (mouseProfile.initialized == FALSE)
		{
			for (int i = 0; i < sizeof(InitAudioDriverFuns) / sizeof(InitAudioDriverFun); i++)
			{
				audioProfile = InitAudioDriverFuns[i](mbars[m]);
				if (audioProfile.initialized == TRUE)
				{
					LOG_INFO("[XHC] Found supported audio driver! \r\n");
					break;
				}
			}
		}

		// Go through all activated mouse drivers
		if (mouseProfile.initialized == FALSE)
		{
			for (int i = 0; i < sizeof(InitMouseDriversFuns) / sizeof(mouseInitFuns); i++)
			{
				mouseProfile = InitMouseDriversFuns[i].InitMouseDriverFun(InitMouseDriversFuns[i].MouseDriverTdCheckFun, mbars[m]);
				if (mouseProfile.initialized == TRUE)
				{
					LOG_INFO("[XHC] Found supported mouse driver! \r\n");
					break;
				}
			}
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
			//  - Slot Context Field			- offset 0x04, size 32-bit
			//		-	Root Hub Port Number	- Bits 23:16	(Intel eXtensible Host Controller Interface for USB, Table 6-5, Page 445, Root Hub Port Number)
			//				Port number is at offset 0x02 in the 32-bit value. We can directly access it by using offset 0x06
			UINT8 RootPortNumber = 0;
			status = pMemCpyForce((EFI_PHYSICAL_ADDRESS)&RootPortNumber, context + 0x06, sizeof(UINT8));

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
				//  - Slot Context Field		- offset 0x04, size 32-bit
				//		-	Context Entries		- Bits 31:27			(Intel eXtensible Host Controller Interface for USB, Table 6-4, Page 445, Context Entries)
				//				Port number is at offset 0x03 in the 32-bit value. We can directly access it using offset 0x03.
				//				As the value is only 5-bits, we will have to perform a right-shift 3 to get the right context entries amount.
				UINT8 endpointAmount = 0;
				UINT8 slotContextEntries = 0;
				status = pMemCpyForce((EFI_PHYSICAL_ADDRESS)&slotContextEntries, context + 0x03, sizeof(UINT8));

				// Perform a right-shift 3 to get the context entries	(Intel eXtensible Host Controller Interface for USB, Table 6-4, Page 445, Context Entries)
				slotContextEntries = slotContextEntries >> 3;

				// Check if the device has endpoints
				if (slotContextEntries == 0 || status == FALSE)
				{
					// Device with no endpoints cant be used as mouse/audio, check next device
					LOG_DBG("[XHC] No Context Entries \r\n");

					continue;
				}

				// An endpoint can have 2 slot context entries (in & out), thus we calculate the endpoint amount based on if its an odd number
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

					// Read the endpoint type
					//  - Endpoint Context Field	- offset 0x04, size 32-bit
					//		-	Endpoint Type		- Bits 5:3		(Intel eXtensible Host Controller Interface for USB, Table 6-9, Page 452, Endpoint Type (EP Type))
					//				As the type is only 3 bits, we will have to clear the topmost bit and perform a right shift by 3.
					UINT8 endpointType = 0;
					status = pMemCpyForce((EFI_PHYSICAL_ADDRESS)&endpointType, endpointBase + (j * 0x20) + 0x04, sizeof(UINT8));

					// Calculate the proper endpoint type by clearing topmost bit and performing a right shift by 3
					endpointType = (endpointType & 0x7F) >> 3;

					LOG_VERB("[XHC] Endpoint %d type: %p\r\n", j, endpointType);

					// Check if the endpoint type we read is the same as the passed one
					if (endpointType != contextType || status == FALSE)
					{
						// Not the same, check the next device
						LOG_DBG("[XHC] Endpoint %d Type missmatch: %p\r\n", i, endpointType);

						continue;
					}

					// Get endpoint max packetsize
					//  - Endpoint Context Field	- offset 0x04, size 32-bit
					//		-	Max Packet Size		- Bits 31:16		(Intel eXtensible Host Controller Interface for USB, Table 6-9, Page 452, Max Packet Size)
					//				As the max packet size is offset 0x02 in the 32-bit value, directly access it using offset 0x06
					UINT16 endpointMaxPacketsize = 0;
					status = pMemCpyForce((EFI_PHYSICAL_ADDRESS)&endpointMaxPacketsize, endpointBase + (j * 0x20) + 0x06, sizeof(UINT16));

					LOG_DBG("[XHC] Endpoint %d Maxpacketsize: %p\r\n", i, endpointMaxPacketsize);

					// check if the endpoint max packet size is the same as the passed one
					if (endpointMaxPacketsize != contextPacketsize || status == FALSE)
					{
						// Not the same, check next device
						LOG_DBG("[XHC] Endpoint %d Maxpacketsize missmatch: %p\r\n", i, endpointMaxPacketsize);
						continue;
					}

					// Get endpoint avg trb length
					//	- Endpoint Context Field	- offset 0x10, size 32-bit
					//		- Average TRB Length	- Bits 15:0		(Intel eXtensible Host Controller Interface for USB, Table 6-11, Page 453, Average TRB Length)
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

					// Endpoint has the same values as the passed one, now get the transfer ring at offset 0x08 Table 6-10: Offset 08h
					//	- Endpoint Context Field	- offset 0x08, size 64-bit
					//		- TR Dequeue Pointer	- Bits 63:4		(Intel eXtensible Host Controller Interface for USB, Table 6-10, Page 453, TR Dequeue Pointer)
					EFI_PHYSICAL_ADDRESS transfer_ring = 0;
					status = pMemCpyForce((EFI_PHYSICAL_ADDRESS)&transfer_ring, endpointBase + (j * 0x20) + 0x08, sizeof(EFI_PHYSICAL_ADDRESS));

					// Null the first 4 bits as they are states & reserved bits
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
						// We use 0x20 steps, as upon a Normal TRB (Intel eXtensible Host Controller Interface for USB, 4.11.2.1, Page 211, Normal TRB), we have always observed an Event Data TRB (Intel eXtensible Host Controller Interface for USB, 4.11.5.2, Page 230, Event Data TRB)
						// For more information how transfer rings work, check out the following:
						//		- 	Intel eXtensible Host Controller Interface for USB, 4.9, Page 166, TRB Ring
						//		-	Intel eXtensible Host Controller Interface for USB, 4.11, Page 208, TRBs
						//		-	Intel eXtensible Host Controller Interface for USB, 6.4, Page 465, Transfer Request Block (TRB)
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

	// Read length of the capability register
	UINT8 CAPLENGTH = 0;
	pMemCpyForce((EFI_PHYSICAL_ADDRESS)&CAPLENGTH, base, sizeof(UINT8));

	// Check if we got valid length
	if (CAPLENGTH == 0)
	{
		// Invalid base, return 0
		LOG_ERROR("[XHC] Invalid CAPLENGTH read \r\n");

		return 0;
	}

	// Return the DCAB from the dynamic offset
	EFI_PHYSICAL_ADDRESS DCAB = 0;
	pMemCpyForce((EFI_PHYSICAL_ADDRESS)&DCAB, base + CAPLENGTH + DCABOffset, sizeof(EFI_PHYSICAL_ADDRESS));

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
	// go through all PCI devices and check for class code 0xC0330 (XHCI controller)
	for (UINT8 bus = 0; bus < 10; bus++)
	{
		for (UINT8 device = 0; device < 32; device++)
		{
			for (UINT8 function = 0; function < 32; function++)
			{
				// Try to read the Register2
				UINT32 Register2 = 0;
				if (PciReadBuffer(PCI_LIB_ADDRESS(bus, device, function, 0x8), 4, &Register2) != 4)
				{
					// Failed reading 
					LOG_VERB("[XHC] Failed PciReadBuffer \r\n");

					continue;
				}

				// Extract the values
				// Bits 31-24 = Class code,				0x0C = Serial bus controllers
				UINT8 ClassCode = (Register2 & 0xFF000000) >> 24;

				// Bits 23-16 = Sub-class,				0x03 = USB
				UINT8 SubClass = (Register2 & 0x00FF0000) >> 16;

				// Bits 15-8 = Programming interface	0x30 = XHCI (USB 3.0)
				UINT8 ProgIf = (Register2 & 0x0000FF00) >> 8;

				// Successfully read it, compare it
				if (ClassCode == 0x0C && SubClass == 0x03 && ProgIf == 0x30)
				{
					// Found a XHCI device
					LOG_INFO("[XHC] Found XHCI device at bus 0x%x device 0x%x function 0x%x \r\n", bus, device, function);

					// Try to read the MBAR
					EFI_PHYSICAL_ADDRESS MBAR = 0;
					if (PciReadBuffer(PCI_LIB_ADDRESS(bus, device, function, MBAROffset), 8, &MBAR) != 8)
					{
						// Failed reading 
						LOG_ERROR("[XHC] Failed PciReadBuffer \r\n");

						return 0;
					}

					LOG_INFO("[XHC] Found MBAR 0x%x \r\n", MBAR);

					// Null last bits
					MBAR = MBAR & 0xFFFFFFFFFFFFFF00;

					// Return MBAR, we will check validity later
					return MBAR;
				}
			}
		}
	}

	return 0;
}

/*
* Function:  getMemoryBaseAddresses
* --------------------
* This function writes the MBAR addresses of the XHCI controllers in the passed array pointer.
*
*  returns:	0 (FALSE) if it fails, otherwise it 1 (TRUE)
*
*/
BOOLEAN getMemoryBaseAddresses(EFI_PHYSICAL_ADDRESS *mbars)
{
	UINT8 index = 0;

	// go through all PCI devices and check for class code 0xC0330 (XHCI controller)
	for (UINT8 bus = 0; bus < 10; bus++)
	{
		for (UINT8 device = 0; device < 32; device++)
		{
			for (UINT8 function = 0; function < 32; function++)
			{
				// Try to read the Register2
				UINT32 Register2 = 0;
				if (PciReadBuffer(PCI_LIB_ADDRESS(bus, device, function, 0x8), 4, &Register2) != 4)
				{
					// Failed reading 
					LOG_VERB("[XHC] Failed PciReadBuffer \r\n");

					continue;
				}

				// Extract the values
				// Bits 31-24 = Class code,				0x0C = Serial bus controllers
				UINT8 ClassCode = (Register2 & 0xFF000000) >> 24;

				// Bits 23-16 = Sub-class,				0x03 = USB
				UINT8 SubClass = (Register2 & 0x00FF0000) >> 16;

				// Bits 15-8 = Programming interface	0x30 = XHCI (USB 3.0)
				UINT8 ProgIf = (Register2 & 0x0000FF00) >> 8;

				// Successfully read it, compare it
				if (ClassCode == 0x0C && SubClass == 0x03 && ProgIf == 0x30)
				{
					// Found a XHCI device
					LOG_INFO("[XHC] Found XHCI device at bus 0x%x device 0x%x function 0x%x \r\n", bus, device, function);

					// Try to read the MBAR
					EFI_PHYSICAL_ADDRESS MBAR = 0;
					if (PciReadBuffer(PCI_LIB_ADDRESS(bus, device, function, MBAROffset), 8, &MBAR) != 8)
					{
						// Failed reading 
						LOG_ERROR("[XHC] Failed PciReadBuffer \r\n");

						continue;
					}

					LOG_INFO("[XHC] Found MBAR 0x%x \r\n", MBAR);

					// Null last bits
					MBAR = MBAR & 0xFFFFFFFFFFFFFF00;

					// Add the MBAR to the list
					if (index > 16)
					{
						// MBAR Array would be out of bounds 
						LOG_ERROR("[XHC] Mbars out of bounds \r\n");

						return FALSE;
					}

					mbars[index] = MBAR;
					index += 1;
				}
			}
		}
	}

	// Return true if we found atleast one MBAR
	if (index > 0)
		return TRUE;

	return FALSE;
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