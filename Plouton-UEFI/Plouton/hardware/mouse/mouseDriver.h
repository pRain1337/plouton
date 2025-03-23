#ifndef __plouton_mouse_driver_h__
#define __plouton_mouse_driver_h__

#include <Base.h>
#include <Uefi/UefiBaseType.h>
#include "../../general/config.h"


// Structures
typedef struct _mouseProfile {
    BOOLEAN initialized;
    EFI_PHYSICAL_ADDRESS PhysicalTDLocation;
    EFI_PHYSICAL_ADDRESS PhysicalTDLocationButton;
    EFI_PHYSICAL_ADDRESS PhysicalTDLocationMove;
    EFI_PHYSICAL_ADDRESS PhysicalTDLocation2;
    EFI_PHYSICAL_ADDRESS PhysicalTDLocation2Button;
    EFI_PHYSICAL_ADDRESS PhysicalTDLocation2Move;
} mouseProfile_t;

// Define function pointer types
#ifdef __GNUC__
typedef BOOLEAN EFIAPI (*MouseDriverTdCheck)(EFI_PHYSICAL_ADDRESS);
typedef mouseProfile_t EFIAPI (*InitMouseDriver)(MouseDriverTdCheck);
#else
typedef BOOLEAN (*MouseDriverTdCheck)(EFI_PHYSICAL_ADDRESS);
typedef mouseProfile_t (*InitMouseDriver)(MouseDriverTdCheck);
#endif

typedef struct _mouseInitFuns {
    InitMouseDriver InitMouseDriverFun;
    MouseDriverTdCheck MouseDriverTdCheckFun;
} mouseInitFuns;

// Function defines
#ifdef MOUSE_DRIVER_DUMMY
    mouseProfile_t EFIAPI initDummyMouseXHCI(MouseDriverTdCheck MouseDriverTdCheckFun);
    BOOLEAN EFIAPI mouseDummyDriverTdCheck(EFI_PHYSICAL_ADDRESS TDPointerPhys);
#endif
#ifdef MOUSE_DRIVER_LOGITECH_G_PRO_SUPERLIGHT
    mouseProfile_t EFIAPI initLogitechGProSuperlightMouseXHCI(MouseDriverTdCheck MouseDriverTdCheckFun);
    BOOLEAN EFIAPI mouseLogitechGProSuperlightDriverTdCheck(EFI_PHYSICAL_ADDRESS TDPointerPhys);
#endif
#ifdef MOUSE_DRIVER_LOGITECH_G_PRO
    mouseProfile_t EFIAPI initLogitechGProMouseXHCI(MouseDriverTdCheck MouseDriverTdCheckFun);
    BOOLEAN EFIAPI mouseLogitechGProDriverTdCheck(EFI_PHYSICAL_ADDRESS TDPointerPhys);
#endif

static const mouseInitFuns InitMouseDriversFuns[] = {
#ifdef MOUSE_DRIVER_DUMMY
    { initDummyMouseXHCI, mouseDummyDriverTdCheck},
#endif
#ifdef MOUSE_DRIVER_LOGITECH_G_PRO_SUPERLIGHT
    { initLogitechGProSuperlightMouseXHCI, mouseLogitechGProSuperlightDriverTdCheck },
#endif
#ifdef MOUSE_DRIVER_LOGITECH_G_PRO
    { initLogitechGProMouseXHCI, mouseLogitechGProDriverTdCheck },
#endif
};

#endif