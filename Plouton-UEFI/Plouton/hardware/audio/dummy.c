#include "audioDriver.h"

#ifdef AUDIO_DRIVER_DUMMY

/*
* Function:  initAudioXHCI
* --------------------
* Dummy function for the dummy device.
*
*  returns:	The audioProfile struct, initialized set As TRUE if it was found
*
*/
audioProfile_t initAudioXHCI()
{
    audioProfile_t ret = { TRUE, 0, 0 };
    return ret;
}

#endif