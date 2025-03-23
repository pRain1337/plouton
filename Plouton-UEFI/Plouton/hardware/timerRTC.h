/*++
*
* Header file for onboard real time clock related functions
*
--*/

// edk2 includes
#include <Base.h>
#include <Library/IoLib.h>

#ifndef __plouton_timer_rtc_h__
#define __plouton_timer_rtc_h__

// Structures

// Definitions


/*
* CMOS Register offsets
*/
#define CMOS_PORT_ADDRESS 0x70
#define CMOS_PORT_DATA    0x71

// Functions

/*
* Function:  readCMOSRegister
* --------------------
* Reads the specified register from the RTC using the CMOS IO buffer.
*/
UINT8 readCMOSRegister(INT32 reg);

/*
* Function:  getCMOSTime
* --------------------
* Writes the current time in the global variables 'currentSecond' and 'currentMinute' that was read using the RTC.
*/
VOID getCMOSTime();

#endif