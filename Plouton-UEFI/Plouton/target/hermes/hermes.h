/*++
*
* Header file for Hermes debugging toolkit
*
--*/

#ifndef __plouton_hermes_h__
#define __plouton_hermes_h__

// our includes
#include "../../os/windows/windows.h"
#include "../../os/windows/NTKernelTools.h"
#include "../../general/config.h"
#include "../../logging/logging.h"

// Structures

// Use 1 byte alignment to have the struct the same size everywhere
#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

// Command Data structures
typedef struct {
	UINT64 moduleBase;
	UINT64 moduleSize;
} moduleData;

typedef struct {
	UINT64 source;
	UINT64 dirbase;
	UINT64 size;
} readRequest;

// Commands
typedef enum {
	HERMES_CMD_DUMMY = 0x0,                 // Dummy for initialization across compilers
	HERMES_CMD_GET_DIRBASE,                 // Command for reading the dirbase of a process
	HERMES_CMD_GET_MODULEDATA,              // Command for getting information of a module of a process
	HERMES_CMD_READ_VIRTUAL,                // Command for reading virtual memory
	HERMES_CMD_WRITE_VIRTUAL,               // Command for writing virtual memory
	HERMES_CMD_READ_PHYSICAL,               // Command for reading physical memory
	HERMES_CMD_WRITE_PHYSICAL,              // Command for writing physical memory
	HERMES_CMD_VIRTUAL_TO_PHYSICAL,         // Command for translating a virtual to a physical address
	HERMES_CMD_DUMP_MODULE,                 // Command for dumping a module (memory range)
	HERMES_CMD_GET_MODULES,                 // Command for returning all modules of a process
	HERMES_CMD_MAX                          // Amount of commands available
} hermes_command_t;


// Main Packet
PACK(typedef struct
{
	UINT16 begin;                         // Signals the start of the packet in memory
	UINT8  command;                       // Command that should be executed by smm
	UINT64 dataPointer;                   // Pointer to data that should be used by smm
	UINT64 resultPointer;                 // Pointer to the result, where smm should write
	UINT16 end;                           // Signals the end of the packet in memory
}) hermes_packet;

// Results
typedef enum {
	HERMES_STATUS_DUMMY = 0x0,              // Dummy for initialization across compilers
	HERMES_STATUS_INVALID_COMMAND,          // Invalid command
	HERMES_STATUS_INVALID_DATA_PTR,         // Invalid data pointer
	HERMES_STATUS_INVALID_RES_PTR,          // Invalid result pointer
	HERMES_STATUS_INVALID_IDENTIFIERS,      // Invalid identifiers
	HERMES_STATUS_FAIL_FIND_PROC,           // Failed finding the process
	HERMES_STATUS_FAIL_FIND_DIRBASE,        // Failed getting dirbase
	HERMES_STATUS_FAIL_FIND_MOD,            // Failed finding the module
	HERMES_STATUS_FAIL_READ_MOD_BASE,       // Failed reading module base
	HERMES_STATUS_FAIL_READ_MOD_SIZE,       // Failed reading module size
	HERMES_STATUS_PROCNAME_TOO_LONG,        // Too big process name size
	HERMES_STATUS_MODNAME_TOO_LONG,         // Too big module name size
	HERMES_STATUS_FAIL_VIRT_READ,           // Failed reading virtual address
	HERMES_STATUS_FAIL_VIRT_WRITE,          // Failed writing virtual address
	HERMES_STATUS_INVALID_DATA_SOURCE,      // Invalid data source
	HERMES_STATUS_INVALID_DATA_DIRBASE,     // Invalid data dirbase
	HERMES_STATUS_INVALID_DATA_SIZE,        // Invalid data size
	HERMES_STATUS_INVALID_DATA_DEST,        // Invalid data destination
	HERMES_STATUS_REQ_TOO_LARGE,            // Requested size is too big
	HERMES_STATUS_FAIL_SBUFF_VIRTW,         // Failed reading source buffer for virtual write
	HERMES_STATUS_FAIL_ADDR_TRANSLATION,    // Failed translating address
	HERMES_STATUS_FAIL_SBUFF_PHYSW,         // Failed reading source buffer for physical write
	HERMES_STATUS_INVALID_MOD_BASE,         // Invalid module base
	HERMES_STATUS_INVALID_MOD_SIZE,         // Invalid module size
	HERMES_STATUS_FAIL_PHYS_READ,           // Failed reading physical address
	HERMES_STATUS_FAIL_PHYS_WRITE,          // Failed writing physical address
	HERMES_STATUS_OK,                       // Everything worked
	HERMES_STATUS_MAX                       // Amount of status available
} hermes_status_t;

// Definitions

// Functions

/*
* Function:  initializeHermes
* --------------------
* Initializes the bare functionality for using hermes.
*/
BOOLEAN EFIAPI initializeHermes();

/*
* Function:  pollCommands
* --------------------
* Function that will check if the Hermes process has been launched and a debug session has been initialized.
*/
VOID EFIAPI pollCommands();

#endif