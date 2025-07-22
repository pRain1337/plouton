#ifndef __plouton_audio_driver_h__
#define __plouton_audio_driver_h__

#include <Base.h>
#include <Uefi/UefiBaseType.h>
#include "../../general/config.h"


// Structures
typedef struct _audioProfile {
    BOOLEAN initialized;
    UINT64 audioChannels;
    EFI_PHYSICAL_ADDRESS audioRingLocation;
} audioProfile_t;

// Define function pointer types
typedef audioProfile_t EFIAPI (*InitAudioDriverFun)(EFI_PHYSICAL_ADDRESS);


// Function defines
#ifdef AUDIO_DRIVER_DUMMY
    audioProfile_t EFIAPI initAudioXHCI(EFI_PHYSICAL_ADDRESS MBAR);
#endif
#ifdef AUDIO_DRIVER_CORSAIR_WIRELESS
    audioProfile_t EFIAPI initCorsairWirelessAudioXHCI(EFI_PHYSICAL_ADDRESS MBAR);
#endif
#ifdef AUDIO_DRIVER_CREATIVE_USB_SPEAKERS
    audioProfile_t EFIAPI initCreativeUsbSpeakersAudioXHCI(EFI_PHYSICAL_ADDRESS MBAR);
#endif
#ifdef AUDIO_DRIVER_HYPERX_CLOUD_2
    audioProfile_t EFIAPI initHyperXCloud2AudioXHCI(EFI_PHYSICAL_ADDRESS MBAR);
#endif
#ifdef AUDIO_DRIVER_LOGITECH_G_PRO_X_2_WIRELESS
    audioProfile_t EFIAPI initLogitechGProX2AudioXHCI(EFI_PHYSICAL_ADDRESS MBAR);
#endif
#ifdef AUDIO_DRIVER_LOGITECH_G_PRO_X_WIRELESS
    audioProfile_t EFIAPI initLogitechGProXAudioXHCI(EFI_PHYSICAL_ADDRESS MBAR);
#endif

static const InitAudioDriverFun InitAudioDriverFuns[] = {
#ifdef AUDIO_DRIVER_DUMMY
	initAudioXHCI,
#endif
#ifdef AUDIO_DRIVER_CORSAIR_WIRELESS
    initCorsairWirelessAudioXHCI,
#endif
#ifdef AUDIO_DRIVER_CREATIVE_USB_SPEAKERS
    initCreativeUsbSpeakersAudioXHCI,
#endif
#ifdef AUDIO_DRIVER_HYPERX_CLOUD_2
    initHyperXCloud2AudioXHCI,
#endif
#ifdef AUDIO_DRIVER_LOGITECH_G_PRO_X_2_WIRELESS
    initLogitechGProX2AudioXHCI,
#endif
#ifdef AUDIO_DRIVER_LOGITECH_G_PRO_X_WIRELESS
    initLogitechGProXAudioXHCI,
#endif
};


#endif