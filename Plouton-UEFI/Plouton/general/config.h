/*++
*
* Contains general definitions used in Plouton
*
--*/

#include "../logging/log_levels.h"

#ifndef __plouton_config_h__
#define __plouton_config_h__

/*
 * Serial port configuration.
 * For EFI_DEBUG_SERIAL_BUILTIN and EFI_DEBUG_SERIAL_PROTOCOL.
 * Port 0 is the default port on the motherboard
 */
#define SERIAL_BAUDRATE 115200
#define SERIAL_PORT_0 0x3F8

 /*
 * This variable is used to indicate if and what serial level debug should be used.
 * See logging.h for available options
 */
#define SERIAL_DEBUG_LEVEL LOG_LEVEL_INFO

/*
 * Memory logging configuration.
 * Set to 1 to enable in-memory logging for debugging purposes.
 * Set to 0 to disable in-memory logging.
 * When enabled, error messages will be stored in a memory buffer that can be
 * inspected with external tools.
 */
#define ENABLE_MEMORY_LOG 0

/*
 * Memory log buffer size configuration.
 * Available options:
 * - MEM_LOG_SIZE_1MB   (1MB - 1024 * 1024 bytes)
 * - MEM_LOG_SIZE_2MB   (2MB - 2 * 1024 * 1024 bytes)
 * - MEM_LOG_SIZE_4MB   (4MB - 4 * 1024 * 1024 bytes)
 * - MEM_LOG_SIZE_8MB   (8MB - 8 * 1024 * 1024 bytes)
 *
 * Larger buffers provide more log history but consume more memory.
 * Choose based on your debugging needs and available system memory.
 */
#define MEMORY_LOG_BUFFER_SIZE MEM_LOG_SIZE_8MB

/*
 * Minimum log level that will be written to the in-memory log buffer.
 * Use LOG_LEVEL_ERROR / LOG_LEVEL_INFO / LOG_LEVEL_VERB / LOG_LEVEL_DBG.
 * This is independent from SERIAL_DEBUG_LEVEL to allow keeping serial chatty
 * while trimming what gets persisted in memory.
 */
#define MEMORY_LOG_MIN_LEVEL LOG_LEVEL_INFO

// Cheat features
#define ENABLE_SOUND FALSE
#define ENABLE_AIM TRUE

// DRIVERS
#define MOUSE_DRIVER_LOGITECH_G_PRO
#define MOUSE_DRIVER_LOGITECH_G_PRO_SUPERLIGHT

#define AUDIO_DRIVER_LOGITECH_G_PRO_X_2_WIRELESS
#define AUDIO_DRIVER_LOGITECH_G_PRO_X_WIRELESS

#endif
