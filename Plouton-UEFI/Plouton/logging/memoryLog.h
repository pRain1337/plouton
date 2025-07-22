#ifndef __memory_log_h__
#define __memory_log_h__

#include <Uefi/UefiBaseType.h>
#include <Library/UefiBootServicesTableLib.h>

// The size of our memory log buffer in bytes. 1MB.
#define MEM_LOG_BUFFER_SIZE (1024 * 1024)

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

#endif // __memory_log_h__ 