/*++
*
* Header file for windows NT Kernel parsing and analysis
*
--*/

#ifndef __plouton_ntkernel_h__
#define __plouton_ntkernel_h__

// our includes
#include "windows.h"
#include "../../memory/string.h"
#include "../../memory/memoryMapUEFI.h"
#include "../../memory/memory.h"
#include "../../general/config.h"
#include "../../logging/logging.h"

// Global variables we want to export
extern WinCtx* winGlobal;
extern BOOLEAN setupWindows;

/*
 * source for general structure:
 * https://github.com/Heep042/vmread
 */

// Structures

// Definitions

// from plouton.c
extern EFI_SMM_SYSTEM_TABLE2* gSmst2;

// Functions

/*
* Function:  getPML4
* --------------------
* Locates the kernel directory base, kernel entry by parsing the first 0x100 pages in memory.
*/
BOOLEAN getPML4(EFI_PHYSICAL_ADDRESS* PML4, EFI_VIRTUAL_ADDRESS* kernelEntry);

/*
* Function:  findNtosKrnl
* --------------------
* Locates the NT Kernel base which will be used to later locate the process list.
*/
BOOLEAN findNtosKrnl(EFI_VIRTUAL_ADDRESS kernelEntry, EFI_PHYSICAL_ADDRESS PML4, EFI_VIRTUAL_ADDRESS* ntKernel);

/*
* Function:  getExportList
* --------------------
* Finds the export table of the passed memory address and returns the pointer to the export passed as string.
*/
EFI_VIRTUAL_ADDRESS getExportList(EFI_VIRTUAL_ADDRESS moduleBase, EFI_PHYSICAL_ADDRESS directoryBase, const char* exportName);

/*
* Function:  getNTHeader
* --------------------
* Gets the header of the passed memory base and indicates if it is 64 or 32 bit.
*/
IMAGE_NT_HEADERS* getNTHeader(EFI_VIRTUAL_ADDRESS moduleBase, EFI_PHYSICAL_ADDRESS directoryBase, UINT8* header, UINT8* is64Bit);

/*
* Function:  parseExportTable
* --------------------
* Parses the given export table to search for the given function name.
*/
EFI_VIRTUAL_ADDRESS parseExportTable(EFI_VIRTUAL_ADDRESS moduleBase, EFI_PHYSICAL_ADDRESS directoryBase, const IMAGE_DATA_DIRECTORY* exports, const char* exportName);

/*
* Function:  getNTVersion
* --------------------
* Extracts the NT version of the running NT kernel.
*/
UINT16 getNTVersion(EFI_VIRTUAL_ADDRESS moduleBase, EFI_PHYSICAL_ADDRESS directoryBase);

/*
* Function:  getNTBuild
* --------------------
* Extracts the NT build of the running NT kernel.
*/
UINT16 getNTBuild(EFI_VIRTUAL_ADDRESS moduleBase, EFI_PHYSICAL_ADDRESS directoryBase);

/*
* Function:  setupOffsets
* --------------------
* Initializes the offset set which will be used in the other functions.
*/
BOOLEAN setupOffsets(UINT16 NTVersion, UINT16 NTBuild, WinOffsets* winOffsets);

/*
* Function:  findProcess
* --------------------
* Searches in the process list if the process with the name that was requested exists.
*/
EFI_PHYSICAL_ADDRESS findProcess(WinCtx* ctx, const char* processname);

/*
* Function:  dumpSingleProcess
* --------------------
* Searches in the process list for a process with the requested name and extracts important pointers from it.
*/
BOOLEAN dumpSingleProcess(const WinCtx* ctx, const char* processname, WinProc* process);

/*
* Function:  GetPeb
* --------------------
* Dumps the 64 bit process environment block (PEB) of the given process.
*/
PEB GetPeb(const WinCtx* ctx, const WinProc* process);

/*
* Function:  GetPeb32
* --------------------
* Dumps the 32 bit process environment block (PEB) of the given process.
*/
PEB32 GetPeb32(const WinCtx* ctx, const WinProc* process);

/*
* Function:  fillModuleList64
* --------------------
* Dumps the 64 bit module list of the given process in a list and writes specific of one module into the passed strct.
*/
BOOLEAN fillModuleList64(const WinCtx* ctx, const WinProc* Process, WinModule* Module, BOOLEAN* x86, EFI_PHYSICAL_ADDRESS moduleList, UINT8* moduleListCount);

/*
* Function:  fillModuleList86
* --------------------
* Dumps the 32 bit module list of the given process in a list and writes specific of one module into the passed strct.
*/
BOOLEAN fillModuleList86(const WinCtx* ctx, const WinProc* Process, WinModule* Module, EFI_PHYSICAL_ADDRESS moduleList, UINT8* moduleListCount);

/*
* Function:  dumpSingleModule
* --------------------
* Dumps all modules of the given process based on the architecture of the process.
*/
BOOLEAN dumpSingleModule(const WinCtx* ctx, const WinProc* process, WinModule* out_module);

/*
* Function:  dumpModuleNames
* --------------------
* Dumps the modules of the given process based on the architecture of the process and returns all their names.
*/
BOOLEAN dumpModuleNames(WinCtx* ctx, WinProc* Process, WinModule* Module, EFI_PHYSICAL_ADDRESS moduleList, UINT8* moduleListCount);

/*
* Function:  InitGlobalWindowsContext
* --------------------
* Initializes the winGlobal struct which is used in the other functions and contains relevant system information.
*/
BOOLEAN InitGlobalWindowsContext();

#endif