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


// Cheat features
#define ENABLE_SOUND FALSE
#define ENABLE_AIM TRUE

// DRIVERS
#define MOUSE_DRIVER_LOGITECH_G_PRO
#define MOUSE_DRIVER_LOGITECH_G_PRO_SUPERLIGHT

#define AUDIO_DRIVER_LOGITECH_G_PRO_X_2_WIRELESS
#define AUDIO_DRIVER_LOGITECH_G_PRO_X_WIRELESS

#endif