#include "audioDriver.h"

#ifdef AUDIO_DRIVER_LOGITECH_G_PRO_X_2_WIRELESS

// our includes
#include "../xhci.h"
#include "../../logging/logging.h"

/*
* Function:  initLogitechGProX2AudioXHCI
* --------------------
* Tries to initialize the audio driver for the Logitech G Pro X 2 headset.
* 
*  MBAR:						EFI_PHYSICAL_ADDRESS pointer to the Memory base address of the XHCI controller
*
*  returns:	The audioProfile struct, initialized set As TRUE if it was found
*
*/
audioProfile_t initLogitechGProX2AudioXHCI(EFI_PHYSICAL_ADDRESS MBAR)
{
    audioProfile_t ret = { 0, 0 };
    // This implementation is based on the following documents:
	// - Intel 600 series datasheet see: https://www.intel.com/content/www/us/en/content-details/742460/intel-600-series-chipset-family-for-iot-edge-platform-controller-hub-pch-datasheet-volume-2-of-2.html
	// - The SMM Rootkit Revisited: Fun with USB https://papers.put.as/papers/firmware/2014/schiffman2014.pdf

	// (Intel 18.1.11, Page 822) First we get the MBAR of the XHCI device
	// Check if we got a valid MBAR
	if (MBAR == 0)
	{
		LOG_ERROR("[XHC] Failed MBAR\r\n");

		// Return 0 to indicate failure
		return ret;
	}

	LOG_INFO("[XHC] Found MBAR %p\r\n", MBAR);

	// (Intel x.x.x, Page xxx) Now that we have the MBAR we can access the Memory Mapped registers, use it to get the Device Context Array Base Pointer which contains pointers to the devices.
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
	//	- contextType				- offset 0x4
	//	- contextPacketsize			- offset 0x6
	//  - contextAveragetrblength	- offset 0x10

	// Check for Logitech G Pro X 2 Wireless Headset
	ret.audioRingLocation = getEndpointRing(DCAB, 1, 0xC0, 0x60, TRUE, NULL); // Hardcoded for now

    if (ret.audioRingLocation == 0)
    {
		LOG_ERROR("[XHC] Failed finding endpoint ring \r\n");
        return ret;
    }

	LOG_VERB("[XHC] Found Audio lxw: %x\r\n", ret.audioRingLocation);

    ret.audioChannels = 8;
	ret.initialized = TRUE;

    return ret;
}

#endif