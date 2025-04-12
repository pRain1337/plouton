# Supported devices out of the box
The XHCI functionality has been tested together with the Intel XHCI USB Controller.
The following devices have been tested and work together with SMM in their standard configuration.
  -  Audio devices
      -  Corsair Wireless Headset
      -  Creative Pebble V3 Speaker
      -  Hyper X Cloud 2 Headset
      -  Logitech G Pro X Headset
      -  Logitech G Pro X 2 Headset
  -  Mouse devices
      -  Logitech G Pro
      -  Logitech G Pro Superlight

# Extending devices
Each device that should be supported requires their own .c file, which will only be compiled if the definition has been activated.
To add your own device, the following information has to be gathered first:
  -  General device:
      -  The following information of the USB endpoint that is receiving/sending data:
         -  Context Type
         -  Max Packet Size
         -  Average TRB Length
  -  Mouse specific:
      -  How the first bytes of the USB packets start (identifier)

## Locating USB specific details
These details can be found in two ways, but as a general approach you should identify the Root Port number of the USB device.
To locate the number, we recommend the [System Information Viewer](http://rh-software.com/) where you can open the USB Bus and locate the USB Device and the Port number based on the device name.

### Option 1: Enabling logging in SMM
The parsing of the XHCI and logging of these values has already been added as code in the function `getEndpointRing` in [xhci.c](xhci.c).
Simply lower the logging level of these messages, and then check the values of the endpoints of the Root Port number you identified earlier.

### Option 2: Manually navigating the XHCI structs
In this option, you will be performing the same steps as the code does in [xhci.c](xhci.c).
To directly access the physical memory and PCI config space, we recommend [RWEverything](https://rweverything.com/).

#### Memory Base Address (MBAR)
First access the PCI Config space of the XHCI USB Controller (Bus 00, Device 14, Function 00).
At offset 0x10, you can find the 64-bit pointer of the Memory Base address (MBAR). Null the last byte and save this value.
![RWEverything showing the PCI Config space and offset to the Memory Base Address](/images/XHCI_MemoryBaseAddress.png)

#### Device Context Array Base (DCAB)
Now access this MBAR using the Memory option in RWEverything, and read the 8-bit value of the capability register length.   
This length (0x80 in the picture) is used to skip the capability register, as afterwards the Host Operational register follows.   
At offset 0x30 in the Host Operational register we can finally read the Device Context Array Base (DCAB) (Final address: MBAR Address + Capability register length + 0x30 = 0xB0).   
This points to an array containing pointers to each connected USB device on the system.
![RWEverything showing the Memory Base Address and offset to the Device Context Array Base](/images/XHCI_DeviceContextArrayBase.png)

#### Device Contexts
Navigate to the DCAB, where there will be an array of 64-byte pointers, each of them for a separate device.
![RWEverything showing the Device Context Array Base and all the pointers to the device contexts](/images/XHCI_DeviceContexts.png)

#### Root Port number
Simply navigate to each of those devices now, and check the 8-bit value at offset 0x6 which is the Root Port number.
Search for the device with the same Root Port number as previously identified for your device.
![RWEverything showing a Device Context and the offset to the Root Port number](/images/XHCI_RootPortNumber.png)

#### Endpoints
At offset 0x20, the array of endpoints starts. Each endpoint struct has a size of 0x20. 
Each of these endpoints could be for the input/output data which is required for the interaction.
![RWEverything showing the endpoints of a device](/images/XHCI_Endpoints.png)

To identify the right endpoint, navigate to the memory address at offset 0x8, which points to the transfer ring.
Each packet received/sent via this endpoint will be written to this ring top to down, thus the right endpoint can be identified by interacting with the device (e.g. move mouse or play music).

When the right endpoint is identified, note down the following values:
  -  Type: Offset 0x4, read 8-bit value, perform a right shift 3 for the type value (3-bit size, in bits 5:3).
  -  Max packet size: Offset 0x6, 16-bit value.
  -  Average TRB Length: Offset 0x10, 16-bit value.

For further information, see our implementation in [xhci.c](xhci.c) or the XHCI specification from Intel (eXtensible Host Controller Interface for Universal Serial Bus (xHCI), Table 6-9 until 6-11).   

To further limit the number of endpoints down and prevent conflicts, also analyze the raw packet that can be found in the transfer ring previously identified.
For newer Logitech mouses, we have identified the magic value "0x409301" at offset 0x8 which we use to 100% identify it as our device.
Investigate if your device has a similar magic value, it is not required but recommended.

## Extending driver code
Now with all values gathered, a new driver can be added as code.

### Extending Audio devices

Create a new .c -file for your audio device, and guard it with C `#ifdef`s. Please see an example from the [dummy.c](audio/dummy.c)
file. Inside this file, you should define a `InitAudioDriverFun` function (`typedef audioProfile_t EFIAPI (*InitAudioDriverFun)(VOID);`).
Inside this function, you will be adding the previously identified values in the call to `getEndpointRing`. See the other implementations for further help.
For this, please again see example from [dummy.c](audio/dummy.c). This function should return a `audioProfile_t` structure,
including a `BOOLEAN` whether the device was initialized successfully, the number of audio channels of this device (`UINT64 audioChannels`)
and the memory location of this device's Audio ring (`EFI_PHYSICAL_ADDRESS audioRingLocation`).
To implement, please find examples from the [audio](audio) directory.

After implementing your driver, add its declaration and entry to the `InitAudioDriverFuns` in [audioDriver.h](audio/audioDriver.h) file.

You also need to edit the [Plouton.inf](../Plouton.inf) and add your implementation's .c file(s) to the list of to-be-compiled objects.

After aforementioned steps, you should now be able to use your driver by defining your chosen `#ifdef` guard name inside [config.h](../general/config.h) 

### Extending Mouse devices

Extending mouse devices work the similar way as extending for new audio devices, but instead of having a single `InitAudioDriverFun`,
the mouse drivers require two functions: `InitMouseDriver` (`typedef mouseProfile_t EFIAPI (*InitMouseDriver)(MouseDriverTdCheck);`)
and `MouseDriverTdCheck` (`typedef BOOLEAN EFIAPI (*MouseDriverTdCheck)(EFI_PHYSICAL_ADDRESS);`). The latter is used at the very last
stage of `getEndpointRing` in [xhci.c](xhci.c) to make sure that the device is really what we are looking for. Since we have two function pointers,
after implementing the aforementioned functions, you should add their declarations and then add them in a new struct entry
to the `InitMouseDriversFuns` array in [mouseDriver.h](mouse/mouseDriver.h) file. (Instead of just a single function pointer in audio side!)
