/*++
*
* Source file for windows NT Kernel parsing and analysis
*
--*/

// our includes
#include "NTKernelTools.h"

/*
 * Global vars for setting up the global
 * struct For Windows context.
 */
WinCtx* winGlobal = NULL;
BOOLEAN setupWindows = FALSE;

/*
* Function:  getPML4
* --------------------
*  This function scans the first 0x100 pages for the low stub which contains the PML4 (kernel directory base) and kernel entry
*
*  PML4:			UINT64* (EFI_PHYSICAL_ADDRESS*) where the PML4 will be written into
*  kernelEntry:		UINT64* (EFI_VIRTUAL_ADDRESS*) where the kernel entry will be written into
*
*  returns:	FALSE if it was unable to find the PML4, true if it found it
*
*/
BOOLEAN getPML4(EFI_PHYSICAL_ADDRESS* PML4, EFI_VIRTUAL_ADDRESS* kernelEntry)
{
	// Loop from 0 to 0x100000 and check for a certain structure
	EFI_PHYSICAL_ADDRESS page = 0;
	while (page < 0x100000)
	{
		// Skip first page
		page += 0x1000;

		// Check if address is okay before we try to read it
		if (IsAddressValid(page) == TRUE)
		{
			// Check the beginning of the page if it matches
			if (0x00000001000600E9 != (0xffffffffffff00ff & *(EFI_PHYSICAL_ADDRESS*)(void*)(page + 0x000))) { continue; } // START 

			// Check for the typical kernel pointer address (0xfffff8xxxxxxxxx is typical start due to paging) at offset 0x70
			if (0xfffff80000000000 != (0xfffff80000000003 & *(EFI_PHYSICAL_ADDRESS*)(void*)(page + 0x070))) { continue; } // KERNEL 

			// Check if there is a valid PML4 address at offset 0xa0
			if (0xffffff0000000fff & *(EFI_PHYSICAL_ADDRESS*)(void*)(page + 0x0a0)) { continue; }                         // PML4

			LOG_INFO("[NT] Found at: %p\r\n", page);

			// Found valid match, copy it into the pointers we got passed as parameters
			pMemCpy((EFI_PHYSICAL_ADDRESS)PML4, (EFI_PHYSICAL_ADDRESS)page + 0xa0, sizeof(EFI_PHYSICAL_ADDRESS));
			pMemCpy((EFI_PHYSICAL_ADDRESS)kernelEntry, (EFI_PHYSICAL_ADDRESS)page + 0x70, sizeof(EFI_VIRTUAL_ADDRESS));

			// Return true as we found them
			return TRUE;
		}
	}

	// If we get here, we were unable to locate them
	return FALSE;
}

/*
* Function:  findNtosKrnl
* --------------------
*  This function scans based on the kernelEntry for the start of the NT Kernel which will be used to locate the process list.
*
*  kernelEntry:		UINT64 (EFI_VIRTUAL_ADDRESS) kernel entry as a starting point for the search
*  PML4:			UINT64 (EFI_PHYSICAL_ADDRESS) dirbase of the kernel to translate virtual to physical
*  ntKernel:		UINT64* (EFI_VIRTUAL_ADDRESS*) where the kernel start will be written into
*
*  returns:	FALSE if it was unable to find the NT Kernel, true if it found it
*
*/
BOOLEAN findNtosKrnl(EFI_VIRTUAL_ADDRESS kernelEntry, EFI_PHYSICAL_ADDRESS PML4, EFI_VIRTUAL_ADDRESS* ntKernel)
{
	LOG_VERB("[NT] findNtosKrnl start\r\n");

	// Initialize some variables for searching
	UINT64 i, p, u, mask = 0xfffff;

	// Loop through the mask which is used to parse the address
	while (mask >= 0xfff)
	{
		// Do large 0x200000 steps based on the kernel entry we received 
		for (i = (kernelEntry & ~0x1fffff) + 0x10000000; i > kernelEntry - 0x20000000; i -= 0x200000)
		{
			// Check each page one by one
			for (p = 0; p < 0x200000; p += 0x1000)
			{
				LOG_DBG("[NT] Checking virtual: %p\r\n", i + p);

				// First translate this address we got (which is virtual) into a physical address
				EFI_PHYSICAL_ADDRESS physicalP = 0;
				physicalP = VTOP(i + p, PML4);

				// Debug print only when we actually have an address
				LOG_DBG("[NT] Checking physical: %p\r\n", physicalP);

				// Check if we got a valid physical address
				if (IsAddressValid(physicalP) == TRUE && physicalP != 0)
				{
					// valid physical address received! Check if the page starts with the typical DOS signature (MZ)
					if (((i + p) & mask) == 0 && *(short*)(void*)(physicalP) == IMAGE_DOS_SIGNATURE)
					{
						// DOS Signature matched, declare some variables outside of the loop
						int kdbg = 0, poolCode = 0;
						for (u = 0; u < 0x1000; u++)
						{
							// Make sure each byte is valid (better be safe than sorry)
							if (IsAddressValid(p + u) == FALSE)
								continue;

							// Check for the kdbg signature and save it in the variable
							kdbg = kdbg || *(EFI_PHYSICAL_ADDRESS*)(void*)(physicalP + u) == 0x4742444b54494e49;

							// check for the poolcode signature and save it in the variable
							poolCode = poolCode || *(EFI_PHYSICAL_ADDRESS*)(void*)(physicalP + u) == 0x45444f434c4f4f50;

							// Check if both are valid
							if (kdbg & poolCode)
							{
								// Found the start of the NTKernel !
								LOG_DBG("[NT] nt kernel found\r\n");

								// Save it in the passed variable
								*ntKernel = i + p;

								// Return true as we found it
								return TRUE;
							}
						}
					}
				}
			}
		}

		// Reduce the mask to search for more
		mask = mask >> 4;
	}

	// If we reach here, it means we did not locate the NT Kernel
	return FALSE;
}

/*
* Function:  getExportList
* --------------------
*  Gets the exportlist of the given module and then searches for the physical address of the export function
*
*  moduleBase:		UINT64 (EFI_VIRTUAL_ADDRESS) Base of the module/process which will be analyzed
*  directoryBase:	UINT64 (EFI_PHYSICAL_ADDRESS) Directory base required to translate the virtual to physical addresses
*  exportName:		String for the export that should be looked up
*
*  returns:	0 if it fails, otherwise the address of the requested export
*
*/
EFI_VIRTUAL_ADDRESS getExportList(EFI_VIRTUAL_ADDRESS moduleBase, EFI_PHYSICAL_ADDRESS directoryBase, const char* exportName)
{
	// Variable that indicates if the module is 64 bit or 32 bit
	UINT8 is64 = 0;

	// Allocate a temporary buffer to hold the header
	UINT8 header[HEADER_SIZE];

	// Get the NT headers of the module we passed and find out if it is a 64 bit module
	IMAGE_NT_HEADERS64* ntHeader64 = getNTHeader(moduleBase, directoryBase, header, &is64);

	// Check if we got valid headers
	if (!ntHeader64)
	{
		LOG_ERROR("[NT] Failed getting NTHeader \r\n");

		// Failed getting header
		return 0;
	}

	// Get the x86 version
	IMAGE_NT_HEADERS32* ntHeader32 = (IMAGE_NT_HEADERS32*)ntHeader64;

	// Prepare a pointer to the export table which will be set depending on the architecture of the header
	IMAGE_DATA_DIRECTORY* exportTable = NULL;

	// Check the format of the headers
	if (is64)
	{
		LOG_INFO("[NT] System architecture is x64 \r\n");

		// Use the 64 bit structs to get the pointer to the export table
		exportTable = ntHeader64->OptionalHeader.DataDirectory + IMAGE_DIRECTORY_ENTRY_EXPORT;
	}
	else
	{
		LOG_INFO("[NT] System architecture is x86 \r\n");

		// Use the 32 bit structs to get the pointer to the export table
		exportTable = ntHeader32->OptionalHeader.DataDirectory + IMAGE_DIRECTORY_ENTRY_EXPORT;
	}

	// Check if we got a valid export table
	if (exportTable == 0)
	{
		LOG_ERROR("[NT] Export table is 0! \r\n");

		// Return 0 as we got an invalid export table pointer
		return 0;
	}

	// Now parse the exprot table and search for the actual function
	EFI_VIRTUAL_ADDRESS PsInitial = parseExportTable(moduleBase, directoryBase, exportTable, exportName);

	// Return the pointer to the function (also return if it is 0 to indicate failure)
	return PsInitial;
}

/*
* Function:  getNTHeader
* --------------------
*  Checks if we got a valid NT header and copies it into the passed memory buffer. Also indicates the architecture of the process.
*
*  moduleBase:		UINT64 (EFI_VIRTUAL_ADDRESS) Base of the module/process which will be analyzed
*  directoryBase:	UINT64 (EFI_PHYSICAL_ADDRESS) Directory base required to translate the virtual to physical addresses
*  header:			UINT8* pointer to a memory buffer where the header struct will be copied into
*  is64Bit:			UINT8* pointer which will indicate if the module is 64 or 32 bit
*
*  returns: The NT Headers of the module
*
*/
IMAGE_NT_HEADERS* getNTHeader(EFI_VIRTUAL_ADDRESS moduleBase, EFI_PHYSICAL_ADDRESS directoryBase, UINT8* header, UINT8* is64Bit)
{
	// Copy the header (full page) into our page so we can directly access the structs
	vMemRead((EFI_PHYSICAL_ADDRESS)header, moduleBase, HEADER_SIZE, directoryBase);

	// First parse it as DOS header
	const IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)(void*)header;

	// Check if it is a valid DOS header (MZ)
	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
	{
		LOG_ERROR("[NT] Invalid NTHeader Magic \r\n");

		// Invalid DOS Header, return null
		return NULL;
	}

	// Valid dos header, get the NT Headers now
	IMAGE_NT_HEADERS* ntHeader = (IMAGE_NT_HEADERS*)(void*)(header + dosHeader->e_lfanew);

	// Check if we got a valid NT header by checking the size and the signature (PE00)
	if ((UINT8*)ntHeader - header > HEADER_SIZE - 0x200 || ntHeader->Signature != IMAGE_NT_SIGNATURE)
	{
		LOG_ERROR("[NT] Invalid NTHeader Signature \r\n");

		// Invalid NT header, return null
		return NULL;
	}

	// Check if it is 64 bit or 32 bit header (future proof for ARM!)
	if (ntHeader->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC && ntHeader->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
	{
		LOG_ERROR("[NT] Not x86 and x64 \r\n");

		// Either struct is changed or not supported architecture, return null
		return NULL;
	}

	// Check if the magic matches the 64 bit matches and if yes, set is64Bit value to 1
	*is64Bit = ntHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC;

	// Return the pointer to the header to indicate successful execution
	return ntHeader;
}

/*
* Function:  parseExportTable
* --------------------
*  Parses the given exporttable and searches for the given name of a function in the exportname.
*
*  moduleBase:		UINT64 (EFI_VIRTUAL_ADDRESS) Base of the module/process which will be analyzed
*  directoryBase:	UINT64 (EFI_PHYSICAL_ADDRESS) Directory base required to translate the virtual to physical addresses
*  exports:			Pointer to the export table of the module from the NT Header
*  exportName:		String for the export that should be looked up
*
*  returns:	Returns 0 if it failed, otherwise returns pointer to the exported function requested as string
*
*/
EFI_VIRTUAL_ADDRESS parseExportTable(EFI_VIRTUAL_ADDRESS moduleBase, EFI_PHYSICAL_ADDRESS directoryBase, const IMAGE_DATA_DIRECTORY* exports, const char* exportName)
{
	// First check if the exporttable pointer we got is valid
	if (exports->Size < sizeof(IMAGE_EXPORT_DIRECTORY) || exports->Size > 0x7fffff || exports->VirtualAddress == moduleBase)
	{
		LOG_ERROR("[NT] Invalid exporttable \r\n");

		// Return 0 as exporttable is invalid
		return 0;
	}

	// We will read the whole exporttable temporarily into SMM

	// Calculate the size of the exporttable and add one page to be sure
	UINT64 realSize = (exports->Size & 0xFFFFFFFFFFFFF000) + 0x1000;

	// Allocate SMRAM pages for the exporttable
	EFI_PHYSICAL_ADDRESS physAddr;
	gSmst2->SmmAllocatePages(AllocateAnyPages, EfiRuntimeServicesData, (realSize / 0x1000), &physAddr);

	// Check if we successfully allocated the pages
	if (physAddr == 0)
	{
		LOG_ERROR("[NT] Failed allocating pages \r\n");

		// Failed allocating pages, thus can not proceed, abort by returning 0
		return 0;
	}

	LOG_INFO("[NT] Allocated Memory at: %p\r\n", physAddr);

	// Cast the allocated page to a byte pointer
	char* buf = (char*)physAddr;

	// Loop through all pages of the export table
	for (int i = 0; i < (realSize / 0x1000); i++)
	{
		// First calculate the physical address of the page
		EFI_PHYSICAL_ADDRESS physicalP = VTOP(moduleBase + exports->VirtualAddress + (i * 0x1000), directoryBase);

		// Check if we got a valid physical address
		if (IsAddressValid(physicalP) == TRUE && physicalP != 0)
		{
			// Valid address, read it now
			if (pMemCpy((EFI_PHYSICAL_ADDRESS)(buf + (i * 0x1000)) & 0xFFFFFFFFFFFFF000, physicalP & 0xFFFFFFFFFFFFF000, 0x1000) == FALSE)
			{
				LOG_DBG("[NT] ET-Failed physread! \r\n");
			}
		}
		else
		{
			LOG_DBG("[NT] ET-Invalid Address! \r\n");
		}
	}

	// Exporttable reading finished, set the export directory variable to the copied exporttable
	IMAGE_EXPORT_DIRECTORY* exportDir = (IMAGE_EXPORT_DIRECTORY*)(void*)buf;

	LOG_INFO("[NT] Finished Export Table.. NameAmount %d\r\n", exportDir->NumberOfNames);

	// Set the end of the exporttable to 0
	buf[exports->Size] = 0;

	// Check if we got a valid export table by checking if it has names
	if (!exportDir->NumberOfNames || !exportDir->AddressOfNames)
	{
		LOG_ERROR("[NT] Export Table invalid! NON is 0 \r\n");

		// Free the allocated buffer
		gSmst2->SmmFreePages(physAddr, (realSize / 0x1000));

		// As the export table is invalid, return 0
		return 0;
	}

	// Get the offset of the export table from the modulebase
	UINT32 exportOffset = exports->VirtualAddress;

	// Set the pointer to the names in the export table
	const UINT32* names = (UINT32*)(void*)(buf + exportDir->AddressOfNames - exportOffset);

	// Check if we got a valid pointer still in the size of the export table
	if (exportDir->AddressOfNames - exportOffset + exportDir->NumberOfNames * sizeof(UINT32) > exports->Size)
	{
		LOG_ERROR("[NT] Export Table invalid! AddressOfNames \r\n");

		// Free the allocated buffer
		gSmst2->SmmFreePages(physAddr, (realSize / 0x1000));

		// As the export table is invalid, return 0
		return 0;
	}

	// Set the pointer to the ordinals in the export table
	const UINT16* ordinals = (UINT16*)(void*)(buf + exportDir->AddressOfNameOrdinals - exportOffset);

	// Check if we got a valid pointer still in the size of the export table
	if (exportDir->AddressOfNameOrdinals - exportOffset + exportDir->NumberOfNames * sizeof(UINT16) > exports->Size)
	{
		LOG_ERROR("[NT] Export Table invalid! AddressOfNameOrdinals \r\n");

		// Free the allocated buffer
		gSmst2->SmmFreePages(physAddr, (realSize / 0x1000));

		// As the export table is invalid, return 0
		return 0;
	}

	// Set the pointer to the functions in the export table
	const UINT32* functions = (UINT32*)(void*)(buf + exportDir->AddressOfFunctions - exportOffset);

	// Check if we got a valid pointer still in the size of the export table
	if (exportDir->AddressOfFunctions - exportOffset + exportDir->NumberOfFunctions * sizeof(UINT32) > exports->Size)
	{
		LOG_ERROR("[NT] Export Table invalid! AddressOfFunctions \r\n");

		// Free the allocated buffer
		gSmst2->SmmFreePages(physAddr, (realSize / 0x1000));

		// As the export table is invalid, return 0
		return 0;
	}

	// Initialize a variable which will be set to the pointer to the function if found
	EFI_VIRTUAL_ADDRESS found = 0;

	// Loop through all names in the export table
	for (UINT32 i = 0; i < exportDir->NumberOfNames; i++)
	{
		// Check if the pointers we got from the exporttable is valid
		if (names[i] > exports->Size + exportOffset || names[i] < exportOffset || ordinals[i] > exportDir->NumberOfNames)
			continue;

		// Check if the name of the entry in the exporttable matches the requested function
		if (!strcmp(exportName, buf + names[i] - exportOffset))
			found = moduleBase + functions[ordinals[i]]; // Match found! Set the 'found' variable to the address
	}

	// Free the allocated buffer
	gSmst2->SmmFreePages(physAddr, (realSize / 0x1000));

	// Return the address even if we did not find anything
	return found;
}

/*
* Function:  getNTVersion
* --------------------
*  Gets the current version of the running NT kernel on the system by searching for the 'RtlGetVersion' function and parsing the code.
*
*  moduleBase:		UINT64 (EFI_VIRTUAL_ADDRESS) Base of the module/process which will be analyzed
*  directoryBase:	UINT64 (EFI_PHYSICAL_ADDRESS) Directory base required to translate the virtual to physical addresses
*
*  returns:	Returns 0 if it failed finding the version, otherwise the version number
*
*/
UINT16 getNTVersion(EFI_VIRTUAL_ADDRESS moduleBase, EFI_PHYSICAL_ADDRESS directoryBase)
{
	// Generate ntoskrnl export list and return the address of the export 'RtlGetVersion'
	EFI_VIRTUAL_ADDRESS getVersion = getExportList(moduleBase, directoryBase, "RtlGetVersion");

	// Check if we successfully located the function
	if (!getVersion)
	{
		LOG_ERROR("[NT] Failed finding RtlGetVersion \r\n");

		// Failed finding function, return 0
		return 0;
	}

	// Copy the first 100 bytes of the function in a buffer for analysis
	char buf[0x100];
	vMemRead((EFI_PHYSICAL_ADDRESS)buf, getVersion, 0x100, directoryBase);

	// Initialize some variables which are used for the analysis
	char major = 0, minor = 0;

	/* Find writes to rcx +4 and +8 -- those are our major and minor versions */
	for (char* b = buf; b - buf < 0xf0; b++)
	{
		// First check for if both major and minor is not yet set
		if (!major && !minor)
			// Check for the signature
			if (*(UINT32*)(void*)b == 0x441c748)
				// Directly return the address
				return ((UINT16)b[4]) * 100 + (b[5] & 0xf);

		// Check if major is not yet set and check the current position for the signature
		if (!major && (*(UINT32*)(void*)b & 0xfffff) == 0x441c7)
			major = b[3]; // Found major version at this position, set it in variable

		// Check if minor is not yet set and check the current position for the signature
		if (!minor && (*(UINT32*)(void*)b & 0xfffff) == 0x841c7)
			minor = b[3]; // Found minor version at this position, set it in variable
	}

	// Check if our minor version is valid (set to 0 if higher than 100)
	if (minor >= 100)
		minor = 0;

	// calculate the real version number and return it immediatly
	return ((UINT16)major) * 100 + minor;
}

/*
* Function:  getNTBuild
* --------------------
*  Gets the current build of the running NT kernel on the system by searching for the 'RtlGetBuild' function and parsing the code.
*
*  moduleBase:		UINT64 (EFI_VIRTUAL_ADDRESS) Base of the module/process which will be analyzed
*  directoryBase:	UINT64 (EFI_PHYSICAL_ADDRESS) Directory base required to translate the virtual to physical addresses
*
*  returns:	Returns 0 if it failed finding the build, otherwise the build number
*
*/
UINT16 getNTBuild(EFI_VIRTUAL_ADDRESS moduleBase, EFI_PHYSICAL_ADDRESS directoryBase)
{
	// Generate ntoskrnl export list and return the address of the export 'NtBuildNumber'
	EFI_VIRTUAL_ADDRESS getBuild = getExportList(moduleBase, directoryBase, "NtBuildNumber");

	// Check if we successfully located the function
	if (getBuild != 0)
	{
		// Copy the first 4 bytes of the function into a local variable
		UINT32 build = 0;
		vMemRead((EFI_PHYSICAL_ADDRESS)&build, getBuild, sizeof(build), directoryBase);

		// Check if we found a valid value
		if (build)
			return build & 0xffffff; // Return it
	}

	// This is a fallback mechanism as NT kernel is sometimes buggy after updates / unclean restarts
	// Extract the build from the 'RtlGetVersion' function instead

	// Generate ntoskrnl export list and return the address of the export 'RtlGetVersion'
	EFI_VIRTUAL_ADDRESS getVersion = getExportList(moduleBase, directoryBase, "RtlGetVersion");

	// Check if we successfully located the function
	if (!getVersion)
	{
		LOG_ERROR("[NT] Failed finding RtlGetVersion \r\n");

		// Failed locating the function, abort execution
		return 0;
	}

	// Copy the full page of the function into a local buffer
	char buf[0x1000];
	vMemRead((EFI_PHYSICAL_ADDRESS)buf, getVersion, 0x1000, directoryBase);

	/* Find writes to rcx +12 -- that's where the version number is stored. These instructions are not on XP, but that is simply irrelevant. */
	for (char* b = buf; b - buf < 0xf0; b++)
	{
		// Get the UINT32 value of the current position
		UINT32 val = *(UINT32*)(void*)b & 0xffffff;

		// Check if it matches one of our signatures
		if (val == 0x0c41c7 || val == 0x05c01b)
			return *(UINT32*)(void*)(b + 3); // Return the build number
	}

	// If we get here, both methods were unable locating the build number and we return 0
	return 0;
}

/*
* Function:  setupOffsets
* --------------------
*  This function initializes the offset set which will be used in the further functions to analyze the NT Kernel
*  For updating, check out https://www.vergiliusproject.com/kernels/x64
*
*  NTVersion:		UINT16 version of the NT kernel
*  NTBuild:			UINT16 build of the NT kernel
*  winOffsets:		Pointer to the winOffsets struct in the winGlobal struct
*
*  returns:	Returns true if it found the offset set or false if it did not
*
*/
BOOLEAN setupOffsets(UINT16 NTVersion, UINT16 NTBuild, WinOffsets* winOffsets)
{
	// First check for the version
	switch (NTVersion)
	{
	case 601: /* W7 */
		winOffsets->apl = 0x188;
		winOffsets->imageFileName = 0x2e0;
		winOffsets->dirBase = 0x28;
		winOffsets->peb = 0x338;

		// Check if it is a build with a special offset
		if (NTBuild == 7601)  /* SP1 */
		{
			winOffsets->imageFileName = 0x2d8;
		}

		break;

	case 602: /* W8 */
		winOffsets->apl = 0x2e8;
		winOffsets->imageFileName = 0x438;
		winOffsets->dirBase = 0x28;
		winOffsets->peb = 0x338;

		break;

	case 603: /* W8.1 */
		winOffsets->apl = 0x2e8;
		winOffsets->imageFileName = 0x438;
		winOffsets->dirBase = 0x28;
		winOffsets->peb = 0x338;

		break;

	case 1000: /* W10 & W11 */
		winOffsets->apl = 0x2e8;
		winOffsets->stackCount = 0x23c;
		winOffsets->imageFileName = 0x450;
		winOffsets->dirBase = 0x28;
		winOffsets->peb = 0x3f8;
		winOffsets->virtualSize = 0x338;
		winOffsets->vadroot = 0x628;

		// Check if it is a build with a special offset
		if (NTBuild >= 18362) /* Version 1903 or higher */
		{
			winOffsets->apl = 0x2f0;
			winOffsets->virtualSize = 0x340;
			winOffsets->vadroot = 0x658;
		}

		// Check if it is a build with a special offset
		if (NTBuild >= 19041) /* version 2004 */
		{
			winOffsets->apl = 0x448;
			winOffsets->stackCount = 0x348;
			winOffsets->imageFileName = 0x5a8;
			winOffsets->dirBase = 0x28;
			winOffsets->peb = 0x550;
			winOffsets->virtualSize = 0x498;
			winOffsets->vadroot = 0x7d8;
		}

		// Check if it is a build with a special offset
		if (NTBuild >= 26100) /* version 24H2 */
		{
			winOffsets->apl = 0x1d8; // UniqueProcessId + 0x8
			winOffsets->stackCount = 0x108;
			winOffsets->imageFileName = 0x338;
			winOffsets->dirBase = 0x28;
			winOffsets->peb = 0x2e0;
			winOffsets->virtualSize = 0x228;
			winOffsets->vadroot = 0x558;
		}

		break;

	default:
		// If we get here we did not match a version, thus return FALSE
		return FALSE;
	}

	// If we get here we matched a version and have set the offsets, thus return TRUE
	return TRUE;
}

/*
* Function:  findProcess
* --------------------
*  This function takes the initialized windows structs and searches in the process list if the requested process name exists.
*
*  ctx:				Pointer to the WinCtx struct that contains offsets / pointers to the important NT kernel structs.
*  processname:		String of the process name that should be found
*
*  returns:	Returns 0 if the process was not found, returns the directory base of the process requested if found
*
*/
EFI_PHYSICAL_ADDRESS findProcess(WinCtx* ctx, const char* processname)
{
	// Set the temporary variables to the initial process (first process)
	EFI_PHYSICAL_ADDRESS curProc = ctx->initialProcess.physProcess;
	EFI_VIRTUAL_ADDRESS virtProcess = ctx->initialProcess.process;

	LOG_INFO("[NT] curProc: %p virtProcess: %p\r\n", curProc, virtProcess);

	// This variable keeps track if we found the system process -> this will be used to cause a re initialization of the structure if we somehow miss it
	BOOLEAN foundSystemProcess = FALSE;

	// Now go through the process linked list until we found something and the next process is not the system process
	UINT32 size = 0;
	while (!size || curProc != ctx->initialProcess.physProcess)
	{
		// First get the stackcount of the EPROCESS struct
		EFI_PHYSICAL_ADDRESS* stackCount = 0;

		// Check if the address with the offset is valid
		if (IsAddressValid(curProc + ctx->offsets.stackCount) == TRUE)
		{
			// Directly dereference the stack count
			stackCount = (EFI_PHYSICAL_ADDRESS*)(curProc + ctx->offsets.stackCount);

			LOG_DBG("[NT] stackCount: %d at %p\r\n", *stackCount, curProc + ctx->offsets.stackCount);
		}

		// Now get the directory base from the EPROCESS struct
		EFI_PHYSICAL_ADDRESS* dirBase = 0;

		// Check if the address with the offset is valid
		if (IsAddressValid(curProc + ctx->offsets.dirBase) == TRUE)
		{
			// Directly dereference the directory base
			dirBase = (EFI_PHYSICAL_ADDRESS*)(curProc + ctx->offsets.dirBase);

			LOG_DBG("[NT] dirBase: %p at %p\r\n", *dirBase, curProc + ctx->offsets.dirBase);
		}

		// Now get the process id frm the EPROCESS struct
		EFI_PHYSICAL_ADDRESS* pid = 0;

		// Check if the address with the offset is valid
		if (IsAddressValid(curProc + ctx->offsets.apl - 8) == TRUE)
		{
			// Directly dereference the process id
			pid = (EFI_PHYSICAL_ADDRESS*)(curProc + ctx->offsets.apl - 8);

			LOG_DBG("[NT] pid: %d at %p\r\n", *pid, curProc + ctx->offsets.apl - 8);
		}

		// Check if we either got a stack count or the process id is 4 (System process)
		if (*stackCount || *pid == 4)
		{
			// Increase the size to indicate the we have found something
			size++;

			// Now read the name of this process
			char* name;

			// Check if the address with the offset is valid 
			if (IsAddressValid(curProc + ctx->offsets.imageFileName) == TRUE)
			{
				// Directly dereference the process name
				name = (char*)(curProc + ctx->offsets.imageFileName);

				LOG_DBG("[NT] scanning process: %s\r\n", name);

				// Check if this is the system process by comparing the process name to the system string
				if (!strcmp(name, "System"))
				{
					// Indicate the we found the system process -> no reason to reinitialize
					foundSystemProcess = TRUE;
				}

				// Check if it's the process requested as parameter
				if (!strcmp(name, processname))
				{
					LOG_INFO("[NT] found process %s\r\n", processname);

					// Return the directory base to indicate we found it
					return *dirBase;
				}
			}
		}

		// get the next process from the list
		EFI_VIRTUAL_ADDRESS* tempVirt;

		// Check if the address with the offset is valid 
		if (IsAddressValid(curProc + ctx->offsets.apl) == TRUE)
		{
			// Directly dereference the virtual address of the next process
			tempVirt = (EFI_VIRTUAL_ADDRESS*)(curProc + ctx->offsets.apl);

			virtProcess = *tempVirt;
		}
		else
		{
			// Not valid, set virtual address to 0
			virtProcess = 0;
		}

		LOG_VERB("[NT] virtProcess: %p\r\n", virtProcess);

		// Check if we got a valid virtual address
		if (!virtProcess)
			break;

		// Calculate the correct virtual address
		virtProcess = virtProcess - ctx->offsets.apl;

		// Calculate the physical address using the dirbase we got
		curProc = VTOP(virtProcess, *dirBase);

		LOG_VERB("[NT] curProc: %x\r\n", curProc);

		// Check if we got a valid physical address
		if (!curProc)
			break;
	}

	// If the windows struct is trashed during bootup and we are unable to find the system process, re-init the windows structs
	if (!foundSystemProcess)
		InitGlobalWindowsContext();

	// Return 0 if we get here
	return 0;
}

/*
* Function:  dumpSingleProcess
* --------------------
*  This function takes the initialized windows structs and searches in the process list for the process names and writes the data into the passed process struct.
*
*  ctx:				Pointer to the WinCtx struct that contains offsets / pointers to the important NT kernel structs.
*  processname:		String of the process name that should be found
*  process:			Pointer to a WinProc struct that will be initialized with the process data
*
*  returns:	Returns false if failed or process not found, returns true if it worked
*
*/
BOOLEAN dumpSingleProcess(const WinCtx* ctx, const char* processname, WinProc* process)
{
	// Set the temporary variables to the initial process (first process)
	EFI_PHYSICAL_ADDRESS curProc = ctx->initialProcess.physProcess;
	EFI_VIRTUAL_ADDRESS virtProcess = ctx->initialProcess.process;

	LOG_INFO("[NT] curProc: %x, virtProcess: %x\r\n", curProc, virtProcess);

	// Now go through the process linked list until we found something and the next process is not the system process
	UINT32 size = 0;
	while (!size || curProc != ctx->initialProcess.physProcess)
	{
		// First get the stackcount of the EPROCESS struct
		EFI_PHYSICAL_ADDRESS* stackCount = 0;

		// Check if the address with the offset is valid
		if (IsAddressValid(curProc + ctx->offsets.stackCount) == TRUE)
		{
			// Directly dereference the stack count
			stackCount = (EFI_PHYSICAL_ADDRESS*)(curProc + ctx->offsets.stackCount);

			LOG_DBG("[NT] stackCount: %d at %p\r\n", *stackCount, curProc + ctx->offsets.stackCount);
		}

		// Now get the directory base from the EPROCESS struct
		EFI_PHYSICAL_ADDRESS* dirBase = 0;

		// Check if the address with the offset is valid
		if (IsAddressValid(curProc + ctx->offsets.dirBase) == TRUE)
		{
			// Directly dereference the directory base
			dirBase = (EFI_PHYSICAL_ADDRESS*)(curProc + ctx->offsets.dirBase);

			LOG_DBG("[NT] dirBase: %p at %p\r\n", *dirBase, curProc + ctx->offsets.dirBase);
		}

		// Now get the process id frm the EPROCESS struct
		EFI_PHYSICAL_ADDRESS* pid = 0;

		// Check if the address with the offset is valid
		if (IsAddressValid(curProc + ctx->offsets.apl - 8) == TRUE)
		{
			// Directly dereference the process id
			pid = (EFI_PHYSICAL_ADDRESS*)(curProc + ctx->offsets.apl - 8);

			LOG_DBG("[NT] pid: %d at %p\r\n", *pid, curProc + ctx->offsets.apl - 8);
		}

		// Check if we either got a stack count or the process id is 4 (System process)
		if (*stackCount || *pid == 4)
		{
			// Increase the size to indicate the we have found something
			size++;

			// Now read the name of this process
			char* name;

			// Check if the address with the offset is valid 
			if (IsAddressValid(curProc + ctx->offsets.imageFileName) == TRUE)
			{
				// Directly dereference the process name
				name = (char*)(curProc + ctx->offsets.imageFileName);

				// Check if it's the process requested as parameter
				if (!strcmp(name, processname))
				{
					// Now get the virtual size of the process
					EFI_PHYSICAL_ADDRESS* VirtualSize = 0;

					// Check if the address with the offset is valid
					if (IsAddressValid(curProc + ctx->offsets.virtualSize) == TRUE)
					{
						// Directly dereference the virtual size
						VirtualSize = (EFI_PHYSICAL_ADDRESS*)(curProc + ctx->offsets.virtualSize);
					}

					// Now get the VAD root of the process
					EFI_PHYSICAL_ADDRESS* VadROOT = 0;

					// Check if the address with the offset is valid
					if (IsAddressValid(curProc + ctx->offsets.vadroot) == TRUE)
					{
						// Directly dereference the VAD root
						VadROOT = (EFI_PHYSICAL_ADDRESS*)(curProc + ctx->offsets.vadroot);
					}

					// We successfully found our process, write the data into the process struct that was passed
					process->dirBase = *dirBase;
					process->process = virtProcess;
					process->physProcess = curProc;
					process->pid = *pid;
					process->size = *VirtualSize;
					process->vadRoot = *VadROOT;

					// Return true for successful execution
					return TRUE;
				}
			}
		}

		// get the next process from the list
		EFI_VIRTUAL_ADDRESS* tempVirt;

		// Check if the address with the offset is valid 
		if (IsAddressValid(curProc + ctx->offsets.apl) == TRUE)
		{
			// Directly dereference the virtual address of the next process
			tempVirt = (EFI_VIRTUAL_ADDRESS*)(curProc + ctx->offsets.apl);

			virtProcess = *tempVirt;
		}
		else
		{
			// Not valid, set virtual address to 0
			virtProcess = 0;
		}

		LOG_VERB("[NT] virtProcess: %p\r\n", virtProcess);

		// Check if we got a valid virtual address
		if (!virtProcess)
			break;

		// Calculate the correct virtual address
		virtProcess = virtProcess - ctx->offsets.apl;

		// Calculate the physical address using the dirbase we got
		curProc = VTOP(virtProcess, *dirBase);

		LOG_VERB("[NT] curProc: %p\r\n", curProc);

		// Check if we got a valid physical address
		if (!curProc)
			break;
	}

	// Nothing found, thus return false
	return FALSE;
}

/*
* Function:  GetPeb
* --------------------
*  This function dumps the 64 bit Process Environment Block (PEB) of the given process and returns it as a struct.
*
*  ctx:				Pointer to the WinCtx struct that contains offsets / pointers to the important NT kernel structs.
*  process:			Pointer to a WinProc struct that contains pointers of the process where the PEB should be retrieved
*
*  returns:	Directly returns the PEB struct that was read, NULL if failed
*
*/
PEB GetPeb(const WinCtx* ctx, const WinProc* process)
{
	LOG_VERB("[NT] Reading PEB.. \r\n");

	// Initialize the struct
	PEB peb = {0, 0, 0, 0, {0, 0, 0}, 0, 0, 0};

	// Initialize the pointer to the PEB that will be read at the offset
	EFI_VIRTUAL_ADDRESS ppeb = 0;

	// Check if we got a valid physical address of the process
	if (process->physProcess == 0)
	{
		LOG_ERROR("[NT] EPROCESS 0\r\n");

		// Physical address of process not set, abort
		return peb;
	}

	// Check if the PEB offset was set
	if (ctx->offsets.peb == 0)
	{
		LOG_ERROR("[NT] PEB Offset 0\r\n");

		// PEB Offset not set, abort
		return peb;
	}

	// Now read the pointer to the PEB from the EPROCESS with the offset
	pMemCpyForce((EFI_PHYSICAL_ADDRESS)&ppeb, process->physProcess + ctx->offsets.peb, sizeof(EFI_VIRTUAL_ADDRESS));

	// Check if we got a valid pointer to the PEB
	if (ppeb == 0)
	{
		LOG_ERROR("[NT] PPEB 0\r\n");

		// PEB Pointer wrong, most likely offset outdated, abort
		return peb;
	}

	// Now directly read the PEB into our local struct
	BOOLEAN status = vMemReadForce((EFI_PHYSICAL_ADDRESS)&peb, ppeb, sizeof(PEB), process->dirBase);

	// Check if the read worked
	if (status == FALSE)
	{
		LOG_ERROR("[NT] Failed PEB read\r\n");

		// Read failed, not a valid pointer due to most likely outdated offset, abort

		// Set Ldr to 0 so the execution will not continue later
		peb.Ldr = 0;

		return peb;
	}

	// If we get here we have successfully read the PEB, return the struct now
	return peb;
}

/*
* Function:  GetPeb32
* --------------------
*  This function dumps the 32 bit Process Environment Block (PEB) of the given process and returns it as a struct.
*
*  ctx:				Pointer to the WinCtx struct that contains offsets / pointers to the important NT kernel structs.
*  process:			Pointer to a WinProc struct that contains pointers of the process where the PEB should be retrieved
*
*  returns:	Directly returns the PEB32 struct that was read, NULL if failed
*
*/
PEB32 GetPeb32(const WinCtx* ctx, const WinProc* process)
{
	LOG_VERB("[NT] Reading PEB32.. \r\n");

	// Initialize the struct
	PEB32 peb = {0, 0, 0, 0, 0, 0, 0};

	// Initialize the pointer to the PEB that will be read at the offset
	EFI_VIRTUAL_ADDRESS ppeb = 0;

	// Check if we got a valid physical address of the process
	if (process->physProcess == 0)
	{
		LOG_ERROR("[NT] EPROCESS 0\r\n");

		// Physical address of process not set, abort
		return peb;
	}

	// Check if the PEB offset was set
	if (ctx->offsets.peb == 0)
	{
		LOG_ERROR("[NT] PEB Offset 0\r\n");

		// PEB Offset not set, abort
		return peb;
	}

	// Now read the pointer to the PEB32 from the EPROCESS with the offset
	pMemCpyForce((EFI_PHYSICAL_ADDRESS)&ppeb, process->physProcess + ctx->offsets.peb, sizeof(EFI_VIRTUAL_ADDRESS));

	// Check if we got a valid pointer to the PEB
	if (ppeb == 0)
	{
		LOG_ERROR("[NT] PPEB 0\r\n");

		// PEB Pointer wrong, most likely offset outdated, abort
		return peb;
	}

	// Now directly read the PEB32 into our local struct
	BOOLEAN status = vMemReadForce((EFI_PHYSICAL_ADDRESS)&peb, ppeb + 0x1000, sizeof(PEB32), process->dirBase);

	// Check if the read worked
	if (status == FALSE)
	{
		LOG_ERROR("[NT] Failed PEB read\r\n");

		// Read failed, not a valid pointer due to most likely outdated offset, abort

		// Set Ldr to 0 so the execution will not continue later
		peb.Ldr = 0;

		return peb;
	}

	// If we get here we have successfully read the PEB32, return the struct now
	return peb;
}

/*
* Function:  fillModuleList64
* --------------------
*  This function dumps the 64 bit module list of the given process and returns specific information about one specified module.
*
*  ctx:				Pointer to the WinCtx struct that contains offsets / pointers to the important NT kernel structs.
*  Process:			Pointer to a WinProc struct that contains pointers of the process where the PEB should be retrieved
*  Module:			Pointer to a WinModule struct that contains pointers and data of the module that was requested
*  x86:				BOOLEAN* pointer that indicates if this process is 32 bit or not
*  moduleList:		UINT64 (EFI_PHYSICAL_ADDRESS) pointer to a list where all module names should be saved
*  moduleListCount:	UINT8* pointer to the counter of the list to know where to add an entry
*
*  returns:	Returns false if it fails, otherwise returns true
*
*/
BOOLEAN fillModuleList64(const WinCtx* ctx, const WinProc* Process, WinModule* Module, BOOLEAN* x86, EFI_PHYSICAL_ADDRESS moduleList, UINT8* moduleListCount)
{
	// Check if the passed process has all the pointers we need
	if (Process->dirBase == 0 || Process->physProcess == 0 || Process->process == 0)
	{
		LOG_ERROR("[NT] Process not setup correctly \r\n");

		// We dont have all pointers to successfully complete this function, abort execution already
		return FALSE;
	}

	// Save the directory base into a local variable, this seems to be more stable
	EFI_PHYSICAL_ADDRESS dirBase = Process->dirBase;

	// First get the PEB of the process
	PEB peb = GetPeb(ctx, Process);

	// Additional safety check, check if the Loader pointer of the PEB is actually set
	if (peb.Ldr == 0)
	{
		LOG_ERROR("[NT] PEB Ldr 0 \r\n");

		// PEB Ldr is 0 which can not be true for a running process, abort
		return FALSE;
	}

	// Create a pointer variable to the PEB_LDR_DATA struct
	PEB_LDR_DATA* ldr;

	// Calculate the physical address of the PEB Loader
	EFI_PHYSICAL_ADDRESS physLdr = VTOP(peb.Ldr, dirBase);

	// Check if we got a valid address
	if (IsAddressValid(physLdr) == FALSE)
	{
		LOG_ERROR("[NT] Ldr Ptr 0 \r\n");

		// Physical address of PEB Loader is 0, abort
		return FALSE;
	}

	LOG_INFO("[NT] Phys Ldr at: %p\r\n", physLdr);

	// Now change the pointer to the physical address of the real PEB Loader struct so we can directly access it
	ldr = (PEB_LDR_DATA*)physLdr;

	LOG_INFO("[NT] Head Flink: %p\r\n", ldr->InMemoryOrderModuleList.f_link);

	// Set the head of the linked list
	UINT64 head = ldr->InMemoryOrderModuleList.f_link;

	// End is also the head in a linked list
	UINT64 end = head;

	// Initialize the prev variable with a different value than head so the loop works
	UINT64 prev = head + 1;

	// Now loop through the linked list which contains all modules
	// The loop stops when we arrive at the end
	do
	{
		// Set the current entry as previous entry as we will get the next entry shortly
		prev = head;

		// Prepare a buffer which will hold the LDR_MODULE containing information about the module
		unsigned char modBuffer[sizeof(LDR_MODULE)];

		// Point a LDR_MODULE struct to the buffer we allocated
		LDR_MODULE* mod = (LDR_MODULE*)modBuffer;

		// Now read the LDR_MODULE by reading the memory infront of the LIST_ENTRY
		vMemReadForce((EFI_PHYSICAL_ADDRESS)mod, head - sizeof(LIST_ENTRY), sizeof(LDR_MODULE), dirBase);

		// Now get the address of the next entry by reading the current position as a pointer
		vMemReadForce((EFI_PHYSICAL_ADDRESS)&head, head, sizeof(head), dirBase);

		LOG_VERB("[NT] Next entry: %p\r\n", head);

		// Check if this entry is valid by checking if the module has a valid name (String buffer is located out of struct)
		if (mod->BaseDllName.buffer == 0)
		{
			LOG_VERB("[NT] Empty name\r\n");

			// Doesnt mean the list is wrong, just that this entry was empty
			// Just skip this one and try the next one
			continue;
		}

		// The BaseDllName is a WCHAR, thus we have to convert it first

		// Allocate a 64 byte buffer which should be enough for 99% of the module names
		char wBuffer[0x40];

		// Copy the WCHAR buffer into our buffer
		vMemReadForce((EFI_PHYSICAL_ADDRESS)wBuffer, mod->BaseDllName.buffer, 0x40, dirBase);

		// Now allocate a buffer half the size for the conversion
		char cBuffer[0x20];

		// Go through the WCHAR and copy every second byte into the CHAR Buffer
		for (int i = 0; i < 0x1f; i++)
			cBuffer[i] = ((char*)wBuffer)[i * 2];

		// At the end, put the null terminator
		cBuffer[0x20 - 1] = '\0';

		LOG_VERB("[NT] Name of module: %s\r\n", cBuffer);

		// Check if the module name is 'wow64.dll', this is a trick to know if it is a 32bit process and properly support it
		if (!strcmp("wow64.dll", cBuffer))
			*x86 = 1;

		// Module list, check if it was passed and we should add values to it
		if (moduleList != 0 && moduleListCount != 0)
		{
			// Some values passed, check if we already have more than 127 modules added
			if (*moduleListCount < 127)
			{
				// We still have room in the list

				// Add the current module name to the list
				pMemCpyForce(moduleList + (*moduleListCount * 0x20), (EFI_PHYSICAL_ADDRESS)cBuffer, 0x20);

				// Increase the list by one entry
				*moduleListCount += 1;
			}
		}

		// Check if this is the module that was requested
		if (!strcmp(Module->name, cBuffer))
		{
			// Found the module that was requested
			LOG_INFO("[NT] 64 Found module \r\n");

			// Set the values in the module struct that was passed
			Module->baseAddress = mod->BaseAddress;
			Module->sizeOfModule = mod->SizeOfImage;
			Module->entryPoint = mod->EntryPoint;

			// Return true to indicate successful execution
			return TRUE;
		}
	} while (head != end && head != prev);

	// Return false to indicate failed execution
	return FALSE;
}

/*
* Function:  fillModuleList86
* --------------------
*  This function dumps the 32 bit module list of the given process and returns specific information about one specified module.
*
*  ctx:				Pointer to the WinCtx struct that contains offsets / pointers to the important NT kernel structs.
*  Process:			Pointer to a WinProc struct that contains pointers of the process where the PEB should be retrieved
*  Module:			Pointer to a WinModule struct that contains pointers and data of the module that was requested
*  moduleList:		UINT64 (EFI_PHYSICAL_ADDRESS) pointer to a list where all module names should be saved
*  moduleListCount:	UINT8* pointer to the counter of the list to know where to add an entry
*
*  returns:	Returns false if it fails, otherwise returns true
*
*
* TODO: REMOVE THE HARDCODED STEAM-RELATED STUFF HERE!!!
*
*/
BOOLEAN fillModuleList86(const WinCtx* ctx, const WinProc* Process, WinModule* Module, EFI_PHYSICAL_ADDRESS moduleList, UINT8* moduleListCount)
{
	// Check if the passed process has all the pointers we need
	if (Process->dirBase == 0 || Process->physProcess == 0 || Process->process == 0)
	{
		LOG_ERROR("[NT] Process not setup correctly \r\n");

		// We dont have all pointers to successfully complete this function, abort execution already
		return FALSE;
	}

	// Save the directory base into a local variable, this seems to be more stable
	EFI_PHYSICAL_ADDRESS dirBase = Process->dirBase;

	// First get the PEB32 of the process
	PEB32 peb = GetPeb32(ctx, Process);

	// Additional safety check, check if the Loader pointer of the PEB is actually set
	if (peb.Ldr == 0)
	{
		LOG_ERROR("[NT] PEB Ldr 0 \r\n");

		// PEB Ldr is 0 which can not be true for a running process, abort
		return FALSE;
	}

	// Create a pointer variable to the PEB_LDR_DATA struct
	PEB_LDR_DATA32* ldr;

	// Calculate the physical address of the PEB Loader
	EFI_PHYSICAL_ADDRESS physLdr = VTOP(peb.Ldr, dirBase);

	// Check if we got a valid address
	if (IsAddressValid(physLdr) == FALSE)
	{
		LOG_ERROR("[NT] Ldr Ptr 0 \r\n");

		// Physical address of PEB Loader is invalid, abort
		return FALSE;
	}

	LOG_INFO("[NT] Phys Ldr at: %p\r\n", physLdr);

	// Now change the pointer to the physical address of the real PEB Loader struct so we can directly access it
	ldr = (PEB_LDR_DATA32*)physLdr;

	LOG_INFO("[NT] Head Flink: %p\r\n", ldr->InMemoryOrderModuleList.f_link);

	// Set the head of the linked list
	UINT32 head = ldr->InMemoryOrderModuleList.f_link;

	// End is also the head in a linked list
	UINT32 end = head;

	// Initialize the prev variable with a different value than head so the loop works
	UINT32 prev = head + 1;

	// Now loop through the linked list which contains all modules
	// The loop stops when we arrive at the end
	do
	{
		// Set the current entry as previous entry as we will get the next entry shortly
		prev = head;

		// Prepare a buffer which will hold the LDR_MODULE32 containing information about the module
		unsigned char modBuffer[sizeof(LDR_MODULE32)];

		// Point a LDR_MODULE32 struct to the buffer we allocated
		LDR_MODULE32* mod = (LDR_MODULE32*)modBuffer;

		// Now read the LDR_MODULE by reading the memory infront of the LIST_ENTRY32
		vMemReadForce((EFI_PHYSICAL_ADDRESS)mod, head - sizeof(LIST_ENTRY32), sizeof(LDR_MODULE32), dirBase);

		// Now get the address of the next entry by reading the current position as a pointer
		vMemReadForce((EFI_PHYSICAL_ADDRESS)&head, head, sizeof(head), dirBase);

		// Check if this entry is valid by checking if the module has a valid name (String buffer is located out of struct)
		if (mod->BaseDllName.buffer == 0)
		{
			LOG_INFO("[NT] Empty name\r\n");

			// Doesnt mean the list is wrong, just that this entry was empty
			// Just skip this one and try the next one
			continue;
		}

		// The BaseDllName is a WCHAR, thus we have to convert it first

		// Allocate a 64 byte buffer which should be enough for 99% of the module names
		unsigned char wBuffer[0x40];

		// Copy the WCHAR buffer into our buffer
		vMemReadForce((EFI_PHYSICAL_ADDRESS)wBuffer, mod->BaseDllName.buffer, 0x40, dirBase);

		// Now allocate a buffer half the size for the conversion
		char cBuffer[0x20];

		// Go through the WCHAR and copy every second byte into the CHAR Buffer
		for (int i = 0; i < 0x1f; i++)
			cBuffer[i] = ((char*)wBuffer)[i * 2];

		// At the end, put the null terminator
		cBuffer[0x20 - 1] = '\0';

		LOG_VERB("[NT] ModName: %s\r\n", cBuffer);

		// Module list, check if it was passed and we should add values to it
		if (moduleList != 0 && moduleListCount != 0)
		{
			// Some values passed, check if we already have more than 127 modules added
			if (*moduleListCount < 127)
			{
				// We still have room in the list

				// Add the current module name to the list
				pMemCpyForce(moduleList + (*moduleListCount * 32), (EFI_PHYSICAL_ADDRESS)cBuffer, 32);

				// Increase the list by one entry
				*moduleListCount += 1;
			}
		}

		// Check if this is the module that was requested and not one of the excluded modules
		if (!strcmp(Module->name, cBuffer))
		{
			// Found the module that was requested
			LOG_INFO("[NT] 32 Found module \r\n");

			// Set the values in the module struct that was passed
			Module->baseAddress = mod->BaseAddress;
			Module->sizeOfModule = mod->SizeOfImage;
			Module->entryPoint = mod->EntryPoint;

			// Return true to indicate successful execution
			return TRUE;
		}
	} while (head != end && head != prev);

	// Return false to indicate failed execution
	return FALSE;
}

/*
* Function:  dumpSingleModule
* --------------------
*  Goes through the loaded modules in a process and returns the details (base address, size) of the specifc module name
*
*  ctx:				Pointer to the WinCtx struct that contains offsets / pointers to the important NT kernel structs.
*  Process:			Pointer to a WinProc struct that contains pointers of the process where the PEB should be retrieved
*  Module:			Pointer to a WinModule struct that contains pointers and data of the module that was requested
*
*  returns: Returns false if it failed to find the module, otherwise true
*
*/
BOOLEAN dumpSingleModule(const WinCtx* ctx, const WinProc* process, WinModule* out_module)
{
	// Status variable to keep track of the result
	BOOLEAN status = FALSE;

	// Indicates if the process is 32 bit or 64 bit
	BOOLEAN x86 = FALSE;

	// First get the 64 bit module list of this process
	status = fillModuleList64(ctx, process, out_module, &x86, 0, 0);

	// Check if the status failed
	if (status == FALSE && x86 == FALSE)
	{
		LOG_ERROR("[NT] Could not find the module from a 64-bit process!\r\n");

		// If it failed, we abort it and return false
		return FALSE;
	}

	if (x86)
	{
		LOG_VERB("[NT] The process seems to be x86 ...\r\n");
		status = fillModuleList86(ctx, process, out_module, 0, 0);
	}

	// Check if the status has changed (doesnt change if this is not a 32 bit process)
	if (status == FALSE)
	{
		LOG_ERROR("[NT] x86 failed dump\r\n");

		// This is a 32 bit process and we failed to dump the 32 bit module list, abort...
		return FALSE;
	}

	return status;
}

/*
* Function:  dumpModuleNames
* --------------------
*  Goes through the loaded modules in a process and writes all module names in the passed pointer
*
*  ctx:				Pointer to the WinCtx struct that contains offsets / pointers to the important NT kernel structs.
*  Process:			Pointer to a WinProc struct that contains pointers of the process where the PEB should be retrieved
*  Module:			Pointer to a WinModule struct that contains pointers and data of the module that was requested
*  moduleList		UINT64 (EFI_PHYSICAL_ADDRESS) to a memory page where the result of all module names should be written to
*  moduleListCount	Pointer to an UINT8 variable that will contain the amount of module names that were written to the page
*
*  returns: Returns false if it failed to dump, otherwise true
*
*/
BOOLEAN dumpModuleNames(WinCtx* ctx, WinProc* Process, WinModule* Module, EFI_PHYSICAL_ADDRESS moduleList, UINT8* moduleListCount)
{
	// Status variable to keep track of the result
	BOOLEAN status = FALSE;

	// Indicates if the process is 32 bit or 64 bit
	BOOLEAN x86 = FALSE;

	// First get the 64 bit module list of this process
	status = fillModuleList64(ctx, Process, Module, &x86, moduleList, moduleListCount);

	// Check if the status failed
	if (status == FALSE && x86 == FALSE && moduleList == 0)
	{
		LOG_ERROR("[NT] Could not find the module from a 64-bit process!\r\n");

		// If it failed, we abort it and return false
		return FALSE;
	}

	// If we found a module with the name 'wow64.dll', we know its a 32 bit process and thus contains 32 bit modules
	// In that case, dump the 32 bit module list
	if (x86)
	{
		LOG_VERB("[NT] The process seems to be x86 ...\r\n");
		status = fillModuleList86(ctx, Process, Module, moduleList, moduleListCount);
	}

	// Check if the status has changed (doesnt change if this is not a 32 bit process)
	if (status == FALSE && moduleList == 0)
	{
		LOG_ERROR("[NT] x86 failed dump\r\n");

		// This is a 32 bit process and we failed to dump the 32 bit module list, abort...
		return FALSE;
	}

	// If we get here, we know that everything worked, thus return true
	return TRUE;
}

/*
* Function:  InitGlobalWindowsContext
* --------------------
*  Initializes the NT Kernel variables by searching for various structures and analyzing the memory.
*
*  returns:	FALSE if initialization failed, true if it worked
*
*/
BOOLEAN InitGlobalWindowsContext()
{
	LOG_INFO("[NT] Starting initializing windows context\r\n");

	// Variable to track the status
	BOOLEAN status = TRUE;

	// Search for the PML4 and kernelEntry in the low stub
	EFI_PHYSICAL_ADDRESS PML4 = 0;
	EFI_VIRTUAL_ADDRESS kernelEntry = 0;
	status = getPML4(&PML4, &kernelEntry);

	// Check if we successfully found it
	if (status == TRUE)
	{
		LOG_INFO("[NT] PML4: %p, KernelEntry: %p\r\n", PML4, kernelEntry);

		// Set the PML4 to the directory base of the initial process (NT kernel)
		winGlobal->initialProcess.dirBase = PML4;
	}
	else
	{
		LOG_ERROR("[NT] Failed finding KernelEntry\r\n");

		// Failed finding PML4 & kernelentry, abort
		return FALSE;
	}

	// Search ntoskrnl starting from the kernel entry using the PML4 we found
	EFI_VIRTUAL_ADDRESS ntosKrnl = 0;
	status = findNtosKrnl(kernelEntry, PML4, &ntosKrnl);

	// Check if we successfully found it
	if (status == TRUE)
	{
		LOG_INFO("[NT] NT Kernel: %p\r\n", ntosKrnl);
	}
	else
	{
		LOG_ERROR("[NT] Failed locating NT Kernel\r\n");

		// Failed finding NT Kernel, abort
		return FALSE;
	}

	// Generate ntoskrnl export list and return the address of the export 'PsInitialSystemProcess'
	EFI_VIRTUAL_ADDRESS PsInitialSystemProcess = getExportList(ntosKrnl, PML4, "PsInitialSystemProcess");

	// Check if we successfully found the export 'PsInitialSystemProcess'
	if (PsInitialSystemProcess != 0)
	{
		LOG_INFO("[NT] PsInitialSystemProcess: %p\r\n", PsInitialSystemProcess);
	}
	else
	{
		LOG_ERROR("[NT] Failed finding PsInitialSystemProcess\r\n");

		// Failed locating the export, abort the execution
		return FALSE;
	}

	// Now get the pointer to the EPROCESS of the System process by reading the first 8 bytes
	EFI_VIRTUAL_ADDRESS systemProcess = 0;
	vMemRead((EFI_PHYSICAL_ADDRESS)&systemProcess, PsInitialSystemProcess, sizeof(EFI_VIRTUAL_ADDRESS), PML4);

	if (systemProcess != 0)
	{
		LOG_INFO("[NT] SystemProcess: %p\r\n", systemProcess);
	}
	else
	{
		LOG_ERROR("[NT] Failed getting SystemProcess!\r\n");

		// Failed getting address to EPROCESS, abort the execution
		return FALSE;
	}

	// Set the variables in the winGlobal so we only have to do this once (System pointers dont change in runtime)
	winGlobal->initialProcess.process = systemProcess;
	winGlobal->initialProcess.physProcess = VTOP(systemProcess, PML4); // Set the physical address directly

	// Now get the NT Version of the kernel
	UINT16 ntVersion = getNTVersion(ntosKrnl, PML4);

	// Check if we got a valid version
	if (ntVersion != 0)
	{
		LOG_INFO("[NT] NtVersion: %d\r\n", ntVersion);
	}
	else
	{
		LOG_ERROR("[NT] Failed finding NT version!\r\n");

		// Failed getting the NT version, abort the execution
		return FALSE;
	}

	// Set the variable in the winGlobal as we use it for later functions
	winGlobal->ntVersion = ntVersion;

	// Now get the NT Build of the kernel
	UINT16 ntBuild = getNTBuild(ntosKrnl, PML4);

	// Check if we got a valid build
	if (ntBuild != 0)
	{
		LOG_INFO("[NT] NtBuild: %d\r\n", ntBuild);
	}
	else
	{
		LOG_ERROR("[NT] Failed finding NT build!\r\n");

		// Failed getting the NT build, abort the execution
		return FALSE;
	}

	// Set the variable in the winGlobal as we use it for later functions
	winGlobal->ntBuild = ntBuild;

	// Now set the offset set for the NT kernel functions according to the version & build we found
	status = setupOffsets(ntVersion, ntBuild, &winGlobal->offsets);

	// Check if we successfully setup offsets
	if (status == FALSE)
	{
		LOG_ERROR("[NT] Failed setting Windows offsets!\r\n");

		// Failed setting up offsets, probably unsupported version thus abort execution
		return FALSE;
	}

	// Finished initializing the NT kernel structures!
	LOG_INFO("[NT] Windows OS initialization ready!\r\n");

	// Return true as everything worked
	return TRUE;
}