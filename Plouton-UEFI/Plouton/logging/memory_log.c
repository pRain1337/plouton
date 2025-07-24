#include "memory_log.h"

#if ENABLE_MEMORY_LOG

#include "../hardware/serial.h"
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>

// Global variables to hold the state of our memory logger
EFI_PHYSICAL_ADDRESS gMemoryLogBufferAddress = 0;
UINTN gMemoryLogCursor = 0;
UINTN gMemoryLogWrapCount = 0;
UINT32 gMemoryLogTimestamp = 0;

EFI_STATUS EFIAPI InitMemoryLog(IN EFI_BOOT_SERVICES* gBS)
{
    EFI_STATUS Status;

    // Prevent double initialization
    if (gMemoryLogBufferAddress != 0)
    {
        return EFI_SUCCESS;
    }

    // Allocate pages for our log buffer
    // Calculate the number of pages needed, rounding up if necessary
    UINTN PagesNeeded = (MEM_LOG_BUFFER_SIZE + EFI_PAGE_SIZE - 1) / EFI_PAGE_SIZE;

    Status = gBS->AllocatePages(
        AllocateAnyPages,
        EfiRuntimeServicesData, // Use runtime memory so it persists after ExitBootServices if needed
        PagesNeeded,
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
    gMemoryLogWrapCount = 0;
    gMemoryLogTimestamp = 0;

    // IMPORTANT: Log the physical address. The user needs this for their memory inspection tool.
    SerialPrintf("\033[1;33m(I) [MEMLOG] Memory Log initialized. Buffer size: %d bytes at physical address: 0x%llx\n\033[0;39;49m",
                 MEM_LOG_BUFFER_SIZE, gMemoryLogBufferAddress);

    return EFI_SUCCESS;
}

// Internal helper function for writing log entries
STATIC VOID WriteLogEntry(IN UINT8 LogLevel, IN CONST CHAR8* LogData, IN UINTN LogDataLength)
{
    if (gMemoryLogBufferAddress == 0 || LogData == NULL || LogDataLength == 0)
    {
        return;
    }

    // Calculate total entry size (header + data)
    UINTN totalEntrySize = sizeof(MEMORY_LOG_ENTRY_HEADER) + LogDataLength;

    // Check if entry is too large
    if (totalEntrySize > MAX_LOG_ENTRY_SIZE)
    {
        return;
    }

    // Check for wrap-around (circular buffer logic)
    if (gMemoryLogCursor + totalEntrySize >= MEM_LOG_BUFFER_SIZE)
    {
        // Wrap around to the beginning
        gMemoryLogCursor = 0;
        gMemoryLogWrapCount++;
    }

    // Ensure we don't write beyond buffer boundaries
    if (gMemoryLogCursor + totalEntrySize > MEM_LOG_BUFFER_SIZE)
    {
        return; // Entry too large for remaining space
    }

    // Create and write the header
    MEMORY_LOG_ENTRY_HEADER header;
    header.Timestamp = ++gMemoryLogTimestamp;
    header.EntryLength = (UINT16)totalEntrySize;
    header.LogLevel = LogLevel;
    header.Reserved = 0;

    // Write header
    CopyMem((VOID*)(UINTN)(gMemoryLogBufferAddress + gMemoryLogCursor),
            &header, sizeof(MEMORY_LOG_ENTRY_HEADER));
    gMemoryLogCursor += sizeof(MEMORY_LOG_ENTRY_HEADER);

    // Write log data
    CopyMem((VOID*)(UINTN)(gMemoryLogBufferAddress + gMemoryLogCursor),
            LogData, LogDataLength);
    gMemoryLogCursor += LogDataLength;

    // Ensure the end of the buffer is null-terminated for safety when reading
    if (gMemoryLogCursor < MEM_LOG_BUFFER_SIZE)
    {
        *((CHAR8*)(UINTN)(gMemoryLogBufferAddress + gMemoryLogCursor)) = '\0';
    }
}

VOID EFIAPI MemoryLogPrint(IN CONST CHAR8* Format, ...)
{
    if (gMemoryLogBufferAddress == 0 || Format == NULL)
    {
        return;
    }

    VA_LIST Marker;
    CHAR8 logEntryBuffer[MAX_LOG_ENTRY_SIZE - sizeof(MEMORY_LOG_ENTRY_HEADER)];
    UINTN logEntryLength;

    // Format the string
    VA_START(Marker, Format);
    logEntryLength = AsciiVSPrint(logEntryBuffer, sizeof(logEntryBuffer), Format, Marker);
    VA_END(Marker);

    // Sanity check the formatted length
    if (logEntryLength == 0 || logEntryLength >= sizeof(logEntryBuffer))
    {
        return;
    }

    // Use default log level (INFO = 2) for backward compatibility
    WriteLogEntry(2, logEntryBuffer, logEntryLength);
}

VOID EFIAPI MemoryLogPrintEx(IN UINT8 LogLevel, IN CONST CHAR8* Format, ...)
{
    if (gMemoryLogBufferAddress == 0 || Format == NULL)
    {
        return;
    }

    VA_LIST Marker;
    CHAR8 logEntryBuffer[MAX_LOG_ENTRY_SIZE - sizeof(MEMORY_LOG_ENTRY_HEADER)];
    UINTN logEntryLength;

    // Format the string
    VA_START(Marker, Format);
    logEntryLength = AsciiVSPrint(logEntryBuffer, sizeof(logEntryBuffer), Format, Marker);
    VA_END(Marker);

    // Sanity check the formatted length
    if (logEntryLength == 0 || logEntryLength >= sizeof(logEntryBuffer))
    {
        return;
    }

    WriteLogEntry(LogLevel, logEntryBuffer, logEntryLength);
}

VOID EFIAPI GetMemoryLogStats(OUT UINTN* TotalSize, OUT UINTN* UsedSize, OUT UINTN* WrapCount)
{
    if (TotalSize != NULL)
    {
        *TotalSize = MEM_LOG_BUFFER_SIZE;
    }

    if (UsedSize != NULL)
    {
        if (gMemoryLogWrapCount > 0)
        {
            *UsedSize = MEM_LOG_BUFFER_SIZE; // Buffer is full if we've wrapped
        }
        else
        {
            *UsedSize = gMemoryLogCursor;
        }
    }

    if (WrapCount != NULL)
    {
        *WrapCount = gMemoryLogWrapCount;
    }
}

VOID EFIAPI ClearMemoryLog(VOID)
{
    if (gMemoryLogBufferAddress != 0)
    {
        // Clear the buffer
        SetMem((VOID*)(UINTN)gMemoryLogBufferAddress, MEM_LOG_BUFFER_SIZE, 0);

        // Reset counters
        gMemoryLogCursor = 0;
        gMemoryLogWrapCount = 0;
        gMemoryLogTimestamp = 0;

        // Log the clear operation
        SerialPrintf("\033[1;33m(I) [MEMLOG] Memory log buffer cleared\n\033[0;39;49m");
    }
}

#endif // ENABLE_MEMORY_LOG