/*++
*
* Header file for serial related functions
*
--*/

#ifndef __plouton_serial_h__
#define __plouton_serial_h__

// our includes
#include "../general/config.h"
#include "../memory/memory.h"

// edk2 includes
#include <Base.h>
#include <Library/IoLib.h>

// Structures

// Definitions

/*
* for printing fix32_t
*/
typedef INT64 fix32_t;

/*
* UART Register Offsets
*/
#define BAUD_LOW_OFFSET         0x00
#define BAUD_HIGH_OFFSET        0x01
#define IER_OFFSET              0x01
#define LCR_SHADOW_OFFSET       0x01
#define FCR_SHADOW_OFFSET       0x02
#define IR_CONTROL_OFFSET       0x02
#define FCR_OFFSET              0x02
#define EIR_OFFSET              0x02
#define BSR_OFFSET              0x03
#define LCR_OFFSET              0x03
#define MCR_OFFSET              0x04
#define LSR_OFFSET              0x05
#define MSR_OFFSET              0x06

/*
* UART Register Bit Defines
*/
#define LSR_TXRDY               0x20
#define LSR_RXDA                0x01
#define DLAB                    0x01

/*
* Settings for the serial
*/
#define BAUDRATE_MAX 115200



// Function

/*
* Function:  SerialPortInitialize
* --------------------
* Initializes the serial device hardware at the supplied address with the passed Baudrate.
*/
VOID SerialPortInitialize(UINT16 Port, UINTN Baudrate);

/*
* Function:  SerialPortWrite
* --------------------
* Writes the passed data to the port and waits for the answer of the serial port
*/
VOID SerialPortWrite(UINT16 Port, UINT8 Data);

/*
* Function:  SerialSendData
* --------------------
* Writes a raw memory buffer to the serial using SerialPortWrite
*/
VOID SerialSendData(const VOID* buf, UINT8 len);

/*
* Function:  SerialPrintNumber
* --------------------
* Converts a hex or dec number into a string and writes it to the serial using SerialPortWrite
*/
VOID SerialPrintNumber(UINT64 _v, INT64 _b);


/*
* Function:  SerialPrintf
* --------------------
* Print a formatted string to serial console
*/
VOID EFIAPI SerialPrintf(const char* fmt, ...);

#endif