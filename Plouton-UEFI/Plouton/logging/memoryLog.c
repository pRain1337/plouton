#include "memoryLog.h"
#include "logging.h"
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>

// Global variables to hold the state of our memory logger
EFI_PHYSICAL_ADDRESS gMemoryLogBufferAddress = 0;
UINTN gMemoryLogCursor = 0;

EFI_STATUS EFIAPI InitMemoryLog(IN EFI_BOOT_SERVICES* gBS)
{
    EFI_STATUS Status;

    // Prevent double initialization
    if (gMemoryLogBufferAddress != 0)
    {
        return EFI_SUCCESS;
    }

    // Allocate pages for our log buffer
    Status = gBS->AllocatePages(
        AllocateAnyPages,
        EfiRuntimeServicesData, // Use runtime memory so it persists after ExitBootServices if needed
        MEM_LOG_BUFFER_SIZE / EFI_PAGE_SIZE,
        &gMemoryLogBufferAddress
    );

    if (EFI_ERROR(Status))
    {
        // We can't use our memory logger yet, so fall back to serial
        SerialPrintf("Failed to allocate memory for log buffer.\n");
        gMemoryLogBufferAddress = 0;
        return Status;
    }

    // Zero out the buffer
    SetMem((VOID*)(UINTN)gMemoryLogBufferAddress, MEM_LOG_BUFFER_SIZE, 0);
    gMemoryLogCursor = 0;

    // IMPORTANT: Log the physical address. The user needs this for their memory inspection tool.
    LOG_INFO("[MEMLOG] Memory Log initialized. Buffer at physical address: 0x%llx\n", gMemoryLogBufferAddress);

    return EFI_SUCCESS;
}

VOID EFIAPI MemoryLogPrint(IN CONST CHAR8* Format, ...)
{
    // If the buffer isn't allocated, we can't do anything
    if (gMemoryLogBufferAddress == 0)
    {
        return;
    }

    VA_LIST Marker;
    CHAR8 logEntryBuffer[512]; // A temporary buffer for the formatted string
    UINTN logEntryLength;

    // Format the string
    VA_START(Marker, Format);
    logEntryLength = AsciiVSPrint(logEntryBuffer, sizeof(logEntryBuffer), Format, Marker);
    VA_END(Marker);

    // Check for wrap-around (circular buffer logic)
    if (gMemoryLogCursor + logEntryLength >= MEM_LOG_BUFFER_SIZE)
    {
        // Wrap around to the beginning
        gMemoryLogCursor = 0;
        // Optionally, we could clear the buffer here, but overwriting is fine
    }

    // Copy the new log entry into our main buffer
    CopyMem((VOID*)(UINTN)(gMemoryLogBufferAddress + gMemoryLogCursor), logEntryBuffer, logEntryLength);

    // Move the write position forward
    gMemoryLogCursor += logEntryLength;
    
    // Ensure the end of the buffer is null-terminated for safety when reading
    if (gMemoryLogCursor < MEM_LOG_BUFFER_SIZE)
    {
        *((CHAR8*)(UINTN)(gMemoryLogBufferAddress + gMemoryLogCursor)) = '\0';
    }
} 