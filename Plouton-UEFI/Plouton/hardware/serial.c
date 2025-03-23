/*++
*
* Source file for serial related functions
*
--*/

// our includes
#include "serial.h"
#include "../memory/string.h"

/*
* UART Settings
*/
UINT8 m_Data = 8;
UINT8 m_Stop = 1;
UINT8 m_Parity = 0;
UINT8 m_BreakSet = 0;

/*
* Function:  SerialPortInitialize
* --------------------
* Initializes the serial device hardware at the supplied address.
*
*  Port:		UINT16 pointer to the serial hardware IO buffer
*  Baudrate:	UINTN Baudrate that should be used for communication
*
*  returns:	Nothing
*
*/
VOID SerialPortInitialize(UINT16 Port, UINTN Baudrate)
{
	// Map 5..8 to 0..3
	UINT8 Data = (UINT8)(m_Data - (UINT8)5);

	// Calculate divisor for baud generator
	UINTN Divisor = BAUDRATE_MAX / Baudrate;

	// Set communications format
	UINT8 OutputData = (UINT8)((DLAB << 7) | (m_BreakSet << 6) | (m_Parity << 3) | (m_Stop << 2) | Data);
	IoWrite8((UINTN)(Port + LCR_OFFSET), OutputData);

	// Configure baud rate
	IoWrite8((UINTN)(Port + BAUD_HIGH_OFFSET), (UINT8)(Divisor >> 8));
	IoWrite8((UINTN)(Port + BAUD_LOW_OFFSET), (UINT8)(Divisor & 0xff));

	// Switch back to bank 0
	OutputData = (UINT8)((~DLAB << 7) | (m_BreakSet << 6) | (m_Parity << 3) | (m_Stop << 2) | Data);
	IoWrite8((UINTN)(Port + LCR_OFFSET), OutputData);
}

/*
* Function:  SerialPortWrite
* --------------------
*  Writes the passed UINT8 Data to the serial port and waites for the acknowledgment.
*
*  Port:		UINT16 pointer to the serial hardware IO buffer
*  Data:		UINT8 data that is sent to the IO buffer
*
*  returns:	Nothing
*
*/
VOID SerialPortWrite(UINT16 Port, UINT8 Data)
{
	// Define the status variable that is used to check if the data can be sent
	UINT8 Status = 0;

	do
	{
		// Get the current status of the serial port
		Status = IoRead8(Port + LSR_OFFSET);
	} while ((Status & LSR_TXRDY) == 0); // Check if the port is ready

	// Send the byte to the serial port
	IoWrite8(Port, Data);

	return;
}

/*
* Function:  SerialPrintString
* --------------------
*  Writes the passed null-terminated string to the serial port using SerialPortWrite.
*  No check for maximum length included so make sure it is null-terminated.
*
*  text:		char* pointer to the null-terminated string
*
*  returns:	Nothing
*
*/
VOID SerialPrintString(const char* text)
{
	// We initiailize it again to make sure the data is sent
	SerialPortInitialize(SERIAL_PORT_0, SERIAL_BAUDRATE);

	// Loop until the position in the char array is 0 (null-terminated)
	while (*text)
	{
		// Send single byte via serial port
		SerialPortWrite(SERIAL_PORT_0, *text++);
	}

	return;
}

/*
* Function:  SerialPrintChar
* --------------------
*  Writes a single character to serial. The serial should be initialized elsewhere!
*
*  c:		character to write
*
*  returns:	Nothing
*
*/
VOID SerialPrintChar(const char c)
{
	SerialPortWrite(SERIAL_PORT_0, c);

	return;
}

/*
* Function:  SerialSendData
* --------------------
*  Writes the passed memory buffer directly to serial without converting it in any way using SerialPortWrite.
*
*  buf:			void* pointer to the memory buffer
*  len:			UINT8 length of the memory buffer (max. 255)
*
*  returns:	Nothing
*
*/
VOID SerialSendData(const VOID* buf, UINT8 len)
{
	// Loop through the memory buffer
	for (UINT64 i = 0; i < len; i++)
	{
		// Send the single byte of the current positionvia serial
		SerialPortWrite(SERIAL_PORT_0, ((const char*)buf)[i]);
	}

	return;
}

/*
* Function:  SerialPrintNumber
* --------------------
*  Converts the passed number to a string using the string library and then writes the string to serial.
*
*  _v:			UINT64 value that will be converted to a string
*  _b:			INT64 value that defines the type of passed number (hex - 16 or dec - 10)
*
*  returns:	Nothing
*
*/
VOID SerialPrintNumber(UINT64 _v, INT64 _b)
{
	char _r[100];

	// check validity as we only supported bitness between 2 and 36
	if (_b < 2 || _b > 36)
	{
		*_r = 0;
		return;
	}

	char* ptr = _r;
	char* ptr1 = _r;
	INT64 tmp_value;

	do {
		tmp_value = _v;
		_v /= _b;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - _v * _b)];
	} while (_v);

	// is the value neg?
	if (tmp_value < 0 && _b == 10)
	{
		*ptr++ = '-';
	}

	char tmp_char;

	*ptr-- = '\0';
	while (ptr1 < ptr)
	{
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}

	// Now write the converted number to the serial
	SerialPrintString(_r);

	return;
}

VOID EFIAPI SerialPrintf(const char* fmt, ...)
{
	if(!fmt)
	{
	return;
	}

	SerialPortInitialize(SERIAL_PORT_0, SERIAL_BAUDRATE);

	UINT32 len = strlen(fmt);
	VA_LIST vl;
	VA_START(vl, fmt);
	for(UINT32 i = 0; i < len; i++)
	{
		if(fmt[i] == '%')
		{
			// decimal
			if(fmt[i+1] == 'd')
			{
				INT64 d = VA_ARG(vl, INT64);
				SerialPrintNumber(d, 10);
				i++;
				continue;
			}
			// hex
			if(fmt[i+1] == 'x' || fmt[i+1] == 'p')
			{
				if(fmt[i+1] == 'p')
					SerialPrintString("0x");
				UINT64 val = VA_ARG(vl, UINT64);
				SerialPrintNumber(val, 16);
				i++;
				continue;
			}
			// string
			if(fmt[i+1] == 's')
			{
				char* s = VA_ARG(vl, char*);
				SerialPrintString(s);
				i++;
				continue;
			}
			// character
			if(fmt[i+1] == 'c')
			{
				char c = VA_ARG(vl, int);
				SerialPrintChar(c);
				i++;
				continue;
			}
			// just in case there was a wrong type input.
			// otherwise we would be stuck in an endless loop
			i++;
		}
		else
		{
			SerialPrintChar(fmt[i]);
		}
	}
	VA_END(vl);
	return;
}
