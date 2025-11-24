#ifndef __plouton_logging_h__
#define __plouton_logging_h__

#include "log_levels.h"
#include "../hardware/serial.h"
#include "../general/config.h"

#if ENABLE_MEMORY_LOG
#include "memory_log.h"
#endif

// color code definition: https://xdevs.com/guide/color_serial/

#if ENABLE_MEMORY_LOG
#define LOG_ERROR(fmt, ...)  do { if (SERIAL_DEBUG_LEVEL >= LOG_LEVEL_ERROR) { SerialPrintf("\033[0;31m(E) " fmt "\033[0;39;49m", ##__VA_ARGS__); MemoryLogPrintEx(1, "(E) " fmt, ##__VA_ARGS__); } } while (0)
#define LOG_INFO(fmt, ...)   do { if (SERIAL_DEBUG_LEVEL >= LOG_LEVEL_INFO)  { SerialPrintf("\033[1;33m(I) " fmt "\033[0;39;49m", ##__VA_ARGS__); MemoryLogPrintEx(2, "(I) " fmt, ##__VA_ARGS__); } } while (0)
#define LOG_VERB(fmt, ...)   do { if (SERIAL_DEBUG_LEVEL >= LOG_LEVEL_VERB)  { SerialPrintf("\033[0;32m(V) " fmt "\033[0;39;49m", ##__VA_ARGS__); MemoryLogPrintEx(3, "(V) " fmt, ##__VA_ARGS__); } } while (0)
#define LOG_DBG(fmt, ...)    do { if (SERIAL_DEBUG_LEVEL >= LOG_LEVEL_DBG)   { SerialPrintf("\033[0;37m(D) " fmt "\033[0;39;49m", ##__VA_ARGS__); MemoryLogPrintEx(4, "(D) " fmt, ##__VA_ARGS__); } } while (0)
#else
#define LOG_ERROR(fmt, ...)  do { if (SERIAL_DEBUG_LEVEL >= LOG_LEVEL_ERROR) SerialPrintf("\033[0;31m(E) " fmt "\033[0;39;49m", ##__VA_ARGS__); } while (0)
#define LOG_INFO(fmt, ...)   do { if (SERIAL_DEBUG_LEVEL >= LOG_LEVEL_INFO)  SerialPrintf("\033[1;33m(I) " fmt "\033[0;39;49m", ##__VA_ARGS__); } while (0)
#define LOG_VERB(fmt, ...)   do { if (SERIAL_DEBUG_LEVEL >= LOG_LEVEL_VERB)  SerialPrintf("\033[0;32m(V) " fmt "\033[0;39;49m", ##__VA_ARGS__); } while (0)
#define LOG_DBG(fmt, ...)    do { if (SERIAL_DEBUG_LEVEL >= LOG_LEVEL_DBG)   SerialPrintf("\033[0;37m(D) " fmt "\033[0;39;49m", ##__VA_ARGS__); } while (0)
#endif

/* non color coded variant

#define LOG_ERROR(fmt, ...)  do { if (SERIAL_DEBUG_LEVEL >= LOG_LEVEL_ERROR)  SerialPrintf("[ERR] " fmt, ##__VA_ARGS__); } while (0)
#define LOG_INFO(fmt, ...)   do { if (SERIAL_DEBUG_LEVEL >= LOG_LEVEL_INFO)   SerialPrintf("[INF] " fmt, ##__VA_ARGS__); } while (0)
#define LOG_VERB(fmt, ...)  do { if (SERIAL_DEBUG_LEVEL >= LOG_LEVEL_VERB)  SerialPrintf("[VRB] " fmt, ##__VA_ARGS__); } while (0)
#define LOG_DBG(fmt, ...)  do { if (SERIAL_DEBUG_LEVEL >= LOG_LEVEL_DBG)  SerialPrintf("[DBG] " fmt, ##__VA_ARGS__); } while (0)

*/

/* To overwrite log levels in a function
*
*	First undefine the current loglevel, then set the log level you want:
* 
*		#undef SERIAL_DEBUG_LEVEL
*		#define SERIAL_DEBUG_LEVEL LOG_LEVEL_INFO
* 
*	When finished, do the same and reset the log level:
*	
*		#undef SERIAL_DEBUG_LEVEL
*		#define SERIAL_DEBUG_LEVEL LOG_LEVEL_ERROR
* 
* */

#endif