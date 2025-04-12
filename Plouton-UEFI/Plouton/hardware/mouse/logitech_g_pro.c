#include "mouseDriver.h"

#ifdef MOUSE_DRIVER_LOGITECH_G_PRO

// our includes
#include "../xhci.h"
#include "../../logging/logging.h"

/*
* Function:  initLogitechGProMouseXHCI
* --------------------
* Tries to initialize the mouse driver for the Logitech G Pro mouse.
*
*  MouseDriverTdCheck:			Pointer to the "MouseDriverTdCheck" function that is used to verify the device packets
*  MBAR:						EFI_PHYSICAL_ADDRESS pointer to the Memory base address of the XHCI controller
*
*  returns:	The mouseProfile struct, initialized set As TRUE if it was found
*
*/
mouseProfile_t EFIAPI initLogitechGProMouseXHCI(MouseDriverTdCheck MouseDriverTdCheckFun, EFI_PHYSICAL_ADDRESS MBAR)
{
	// This implementation is based on the following documents:
	// - Intel 600 series datasheet see: https://www.intel.com/content/www/us/en/content-details/742460/intel-600-series-chipset-family-for-iot-edge-platform-controller-hub-pch-datasheet-volume-2-of-2.html
	// - The SMM Rootkit Revisited: Fun with USB https://papers.put.as/papers/firmware/2014/schiffman2014.pdf
	// - eXtensible Host Controller Interface for Universal Serial Bus (xHCI) https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/extensible-host-controler-interface-usb-xhci.pdf

	mouseProfile_t ret = { 0, 0, 0, 0, 0, 0 };

	// (Intel 600 series datasheet, 18.1.11, Page 822) First we get the MBAR of the XHCI device
	// Check if we got a valid MBAR
	if (MBAR == 0)
	{
		LOG_ERROR("[XHC] Failed MBAR\r\n");

		// Return 0 to indicate failure
		return ret;
	}

	LOG_INFO("[XHC] Found MBAR %p\r\n", MBAR);

	// (Intel eXtensible Host Controller Interface for USB, 5.4, Page 391) Now that we have the MBAR we can access the Memory Mapped registers, use it to get the Device Context Array Base Pointer which contains pointers to the devices.
	EFI_PHYSICAL_ADDRESS DCAB = getDeviceContextArrayBase(MBAR);

	// Check if we got a valid DCAB
	if (DCAB == 0)
	{
		LOG_ERROR("[XHC] Failed DCAB\r\n");

		// Return 0 to indicate failure
		return ret;
	}

	LOG_INFO("[XHC] Found DCAB %p\r\n", DCAB);

	// Now as we have the DCAB, we scan all of the devices for the following specifications:
	//  - Endpoint Context Field	- offset 0x04, size 32-bit
	//		-	Endpoint Type		- Bits 5:3		(Intel eXtensible Host Controller Interface for USB, Table 6-9, Page 452, Endpoint Type (EP Type))
	//		-	Max Packet Size		- Bits 31:16	(Intel eXtensible Host Controller Interface for USB, Table 6-9, Page 452, Max Packet Size)
	// 
	//	- Endpoint Context Field	- offset 0x10, size 32-bit
	//		- Average TRB Length	- Bits 15:0		(Intel eXtensible Host Controller Interface for USB, Table 6-11, Page 453, Average TRB Length)

	//Check for  Logitech G Pro Mouse (Normal)
	EFI_PHYSICAL_ADDRESS endpointRing = getEndpointRing(DCAB, 7, 0x10, 0x8, FALSE, MouseDriverTdCheckFun); // Hardcoded for now

	// check if we found the device
	if (endpointRing == 0)
	{
		LOG_ERROR("[XHC] Failed finding endpoint ring \r\n");
		return ret;
	}

	LOG_VERB("[XHC] Found endpoint Ring %p\r\n", endpointRing);

	// Find transferBuffer
	// loop is not needed, the ring is filled from top to bottom.
	// --> if there is no data at the top, there will not be data in the bottom either.
	EFI_PHYSICAL_ADDRESS TDPointerPhys = 0;
	EFI_PHYSICAL_ADDRESS TDPointerPhys2 = 0;

	// Copy the first and the second transfer buffer pointers (they are positioned with a distance of 0x20 between)
	// We use 0x20 steps, as upon a Normal TRB (Intel eXtensible Host Controller Interface for USB, 4.11.2.1, Page 211, Normal TRB), we have always observed an Event Data TRB (Intel eXtensible Host Controller Interface for USB, 4.11.5.2, Page 230, Event Data TRB)
	pMemCpyForce((EFI_PHYSICAL_ADDRESS)&TDPointerPhys, endpointRing, sizeof(EFI_PHYSICAL_ADDRESS));
	pMemCpyForce((EFI_PHYSICAL_ADDRESS)&TDPointerPhys2, endpointRing + 0x20, sizeof(EFI_PHYSICAL_ADDRESS));

	// Check if either of them is 0
	if (TDPointerPhys == 0 || TDPointerPhys2 == 0)
	{
		LOG_ERROR("[XHC] Received invalid transfer buffers for mouse \r\n");

		// Return 0 as the transfer buffers were invalid
		return ret;
	}

	// We now got the addresses but want to verify if those are really from a Logitech mouse

	// Copy the first byte of each descriptor
	UINT8 tempIdentifier = 0;
	UINT8 tempIdentifier2 = 0;
	pMemCpyForce((EFI_PHYSICAL_ADDRESS)&tempIdentifier, TDPointerPhys, sizeof(UINT8));
	pMemCpyForce((EFI_PHYSICAL_ADDRESS)&tempIdentifier2, TDPointerPhys2, sizeof(UINT8));

	LOG_VERB("[XHC] Temp identifier 0x%x Temp Identifier 2 0x%x \r\n", tempIdentifier, tempIdentifier2);

	// Check if they start with 0x2 or 0x3 (depends on firmware)
	if (((tempIdentifier == 0x2 || tempIdentifier == 0x3) && (tempIdentifier2 == 0x2 || tempIdentifier2 == 0x3)))
	{
		// Now set all global variables so we dont have to recalculate them when doing the actual manipulation
		mouseProfile_t ret_valid = { TRUE, TDPointerPhys, TDPointerPhys + 0x1, TDPointerPhys + 0x3, TDPointerPhys2, TDPointerPhys2 + 0x1, TDPointerPhys2 + 0x3 };

		// Return the pointer to the transfer buffer to indicate successful execution
		return ret_valid;
	}

	return ret;
}

/*
* Function:  mouseLogitechGProDriverTdCheck
* --------------------
* Verifies the TD of the Logitech G Pro mouse.
*
*  TDPointerPhys:				UINT64 (EFI_PHYSICAL_ADDRESS) to the TD that should be checked
*
*  returns:	TRUE if the check passed, FALSE otherwise
*
*/
BOOLEAN EFIAPI mouseLogitechGProDriverTdCheck(EFI_PHYSICAL_ADDRESS TDPointerPhys)
{
	LOG_VERB("[XHC] Checking TD at 0x%x \r\n", TDPointerPhys);

	// check the identifier at the start of the packet
	UINT8 tdCheck = 0;
	BOOLEAN status = pMemCpyForce((EFI_PHYSICAL_ADDRESS)&tdCheck, TDPointerPhys, sizeof(UINT8));

	LOG_VERB("[XHC] TD check 0x%x \r\n", tdCheck);

	// Check if it starts with 0x2 or 0x3 (depends on FW version)
	if ((tdCheck == 2 || tdCheck == 3) && status == TRUE)
	{
		// Found device successfully
		LOG_DBG("[XHC] Found Normal \r\n");

		return TRUE;
	}

	return FALSE;
}

#endif