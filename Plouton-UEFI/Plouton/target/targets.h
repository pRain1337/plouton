#ifndef __plouton_targets_h__
#define __plouton_targets_h__

#include <Base.h>
#include "cs2/cheatCS2.h"
#include "hermes/hermes.h"


// Define function pointer types
#ifdef __GNUC__
typedef BOOLEAN EFIAPI (*InitCheat)(VOID);
typedef VOID EFIAPI (*CheatLoop)(VOID);
#else
typedef BOOLEAN (*InitCheat)(VOID);
typedef VOID (*CheatLoop)(VOID);
#endif

typedef struct {
    char name[64];
    InitCheat cheatInitFun;
    CheatLoop cheatLoopFun;
    BOOLEAN initialized;
	BOOLEAN isGame;
    EFI_PHYSICAL_ADDRESS dirBase;
} TargetEntry;

TargetEntry targets[] = {
    { "cs2.exe", InitCheatCS2, CheatCs2Main, FALSE , TRUE},
    { "hermes.exe", initializeHermes, pollCommands, FALSE, FALSE }
};

#endif