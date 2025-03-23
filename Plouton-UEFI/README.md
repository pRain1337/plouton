# BUILDING
For building the Plouton module, we have outlined two possibilities, either using a docker container or doing it manually on windows.   

## Submodule init (EDK2)

In order to retrieve the right tag of EDK2 (vUDK2018):

```
git submodule update --init
```

This only needs to be done once

## Option 1: Building on Docker (Windows, Linux, OS X)

1. Either pull the docker image from dockerhub:

```
# docker pull jussihi/edk-builder
```

**or**

Inside this directory (Plouton-UEFI), run

```
# docker build --no-cache -t jussihi/edk-builder .
```


2. Inside this directory, run docker and its build script to build firmware image:

```
# docker run --privileged -v .:/root/ -u root -w /root jussihi/edk-builder /bin/bash docker_build.sh
```

The resulting OVMF firmware will be inside edk2/Build/OvmfX64/RELEASE_GCC5/FV.

## Option 2: Building on Windows
To get started you first have to setup a build environment for [edk2](https://github.com/tianocore/edk2).   
We recommend following the instructions from this [video](https://www.youtube.com/watch?v=jrY4oqgHV0o) to setup the build environment on windows, but choosing the vUDK2018 release instead of the newest one, as Plouton has been tested using this version.   

With the edk2 setup ready, you will have to add Plouton as a build target for it to get compiled.   
First modify the file `edk2\Conf\target.txt` with the following text:   
  -  Change `ACTIVE_PLATFORM` to `OvmfPkg/OvmfPkgX64.dsc`
  -  Change `TARGET` to `RELEASE`
  -  Change `TARGET_ARCH` to `X64`

edk2 will compile the modules mentioned in `OvmfPkg/OvmfPkgX64.dsc`, thus we have to add the Plouton module in the file `edk2\OvmfPkg\OvmfPkgX64.dsc`.   
  -  Add the path to the Plouton module .INF file from the edk2 build environment at the bottom of the file,  e.g. `Plouton-UEFI/Plouton/Plouton.inf`    

Now with all requirements set, open a Visual Studio command prompt, navigate to the edk2 root folder and prepare the environment with `edksetup.bat rebuild`.    
When this is finished, navigate to the Plouton folder and enter `build`.   

If you have problems setting it up, we also recommend taking a look at the alternative approach by [Zepta](https://github.com/Oliver-1-1/SmmInfect?tab=readme-ov-file#compiling-and-installation).   

## Running on real HW

### Preparing the firmware image

As a result of the compilation, you should receive an .EFI file.   
This .EFI file will be added to your firmware image, for this we recommend first dumping your firmware image using a external SPI programmer or using onboard tools (Intel Flash Programming tools).   
To patch the firmware image, we recommend using the old version of [UEFITool](https://github.com/LongSoft/UEFITool/releases/tag/0.28.0).   

#### How to patch PiSmmCpuDxeSmm 
If you are trying to run this SMM module on recent real hardware (after ~2021), you need to patch
your motherboard's `PiSmmCpuDxeSmm` module from the UEFI firmware. Modern
SMM protections setup by edk2 will produce a fault otherwise when accessing
normal OS memory. 
You can mimick
[our patch](https://github.com/jussihi/SMM-Rootkit/tree/master/SMM%20Rootkit/UefiCpuPkg)
by Patching this variable initialization out and hard code the variable itself to
0 with your favorite disassembler (IDA or similar):

https://github.com/tianocore/edk2/blob/edk2-stable202105/UefiCpuPkg/PiSmmCpuDxeSmm/X64/PageTbl.c#L387

Easiest way to find that function (SmmInitPageTable) is to search for the
strings of the error messages:

https://github.com/tianocore/edk2/blob/edk2-stable202105/UefiCpuPkg/Library/CpuExceptionHandlerLib/X64/ArchExceptionHandler.c#L278

Which is referenced multiple times in the SMI Page fault handler:

https://github.com/tianocore/edk2/blob/edk2-stable202105/UefiCpuPkg/PiSmmCpuDxeSmm/X64/PageTbl.c#L1014

And the page fault handler is initialized in the same function as the variable
initialization (SmmInitPageTable):

https://github.com/tianocore/edk2/blob/edk2-stable202105/UefiCpuPkg/PiSmmCpuDxeSmm/X64/PageTbl.c#L476

After the patch is performed, replace the PE32 Image body of the `PiSmmCpuDxeSmm` in the image.   

#### Adding the Plouton module
To add the plouton module into the firmware image, you can directly replace a SMM module as some of them are not crucial for the system execution.   
To locate the less relevant modules, we recommend the newer UEFITool versions or just looking up the GUIDs of the module online.   
Simply select the "PE32 Image section" of the module and replace the body with the Plouton .EFI file.   
![Shows a module in UEFITool with the "Replace Body" option selected](/images/UEFITool_ReplaceBody.png)

Afterwards save the image using UEFITool.   

### Flashing the firmware image
Now with the image prepared, you can flash it to the mainboard using an external SPI programmer.   
Manufacturers have started to crack down on SPI Protection, thus it is not as easy anymore to just overwrite the SPI chip from the operating system.   

There are numerous guides/manuals how to setup such an external SPI programmer, which is why we will not go further down in the specifics here.   
Our recommendations are to use [flashrom](https://flashrom.org/) together with a Raspberry PI and using a SOIC-8 clip to directly attach to the SPI chip.   

**Always make sure to disconnect the power before using any external SPI programmer!**

Commands used by us together with flashrom:
  -  Reading:
      -  `flashrom -p linux_spi:dev=/dev/spidev0.0,spispeed=512 -n -r <filename>`
  -  Writing:
      -  `flashrom -p linux_spi:dev=/dev/spidev0.0,spispeed=512 -n -w <filename>`

Good tutorials:   
  -  [[Guide] How to Use a CH341A SPI Programmer/Flasher (with Pictures!)](https://winraid.level1techs.com/t/guide-how-to-use-a-ch341a-spi-programmer-flasher-with-pictures/33041)
  -  [Recover Bricked BIOS using FlashRom on a Raspberry Pi](https://www.youtube.com/watch?v=KNy-_ZzMnG0)
  -  [How to flash bios chips with Raspberry Pi](https://tomvanveen.eu/flashing-bios-chip-raspberry-pi/)

## FAQ

### Flashrom does not recognize the chip
Verify that the clip is properly attached to the chip and connected to the bottom of the chip.   
Verify that the wiring has been properly done after the SPI specs.   

### No serial output after boot

Sometimes the serial traffic is blocked because of the operating system's own serial
driver. This is at least the case in Windows systems when not booting inside
a (QEMU/KVM) virtual machine.

#### There are two ways to get serial working on this sort of situation:

- Block the Operating System from loading the driver.

On GNU+Linux, you can disable the driver completely if one is loaded. On
Windows systems, you might need to rename/delete the system's serial driver.
The default path to the driver executable is
`C:\Windows\System32\drivers\serial.sys`.

- Open an SSH client locally

You can also open the connection to the local serial port using your favorite
serial client. At least on Windows this will prevent Windows own driver from
suppressing the serial output.
