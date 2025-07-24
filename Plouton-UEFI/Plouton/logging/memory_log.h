#ifndef __memory_log_h__
#define __memory_log_h__

#include "../general/config.h"

#if ENABLE_MEMORY_LOG

#include <Uefi/UefiBaseType.h>
#include <Library/UefiBootServicesTableLib.h>

// Memory log buffer size options
#define MEM_LOG_SIZE_1MB   (1024 * 1024)
#define MEM_LOG_SIZE_2MB   (2 * 1024 * 1024)
#define MEM_LOG_SIZE_4MB   (4 * 1024 * 1024)
#define MEM_LOG_SIZE_8MB   (8 * 1024 * 1024)

// Use the configured buffer size from config.h
#define MEM_LOG_BUFFER_SIZE MEMORY_LOG_BUFFER_SIZE

// Memory log entry header for structured logging
typedef struct {
    UINT32 Timestamp;      // Simple timestamp counter
    UINT16 EntryLength;    // Length of this log entry including header
    UINT8  LogLevel;       // Log level (ERROR=1, INFO=2, VERB=3, DBG=4)
    UINT8  Reserved;       // Reserved for alignment
} MEMORY_LOG_ENTRY_HEADER;

// Maximum size for a single log entry (including header)
#define MAX_LOG_ENTRY_SIZE 1024

/**
 * @brief Initializes the memory logging facility.
 * 
 * This function allocates a block of physical memory to be used as a circular
 * buffer for log messages. The physical address of this buffer is printed to
 * the serial log for later inspection with tools like RW-Everything.
 * 
 * @param gBS Pointer to the UEFI Boot Services Table.
 * @return EFI_STATUS Returns EFI_SUCCESS on successful initialization, or an error code on failure.
 */
EFI_STATUS EFIAPI InitMemoryLog(IN EFI_BOOT_SERVICES* gBS);

/**
 * @brief Writes a formatted string to the in-memory log buffer.
 *
 * This function takes a format string and a variable number of arguments,
 * formats them into a string, and writes the result to the circular memory buffer.
 * If the buffer is full, it wraps around to the beginning.
 *
 * @param Format A Null-terminated ASCII format string.
 * @param ... A variable number of arguments to format.
 */
VOID EFIAPI MemoryLogPrint(IN CONST CHAR8* Format, ...);

/**
 * @brief Writes a formatted string with log level to the in-memory log buffer.
 *
 * This enhanced version includes structured logging with timestamps and log levels.
 *
 * @param LogLevel The log level (1=ERROR, 2=INFO, 3=VERB, 4=DBG).
 * @param Format A Null-terminated ASCII format string.
 * @param ... A variable number of arguments to format.
 */
VOID EFIAPI MemoryLogPrintEx(IN UINT8 LogLevel, IN CONST CHAR8* Format, ...);

/**
 * @brief Gets current memory log statistics.
 *
 * @param TotalSize Pointer to receive total buffer size.
 * @param UsedSize Pointer to receive currently used size.
 * @param WrapCount Pointer to receive number of buffer wraps.
 */
VOID EFIAPI GetMemoryLogStats(OUT UINTN* TotalSize, OUT UINTN* UsedSize, OUT UINTN* WrapCount);

/**
 * @brief Clears the memory log buffer.
 */
VOID EFIAPI ClearMemoryLog(VOID);

#else // ENABLE_MEMORY_LOG

// When memory logging is disabled, provide stub functions
#define InitMemoryLog(gBS) EFI_SUCCESS
#define MemoryLogPrint(Format, ...)
#define MemoryLogPrintEx(LogLevel, Format, ...)
#define GetMemoryLogStats(TotalSize, UsedSize, WrapCount)
#define ClearMemoryLog()

#endif // ENABLE_MEMORY_LOG

#endif // __memory_log_h__