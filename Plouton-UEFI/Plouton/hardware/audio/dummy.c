#include "audioDriver.h"

#ifdef AUDIO_DRIVER_DUMMY

/*
* Function:  initAudioXHCI
* --------------------
* Dummy function for the dummy device.
* 
*  MBAR:						EFI_PHYSICAL_ADDRESS pointer to the Memory base address of the XHCI controller
*
*  returns:	The audioProfile struct, initialized set As TRUE if it was found
*
*/
audioProfile_t EFIAPI initAudioXHCI(EFI_PHYSICAL_ADDRESS MBAR)
{
    audioProfile_t ret = { TRUE, 0, 0 };
    return ret;
}

#endif