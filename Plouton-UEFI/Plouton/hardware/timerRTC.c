/*++
*
* Source file for onboard real time clock related functions
*
--*/

// edk2 includes
#include <Library/BaseLib.h>

// our includes
#include "timerRTC.h"

// Global variables that indicate the current time of the RTC
UINT8 currentSecond = 0;
UINT8 currentMinute = 0;
UINT8 currentDay = 0;
UINT8 currentMonth = 0;

/*
* Function:  readCMOSRegister
* --------------------
* Reads the passed CMOS register using the IO buffer of the CMOS.
*
*  reg:			INT32 register that should be read
*
*  returns:	Nothing
*
*/
UINT8 readCMOSRegister(INT32 reg)
{
	// Write the register we want to the CMOS Port address
	IoWrite8(CMOS_PORT_ADDRESS, reg);

	// Now read the CMOS Data address which will be the value of the register we requested
	return IoRead8(CMOS_PORT_DATA);
}

/*
* Function:  getCMOSTime
* --------------------
* Reads out the current time from the RTC using the IO buffer and writes it into the global variables 'currentSecond' and 'currentMinute'.
*
*
*  returns:	Nothing
*
*/
VOID getCMOSTime()
{
	// Read the current second (register 0x00)
	UINT8 second = readCMOSRegister(0x00);

	// Read the current minute (register 0x02)
	UINT8 minute = readCMOSRegister(0x02);

	// Read the current day (register 0x02)
	UINT8 day = readCMOSRegister(0x07);

	// Read the current month (register 0x02)
	UINT8 month = readCMOSRegister(0x08);

	// Read the register B as it contains important definitions (https://stanislavs.org/helppc/cmos_ram.html)
	UINT8 registerB = readCMOSRegister(0xB);

	// check if the data is in binary or not
	if (!(registerB & 0x04))
	{
		// Data is not in binary, means we have to convert it first (see https://wiki.osdev.org/CMOS#Getting_Current_Date_and_Time_from_RTC)
		second = (second & 0x0F) + ((second / 16) * 10);
		minute = (minute & 0x0F) + ((minute / 16) * 10);
		day = (day & 0x0F) + ((day / 16) * 10);
		month = (month & 0x0F) + ((month / 16) * 10);
	}

	// Pass the data to the global variabes
	currentSecond = second;
	currentMinute = minute;
	currentDay = day;
	currentMonth = month;
}