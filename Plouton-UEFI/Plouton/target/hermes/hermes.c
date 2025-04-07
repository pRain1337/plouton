/*++
*
* Source file for Hermes debugging toolkit
*
--*/

// our includes
#include "hermes.h"

// Full source https://github.com/pRain1337/Hermes/blob/main/Hermes-SMM

// From NtKernelTools.c
extern WinCtx* winGlobal;

// Global variables
UINT64 found_packet = 0;
WinProc hermesProcess;
WinModule hermesModule;

const static unsigned char hermes_identifier[128] = {
0x0f, 0x2b, 0x34, 0x7f, 0x58, 0x40, 0x22, 0x61, 0x9c, 0xcc, 0x0f, 0xfa, 0x44, 0x0f, 0x59, 0x61,
0x40, 0x8b, 0x37, 0x6e, 0x8f, 0xe9, 0x1d, 0x2d, 0x81, 0x12, 0x65, 0x48, 0x6b, 0x8b, 0xcb, 0x0c,
0xa8, 0x25, 0x6a, 0xac, 0x9e, 0x66, 0xe3, 0x5d, 0x14, 0x24, 0x19, 0x45, 0x75, 0xab, 0xe8, 0xa1,
0x5a, 0x89, 0x99, 0xbe, 0xf9, 0xcc, 0x71, 0x81, 0x98, 0xe1, 0xce, 0x07, 0x9b, 0xc8, 0x59, 0x49,
0xa7, 0x5b, 0xfe, 0xa4, 0x05, 0x6a, 0xa6, 0x70, 0xd3, 0xe9, 0x0e, 0x85, 0xb3, 0xb1, 0x43, 0xde,
0xdc, 0xef, 0x3b, 0xfe, 0x0d, 0x20, 0xf1, 0xc6, 0x65, 0x92, 0x20, 0x97, 0xad, 0xa7, 0xcc, 0x32,
0x46, 0x05, 0x5c, 0xc9, 0xf2, 0xa1, 0xb8, 0x79, 0x92, 0x34, 0x03, 0xa4, 0x17, 0x20, 0x12, 0x0b,
0x35, 0x85, 0x81, 0x98, 0xe8, 0x76, 0xf5, 0x61, 0x0b, 0x7c, 0xe7, 0xfc, 0xcf, 0x97, 0x88, 0x81 };

const static UINT16 hermes_start_identifier = 0xf345;
const static UINT16 hermes_end_identifier = 0xa13f;

/*
* Function:  initializeHermes
* --------------------
*  x
*
*  _v:			x
*  _b:			x
*
*  returns: Returns false if it failed to dump, otherwise true
*
*/
BOOLEAN EFIAPI initializeHermes()
{
	BOOLEAN status = FALSE;
	LOG_INFO("[HER] Initializing Hermes...\r\n");

	// Dump Process List and search for process
	CHAR8* hermesProcessName = "hermes.exe";

	status = dumpSingleProcess(winGlobal, hermesProcessName, &hermesProcess);

	if (status == FALSE)
	{
		LOG_ERROR("[HER] Failed finding Hermes UM process\r\n");
		return FALSE;
	}
	else
	{
		LOG_INFO("[HER] Found Hermes UM Dir 0x%x\r\n", hermesProcess.dirBase);
	}

	// Get hermes.exe module, prepare Module list
	hermesModule.name = hermesProcessName;

	status = dumpSingleModule(winGlobal, &hermesProcess, &hermesModule);

	if (status == FALSE)
	{
		LOG_ERROR("[HER] Failed finding Hermes module \r\n");

		return FALSE;
	}

	// Find Memory Communication buffer
	if (hermesModule.baseAddress == 0 || hermesModule.sizeOfModule == 0)
	{
		LOG_ERROR("[HER] Invalid Module info \r\n");

		return FALSE;
	}
	else
	{
		LOG_INFO("[HER] Hermes Module Base 0x%x Size 0x%x\r\n", hermesModule.baseAddress, hermesModule.sizeOfModule);
	}

	for (UINT64 i = hermesModule.baseAddress; i < (hermesModule.baseAddress + hermesModule.sizeOfModule); i += 0x1000)
	{

		UINT64 pSrc = VTOP(i, hermesProcess.dirBase);

		if (pSrc == 0)
			continue;

		for (UINT64 k = 0; k < (0x1000 - 128); k++)
		{
			BOOLEAN bFound = TRUE;

			for (UINT32 j = 0; j < 128; j++)
			{
				if (hermes_identifier[j] != ((unsigned char*)pSrc)[k + j])
				{
					bFound = FALSE;
					break;
				}
			}

			if (bFound != FALSE)
			{
				found_packet = (UINT64)pSrc + k + 128;
			}
		}
	}

	LOG_INFO("[HER] Found Packet at: 0x%x sizeof(packet): 0x%x\r\n", found_packet, sizeof(hermes_packet));

	return TRUE;
}

/*
* Function:  pollCommands
* --------------------
*  x
*
*  _v:			x
*  _b:			x
*
*  returns: Returns false if it failed to dump, otherwise true
*
*/
VOID EFIAPI pollCommands()
{
	BOOLEAN status = FALSE;

	LOG_VERB("[HER] Polling for Hermes commands\r\n");

	if (found_packet != 0)
	{
		hermes_packet* pkt = (hermes_packet*)found_packet;

		// Check if there's a packet at the queue
		if (pkt->begin == hermes_start_identifier)
		{
			if (pkt->end == hermes_end_identifier)
			{
				if (pkt->command == HERMES_CMD_GET_DIRBASE)
				{
					LOG_INFO("[HER] HERMES_CMD_GET_DIRBASE request\r\n");

					// Null the startidentifer to act as mutex
					pkt->begin = 0;

					if (pkt->resultPointer == 0)
					{
						LOG_ERROR("[HER] Error: resultPtr not set \r\n");
						goto clear_packet;
					}

					if (pkt->dataPointer == 0)
					{
						LOG_ERROR("[HER] Error: dataPtr not set \r\n");

						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_INVALID_DATA_PTR;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					// Get Size of Process name
					UINT8 size = 0;
					vMemRead((EFI_PHYSICAL_ADDRESS)&size, pkt->dataPointer, sizeof(UINT8), hermesProcess.dirBase);

					if (size == 0 || size >= 64)
					{
						LOG_ERROR("[HER] Error: Size is %d\r\n", size);

						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_PROCNAME_TOO_LONG;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);

						return;
					}

					// Read Processname
					CHAR8 bName[64];

					status = vMemRead((EFI_PHYSICAL_ADDRESS)&bName, pkt->dataPointer + sizeof(UINT8), size, hermesProcess.dirBase);

					if (status == FALSE)
					{
						LOG_ERROR("[HER] Error: Failed reading process name \r\n");

						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_PROCNAME_TOO_LONG;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);

						return;
					}

					LOG_VERB("[HER] Read Name: %s\r\n", bName);

					// Get Dirbase
					UINT64 dirBase = 0;

					WinProc getDirBaseProcess;
					status = dumpSingleProcess(winGlobal, bName, &getDirBaseProcess);

					if (status == FALSE)
					{
						// Set result
						UINT64 temp = HERMES_STATUS_FAIL_FIND_PROC;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					if (getDirBaseProcess.dirBase != 0)
					{
						dirBase = getDirBaseProcess.dirBase;
					}
					else
					{
						dirBase = HERMES_STATUS_FAIL_FIND_DIRBASE;
					}

					// Set Result         
					vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&dirBase, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);

					return;
				}
				else if (pkt->command == HERMES_CMD_GET_MODULEDATA)
				{
					LOG_INFO("[HER] HERMES_CMD_GET_MODULEDATA request\r\n");

					// Null the startidentifer to act as mutex
					pkt->begin = 0;

					if (pkt->resultPointer == 0)
					{
						LOG_ERROR("[HER] Error: resultPtr not set \r\n");
						goto clear_packet;
					}

					if (pkt->dataPointer == 0)
					{
						LOG_ERROR("[HER] Error: dataPtr not set \r\n");

						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_INVALID_DATA_PTR;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					// Get Size of Process name
					UINT8 bProcessSize = 0;
					vMemRead((EFI_PHYSICAL_ADDRESS)&bProcessSize, pkt->dataPointer, sizeof(UINT8), hermesProcess.dirBase);

					if (bProcessSize == 0 || bProcessSize >= 64)
					{
						LOG_ERROR("[HER] Error: Size is %d \r\n", bProcessSize);

						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_PROCNAME_TOO_LONG;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);

						return;
					}

					// Read Processname
					CHAR8 bProcessName[64];

					status = vMemRead((EFI_PHYSICAL_ADDRESS)&bProcessName, pkt->dataPointer + sizeof(UINT8), bProcessSize, hermesProcess.dirBase);

					if (status == FALSE)
					{
						LOG_ERROR("[HER] Error: Failed reading process name \r\n");

						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_PROCNAME_TOO_LONG;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);

						return;
					}

					LOG_VERB("[HER] Processname: %s \r\n", bProcessName);

					// Get Size of Module name
					UINT8 pModuleSize = 0;
					vMemRead((EFI_PHYSICAL_ADDRESS)&pModuleSize, pkt->dataPointer + sizeof(UINT8) + bProcessSize, sizeof(UINT8), hermesProcess.dirBase);

					if (pModuleSize == 0 || pModuleSize >= 64)
					{
						LOG_ERROR("[HER] Error: Size is %d\r\n", pModuleSize);

						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_MODNAME_TOO_LONG;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);

						return;
					}

					// Read Modulename
					CHAR8 bModuleName[64];

					status = vMemRead((EFI_PHYSICAL_ADDRESS)&bModuleName, pkt->dataPointer + sizeof(UINT8) + bProcessSize + sizeof(UINT8), pModuleSize, hermesProcess.dirBase);

					if (status == FALSE)
					{
						LOG_ERROR("[HER] Error: Failed reading module name \r\n");

						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_MODNAME_TOO_LONG;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);

						return;
					}


					LOG_VERB("[HER] Modulename: %s \r\n", bModuleName);

					// Initialize process struct 
					WinProc getModuleDataProcess;
					status = dumpSingleProcess(winGlobal, bProcessName, &getModuleDataProcess);

					// Check status
					if (status == FALSE)
					{
						// Set result
						UINT64 temp = HERMES_STATUS_FAIL_FIND_PROC;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					// Process is ok, search module
					WinModule getModuleDataModule;
					getModuleDataModule.name = bModuleName;
					status = dumpSingleModule(winGlobal, &getModuleDataProcess, &getModuleDataModule);

					// Check status
					if (status == FALSE)
					{
						// Set result
						UINT64 temp = HERMES_STATUS_FAIL_FIND_MOD;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					if (getModuleDataModule.baseAddress == 0)
					{
						// Set result
						UINT64 temp = HERMES_STATUS_INVALID_MOD_BASE;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					if (getModuleDataModule.sizeOfModule == 0)
					{
						// Set result
						UINT64 temp = HERMES_STATUS_INVALID_MOD_SIZE;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					// Module is ok, prepare module dataPointer
					moduleData modData;
					modData.moduleBase = getModuleDataModule.baseAddress;
					modData.moduleSize = getModuleDataModule.sizeOfModule;

					// Set Result         
					vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&modData, sizeof(moduleData), hermesProcess.dirBase);

					return;
				}
				else if (pkt->command == HERMES_CMD_READ_VIRTUAL)
				{
					LOG_INFO("[HER] HERMES_CMD_READ_VIRTUAL request\r\n");

					// Null the startidentifer to act as mutex
					pkt->begin = 0;

					if (pkt->resultPointer == 0)
					{
						LOG_ERROR("[HER] Error: resultPtr not set \r\n");
						goto clear_packet;
					}

					if (pkt->dataPointer == 0)
					{
						LOG_ERROR("[HER] Error: dataPtr not set \r\n");

						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_INVALID_DATA_PTR;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					EFI_VIRTUAL_ADDRESS source = 0;
					vMemRead((EFI_PHYSICAL_ADDRESS)&source, pkt->dataPointer, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);

					if (source == 0)
					{
						LOG_ERROR("[HER] Error: Source 0 \r\n");
						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_INVALID_DATA_SOURCE;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					EFI_PHYSICAL_ADDRESS dirbase = 0;
					vMemRead((EFI_PHYSICAL_ADDRESS)&dirbase, pkt->dataPointer + sizeof(EFI_PHYSICAL_ADDRESS), sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);

					if (dirbase == 0)
					{
						LOG_ERROR("[HER] Error: Dirbase 0 \r\n");
						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_INVALID_DATA_DIRBASE;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					UINT64 size = 0;
					vMemRead((EFI_PHYSICAL_ADDRESS)&size, pkt->dataPointer + sizeof(EFI_PHYSICAL_ADDRESS) + sizeof(EFI_PHYSICAL_ADDRESS), sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);

					if (size == 0)
					{
						LOG_ERROR("[HER] Error: Size 0 \r\n");
						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_INVALID_DATA_SIZE;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					if (size > 0x1000)
					{
						LOG_ERROR("[HER] Error: Size >0x1000 \r\n");
						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_REQ_TOO_LARGE;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					LOG_VERB("[HER] Addr: 0x%x Dirbase: 0x%x Size: 0x%x \r\n", source, dirbase, size);

					// Try to read it
					unsigned char readBuffer[0x1000];

					status = vMemRead((EFI_PHYSICAL_ADDRESS)readBuffer, source, size, dirbase);

					if (status == FALSE)
					{
						LOG_ERROR("[HER] Error: vMem Fail \r\n");
						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_FAIL_VIRT_READ;
						vMemWrite(pkt->resultPointer + size, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					// Set Result       
					UINT16 temp = HERMES_STATUS_OK;
					vMemWrite(pkt->resultPointer + size, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(UINT16), hermesProcess.dirBase);
					vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)readBuffer, size, hermesProcess.dirBase);

					return;
				}
				else if (pkt->command == HERMES_CMD_WRITE_VIRTUAL)
				{
					LOG_INFO("[HER] HERMES_CMD_WRITE_VIRTUAL request\r\n");

					// Null the startidentifer to act as mutex
					pkt->begin = 0;

					if (pkt->resultPointer == 0)
					{
						LOG_ERROR("[HER] Error: resultPtr not set \r\n");
						goto clear_packet;
					}

					if (pkt->dataPointer == 0)
					{
						LOG_ERROR("[HER] Error: dataPtr not set \r\n");

						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_INVALID_DATA_PTR;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					UINT64 destination = 0;
					vMemRead((EFI_PHYSICAL_ADDRESS)&destination, pkt->dataPointer, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);

					if (destination == 0)
					{
						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_INVALID_DATA_DEST;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					UINT64 dirbase = 0;
					vMemRead((EFI_PHYSICAL_ADDRESS)&dirbase, pkt->dataPointer + sizeof(EFI_PHYSICAL_ADDRESS), sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);

					if (dirbase == 0)
					{
						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_INVALID_DATA_DIRBASE;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					UINT64 size = 0;
					vMemRead((EFI_PHYSICAL_ADDRESS)&size, pkt->dataPointer + sizeof(EFI_PHYSICAL_ADDRESS) + sizeof(EFI_PHYSICAL_ADDRESS), sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);

					if (size == 0)
					{
						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_INVALID_DATA_SIZE;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					if (size > 0x1000)
					{
						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_REQ_TOO_LARGE;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					// Try to read the buffer to write
					unsigned char writeBuffer[0x1000];

					status = vMemRead((EFI_PHYSICAL_ADDRESS)writeBuffer, (UINT64)pkt->dataPointer + sizeof(EFI_PHYSICAL_ADDRESS) + sizeof(EFI_PHYSICAL_ADDRESS) + sizeof(EFI_PHYSICAL_ADDRESS), size, hermesProcess.dirBase);

					if (status == FALSE)
					{
						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_FAIL_VIRT_WRITE;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					// Try to write the buffer to destination
					status = vMemWrite(destination, (UINT64)writeBuffer, size, hermesProcess.dirBase);

					if (status == FALSE)
					{
						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_FAIL_SBUFF_VIRTW;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					// Set Result  
					UINT16 temp = HERMES_STATUS_OK;
					vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(UINT16), hermesProcess.dirBase);

					return;
				}
				else if (pkt->command == HERMES_CMD_READ_PHYSICAL)
				{
					LOG_INFO("[HER] HERMES_CMD_READ_PHYSICAL request\r\n");

					// Null the startidentifer to act as mutex
					pkt->begin = 0;

					if (pkt->resultPointer == 0)
					{
						LOG_ERROR("[HER] Error: resultPtr not set \r\n");
						goto clear_packet;
					}

					if (pkt->dataPointer == 0)
					{
						LOG_ERROR("[HER] Error: dataPtr not set \r\n");

						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_INVALID_DATA_PTR;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					EFI_VIRTUAL_ADDRESS source = 0;
					vMemRead((EFI_PHYSICAL_ADDRESS)&source, pkt->dataPointer, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);

					if (source == 0)
					{
						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_INVALID_DATA_SOURCE;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					UINT64 size = 0;
					vMemRead((EFI_PHYSICAL_ADDRESS)&size, pkt->dataPointer + sizeof(EFI_PHYSICAL_ADDRESS), sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);

					if (size == 0)
					{
						LOG_ERROR("[HER] Error: Size 0 \r\n");
						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_INVALID_DATA_SIZE;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					if (size > 0x1000)
					{
						LOG_ERROR("[HER] Error: Size >0x1000 \r\n");
						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_REQ_TOO_LARGE;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					// Try to read it
					unsigned char readBuffer[0x1000];

					status = pMemCpy((UINT64)readBuffer, source, size);

					if (status == FALSE)
					{
						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_FAIL_PHYS_READ;
						vMemWrite(pkt->resultPointer + size, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					// Set Result       
					UINT16 temp = HERMES_STATUS_OK;
					vMemWrite(pkt->resultPointer + size, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(UINT16), hermesProcess.dirBase);
					vMemWrite(pkt->resultPointer, (UINT64)readBuffer, size, hermesProcess.dirBase);

					return;
				}
				else if (pkt->command == HERMES_CMD_WRITE_PHYSICAL)
				{
					LOG_INFO("[HER] HERMES_CMD_WRITE_PHYSICAL request\r\n");

					// Null the startidentifer to act as mutex
					pkt->begin = 0;

					if (pkt->resultPointer == 0)
					{
						LOG_ERROR("[HER] Error: resultPtr not set \r\n");
						goto clear_packet;
					}

					if (pkt->dataPointer == 0)
					{
						LOG_ERROR("[HER] Error: dataPtr not set \r\n");

						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_INVALID_DATA_PTR;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					UINT64 destination = 0;
					vMemRead((EFI_PHYSICAL_ADDRESS)&destination, pkt->dataPointer, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);

					if (destination == 0)
					{
						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_INVALID_DATA_SOURCE;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					UINT64 size = 0;
					vMemRead((EFI_PHYSICAL_ADDRESS)&size, pkt->dataPointer + sizeof(EFI_PHYSICAL_ADDRESS), sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);

					if (size == 0)
					{
						LOG_ERROR("[HER] Error: Size 0 \r\n");
						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_INVALID_DATA_SIZE;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					if (size > 0x1000)
					{
						LOG_ERROR("[HER] Error: Size >0x1000 \r\n");
						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_REQ_TOO_LARGE;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					// Try to read the buffer to write
					unsigned char writeBuffer[0x1000];

					status = vMemRead((EFI_PHYSICAL_ADDRESS)writeBuffer, pkt->dataPointer + sizeof(EFI_PHYSICAL_ADDRESS) + sizeof(EFI_PHYSICAL_ADDRESS), size, hermesProcess.dirBase);

					if (status == FALSE)
					{
						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_FAIL_SBUFF_PHYSW;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					// Try to write the buffer to destination
					status = pMemCpy(destination, (UINT64)writeBuffer, size);

					if (status == FALSE)
					{
						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_FAIL_PHYS_WRITE;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					// Set Result       
					UINT16 temp = HERMES_STATUS_OK;
					vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(UINT16), hermesProcess.dirBase);

					return;
				}
				else if (pkt->command == HERMES_CMD_VIRTUAL_TO_PHYSICAL)
				{
					LOG_INFO("[HER] HERMES_CMD_VIRTUAL_TO_PHYSICAL request\r\n");

					// Null the startidentifer to act as mutex
					pkt->begin = 0;

					if (pkt->resultPointer == 0)
					{
						LOG_ERROR("[HER] Error: resultPtr not set \r\n");
						goto clear_packet;
					}

					if (pkt->dataPointer == 0)
					{
						LOG_ERROR("[HER] Error: dataPtr not set \r\n");

						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_INVALID_DATA_PTR;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					EFI_VIRTUAL_ADDRESS source = 0;
					vMemRead((EFI_PHYSICAL_ADDRESS)&source, pkt->dataPointer, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);

					if (source == 0)
					{
						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_INVALID_DATA_SOURCE;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					UINT64 dirbase = 0;
					vMemRead((EFI_PHYSICAL_ADDRESS)&dirbase, pkt->dataPointer + sizeof(EFI_PHYSICAL_ADDRESS), sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);

					if (dirbase == 0)
					{
						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_INVALID_DATA_DIRBASE;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					UINT64 temp = VTOP(source, dirbase);

					EFI_PHYSICAL_ADDRESS physical = 0;

					if (temp == 0)
					{
						// Set Error Code as Result
						UINT64 tempError = HERMES_STATUS_FAIL_ADDR_TRANSLATION;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&tempError, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}
					else
					{
						physical = temp;
					}

					// Set Result         
					vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&physical, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);

					return;
				}
				else if (pkt->command == HERMES_CMD_DUMP_MODULE)
				{
					LOG_INFO("[HER] HERMES_CMD_DUMP_MODULE request\r\n");

					// Null the startidentifer to act as mutex
					pkt->begin = 0;

					if (pkt->resultPointer == 0)
					{
						LOG_ERROR("[HER] Error: resultPtr not set \r\n");
						goto clear_packet;
					}

					if (pkt->dataPointer == 0)
					{
						LOG_ERROR("[HER] Error: dataPtr not set \r\n");

						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_INVALID_DATA_PTR;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					UINT64 base = 0;
					vMemRead((EFI_PHYSICAL_ADDRESS)&base, pkt->dataPointer, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);

					if (base == 0)
					{
						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_INVALID_MOD_BASE;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					UINT64 dirbase = 0;
					vMemRead((EFI_PHYSICAL_ADDRESS)&dirbase, pkt->dataPointer + sizeof(EFI_PHYSICAL_ADDRESS), sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);

					if (dirbase == 0)
					{
						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_INVALID_DATA_DIRBASE;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					UINT64 size = 0;
					vMemRead((EFI_PHYSICAL_ADDRESS)&size, pkt->dataPointer + sizeof(EFI_PHYSICAL_ADDRESS) + sizeof(EFI_PHYSICAL_ADDRESS), sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);

					if (size == 0 || size < 0x1000)
					{
						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_INVALID_MOD_SIZE;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					// Copy page per page
					UINT16 invalid_counter = 0;

					for (UINT64 i = 0; i < size; i += 0x1000)
					{
						status = vMemCpy((EFI_VIRTUAL_ADDRESS)pkt->resultPointer + i, (EFI_VIRTUAL_ADDRESS)base + i, 0x1000, hermesProcess.dirBase, dirbase);

						if (status == FALSE)
						{
							invalid_counter += 1;

							if (invalid_counter >= 100)
							{
								// Abort
								LOG_VERB("[HER] Many errors... \r\n");
								//goto clear_packet;
							}

							LOG_VERB("[HER] FP 0x%x \r\n", i);

							// Maybe do some reporting? Something to think about
							// Dont have to fill the pages with 0 as they are 0 initialized
						}
					}

					// Set Result after the dump  
					UINT16 tempStatus = HERMES_STATUS_OK;
					vMemWrite(pkt->resultPointer + size, (EFI_PHYSICAL_ADDRESS)&tempStatus, sizeof(UINT16), hermesProcess.dirBase);

					return;
				}
				else if (pkt->command == HERMES_CMD_GET_MODULES)
				{
					LOG_INFO("[HER] HERMES_CMD_GET_MODULES request\r\n");

					// Null the startidentifer to act as mutex
					pkt->begin = 0;

					if (pkt->resultPointer == 0)
					{
						LOG_ERROR("[HER] Error: resultPtr not set \r\n");
						goto clear_packet;
					}

					if (pkt->dataPointer == 0)
					{
						LOG_ERROR("[HER] Error: dataPtr not set \r\n");

						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_INVALID_DATA_PTR;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					// Get Size of Process name
					UINT8 size = 0;
					vMemRead((EFI_PHYSICAL_ADDRESS)&size, pkt->dataPointer, sizeof(UINT8), hermesProcess.dirBase);

					if (size == 0 || size >= 64)
					{
						LOG_ERROR("[HER] Error: Size is %d \r\n", size);

						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_PROCNAME_TOO_LONG;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);

						return;
					}

					// Read Processname
					CHAR8 bName[64];

					status = vMemRead((EFI_PHYSICAL_ADDRESS)&bName, pkt->dataPointer + sizeof(UINT8), size, hermesProcess.dirBase);

					if (status == FALSE)
					{
						LOG_ERROR("[HER] Error: Failed reading process name \r\n");

						// Set Error Code as Result
						UINT64 temp = HERMES_STATUS_PROCNAME_TOO_LONG;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);

						return;
					}

					LOG_VERB("[HER] Read Name: %s \r\n", bName);

					WinProc getModulesProcess;
					status = dumpSingleProcess(winGlobal, bName, &getModulesProcess);

					// Check status
					if (status == FALSE)
					{
						// Set result
						UINT64 temp = HERMES_STATUS_FAIL_FIND_PROC;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					// Process is ok, get list of modules
					unsigned char names[0x1000];
					UINT8 moduleListCount = 0;
					status = dumpModuleNames(winGlobal, &getModulesProcess, FALSE, (UINT64)names, &moduleListCount);

					LOG_VERB("[HER] Modules: %d \r\n", moduleListCount);

					// Check status
					if (status == FALSE)
					{
						// Set result
						UINT64 temp = HERMES_STATUS_FAIL_FIND_MOD;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);
						return;
					}

					// Set Result       
					UINT16 temp = HERMES_STATUS_OK;
					vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(UINT16), hermesProcess.dirBase);
					vMemWrite(pkt->resultPointer + sizeof(EFI_PHYSICAL_ADDRESS), (EFI_PHYSICAL_ADDRESS)&moduleListCount, sizeof(UINT8), hermesProcess.dirBase);
					vMemWrite(pkt->resultPointer + sizeof(EFI_PHYSICAL_ADDRESS) + sizeof(UINT8), (UINT64)names, 0x1000 - sizeof(EFI_PHYSICAL_ADDRESS) - sizeof(UINT8), hermesProcess.dirBase);

					return;
				}
				else
				{
					// No command matched, return error code in status if possible
					if (pkt->resultPointer == 0)
					{
						UINT64 temp = HERMES_STATUS_INVALID_COMMAND;
						vMemWrite(pkt->resultPointer, (EFI_PHYSICAL_ADDRESS)&temp, sizeof(EFI_PHYSICAL_ADDRESS), hermesProcess.dirBase);

						LOG_ERROR("[HER] Error: command invalid \r\n");

						return;
					}
				}

				// If it reaches here we should clear the found_packet as we cant return an error code
			clear_packet:
				LOG_DBG("[HER] Nulling Packet.. \r\n");
				for (UINT8 k = 0; k < sizeof(UINT16) + sizeof(UINT8) + sizeof(EFI_PHYSICAL_ADDRESS) + sizeof(EFI_PHYSICAL_ADDRESS) + sizeof(UINT16); k++)
				{
					UINT8* nuller = (UINT8*)found_packet + k;
					*nuller = 0x0;
				}
			}
		}
	}
	return;
}