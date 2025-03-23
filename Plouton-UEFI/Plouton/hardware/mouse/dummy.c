#include "mouseDriver.h"

#ifdef MOUSE_DRIVER_DUMMY

/*
* Function:  initDummyMouseXHCI
* --------------------
* Dummy function for the dummy device.
*
*  MouseDriverTdCheck:			Pointer to the "MouseDriverTdCheck" function that is used to verify the device packets
*
*  returns:	The mouseProfile struct, initialized set As TRUE if it was found
*
*/
mouseProfile_t EFIAPI initDummyMouseXHCI(MouseDriverTdCheck MouseDriverTdCheckFun)
{
    mouseProfile_t ret = { TRUE, 0, 0, 0, 0, 0, 0 };
    return ret;
}

/*
* Function:  mouseDummyDriverTdCheck
* --------------------
* Dummy function to check the TD buffers.
*
*  TDPointerPhys:				UINT64 (EFI_PHYSICAL_ADDRESS) to the TD that should be checked
*
*  returns:	TRUE if the check passed, FALSE otherwise
*
*/
BOOLEAN EFIAPI mouseDummyDriverTdCheck(EFI_PHYSICAL_ADDRESS TDPointerPhys)
{
    return FALSE;
}

#endif