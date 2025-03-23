/*++
*
* Source file for functions related to SMI timer using PCH functionalities
*
--*/

// our includes
#include "timerPCH.h"
#include "../logging/logging.h"

/*
* Function:  pchInitUSB
* --------------------
* This function enables the XHCI_SMI_EN function in the XHCI, thus generating a SMI on each USB packet.
* This functionality enables us to generate a lot of SMIs and intercept the USB packets, before they reach the OS.
* This function has to be called early in the boot, as it gets locked otherwise by UEFI
*
*  returns:	FALSE if it failed, otherwise TRUE
*
*/
BOOLEAN pchInitUSB()
{
	// This feature has to be enabled in the PCH and close to boot.

	// First we want to get the ABASE from the XHCI device
	// Note 23.07.2024 - This device is not present anymore on newer devices after boot.
	UINT32 ABASE = PciRead32(PCI_LIB_ADDRESS(0, 31, 2, 0x20));

	// Null the last byte
	ABASE = ABASE & 0xFFFFF00;

	LOG_VERB("[TPC] ABASE: %x\r\n", ABASE);

	// Check if we got a valid address
	if (ABASE == 0xFFFFF00)
	{
		// On newer devices, we will get FFFFFFF as address as the BAR2 is missing
		// In this case we simply use a hardcoded BAR2 (0x1800), this has been confirmed to be the same on series 300 and 600 chipsets

		// Before proceeding with the hardcoded value, verify it first

		// Null check (this area should be null and it is not reserved on the manual)
		UINT32 nullCheck = IoRead32(0x1800 + 0x10);

		// Check if there is something at that location
		if (nullCheck != 0)
		{
			LOG_ERROR("[TPC] Null check failed \r\n");

			// Return false as we were unable to turn on the timer
			return FALSE;
		}

		// SMI_EN Check, check different bits which are enabled most of the times
		UINT32 smiEnCheck = IoRead32(0x1800 + PCH_SMI_EN);

		// Global SMI Enable Bit, should always be set
		if ((smiEnCheck & 0x1) == 0)
		{
			LOG_ERROR("[TPC] SMI EN Global SMI \r\n");

			// Return false as we were unable to turn on the timer
			return FALSE;
		}

		// TCO SMI Enable Bit, should always be set
		if ((smiEnCheck & 0x2000) == 0)
		{
			LOG_ERROR("[TPC] SMI EN TCO SMI \r\n");

			// Return false as we were unable to turn on the timer
			return FALSE;
		}

		// Checks passed, set ABASE to hardcoded and continue
		ABASE = 0x1800;
	}

	// Calculate the address of the SMI_EN variable in the ABASE
	UINT32 mSmiEnable = ABASE + PCH_SMI_EN;

	// Read the value of the I/O address which points to SMI_EN
	UINT32 SmiEnableVal = 0;
	SmiEnableVal = IoRead32(mSmiEnable);

	LOG_VERB("[TPC] SmiEnableVal: %x\r\n", SmiEnableVal);

	// Check if the read was successfull
	if (SmiEnableVal == 0xffffffff)
	{
		// Failed read
		LOG_ERROR("[TPC] Failed io read \r\n");

		// Return false as we were unable to turn on the timer
		return FALSE;
	}

	// Check if the Bit 31 (XHCI_SMI_EN) is already toggled
	if ((SmiEnableVal & 0x80000000) == 0)
	{
		// Not toggled yet, set the bit
		SmiEnableVal |= 0x80000000;
		// Write the new value to the I/O address
		IoWrite32(mSmiEnable, SmiEnableVal);
	}

	// Now also enable the SMI generation in the XHCI
	// First get the memory base address of the XHCI device
	UINT64 MBAR = 0;
	if (PciReadBuffer(PCI_LIB_ADDRESS(0, 0x14, 0x0, 0x10), 8, &MBAR) != 8) // See Intel series 200 page 1150
	{
		LOG_ERROR("[TPC] Failed PciReadBuffer \r\n");

		// Return false as we were unable to turn on the timer
		return FALSE;
	}

	// Null last bits
	MBAR = MBAR & 0xFFFFFFFFFFFFFF00;

	// Verify if we got a valid MBAR
	if (MBAR == 0)
	{
		LOG_VERB("[TPC] Failed MBAR \r\n");

		// Return false as we were unable to turn on the timer
		return FALSE;
	}

	LOG_VERB("[TPC] MBAR: %x\r\n", MBAR);

	// Now we want to read the 'Usb Legacy Support Control Status' which is not documented anymore on newer specifications

	// First define the pointer to the Usb Legacy Support Control Status
	EFI_PHYSICAL_ADDRESS pUSBLEGCTL = MBAR + XHCI_USBLegacySupportControlStatus;

	// Now read only a single byte of it, as sometimes it buggs out when reading more
	UINT8 USBLEGCTL = 0;
	pMemCpyForce((EFI_PHYSICAL_ADDRESS)&USBLEGCTL, pUSBLEGCTL, sizeof(UINT8));

	LOG_VERB("[TPC] ABASE: %x\r\n", USBLEGCTL);

	// Check if its already enabled or not
	if ((USBLEGCTL & 1) == 0)
	{
		// Its off, enable it
		USBLEGCTL |= 1;

		LOG_VERB("[TPC] USBLEGCTL2: %x\r\n", USBLEGCTL);

		// Write the change back
		pMemCpyForce(pUSBLEGCTL, (EFI_PHYSICAL_ADDRESS)&USBLEGCTL, sizeof(UINT8));
	}

	// Return true
	return TRUE;
}

/*
* Function:  pchClearUSB
* --------------------
*   This function clears the status Bits which are set by the XHCI SMI generation.
*   If these bits are not cleared, the system would otherwise get stuck eventually.
*
*  returns:	Nothing
*
*/
VOID pchClearUSB()
{
	// First get the memory base address of the XHCI device
	EFI_PHYSICAL_ADDRESS MBAR = 0;
	if (PciReadBuffer(PCI_LIB_ADDRESS(0, 0x14, 0x0, 0x10), 8, &MBAR) != 8) // See Intel series 200 page 1150
	{
		LOG_ERROR("[TPC] Failed PciReadBuffer \r\n");

		return;
	}

	// Null last bits
	MBAR = MBAR & 0xFFFFFFFFFFFFFF00;

	// Check if we got a valid MBAR
	if (MBAR == 0)
	{
		// MBAR is 0, abort
		LOG_VERB("[TPC] Zero MBAR \r\n");

		return;
	}

	// Now read out the USB Status variale
	UINT32 _usbsts = 0;
	pMemCpyForce((EFI_PHYSICAL_ADDRESS)&_usbsts, MBAR + XHCI_USBStatus, sizeof(UINT32));

	// Check if we read a valid status
	if (_usbsts == 0)
	{
		return;
	}

	// Check if there is an incoming SMI from USB
	if ((_usbsts & 8) != 0)
	{
		// There is one, write the bit again to force a reset
		_usbsts |= 8;

		// Write it back
		pMemCpyForce(MBAR + XHCI_USBStatus, (EFI_PHYSICAL_ADDRESS)&_usbsts, sizeof(UINT32));
	}

	return;
}